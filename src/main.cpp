#include "main.h"
#include "snic.h"
#include <iostream>
#include <gdal_priv.h>
#include <QMainWindow>
#include <QApplication>
#include <QPushButton>
#include <QFileDialog>
#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsPolygonItem>
#include <QDebug>
#include <QPolygon>
#include <QPoint>
#include "CustomGraphicsView.h"
#include "SegmentationResult.cpp"
#include "CustomPolygonItem.cpp"


extern "C" {
    void SNIC_main(double* pinp, int w, int h, int c, int numsuperpixels, double compactness, bool doRGBtoLAB, int* plabels, int* pnumlabels);
}


class SNICApp : public QMainWindow {
    Q_OBJECT

public:
    SNICApp(QWidget* parent = nullptr) : QMainWindow(parent), mDataset(nullptr), mSegResult(nullptr) {
        initUI();
    }

    ~SNICApp() {
        if (mDataset != nullptr) {
            GDALClose(mDataset);
        }

        if (mButton != nullptr) {
            delete mButton;
        }

        if (mGraphicsScene != nullptr) {
            delete mGraphicsScene;
        }

        if (mGraphicsView != nullptr) {
            delete mGraphicsView;
        }

        if (mSegResult != nullptr) {
            delete mSegResult;
        }


    }

private slots:
    void loadImage() {

        QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.xpm *.jpg);;All Files (*)");
        if (!fileName.isEmpty()) {
            mGraphicsView->resetCachedContent();
            mGraphicsScene->clear();
            mImageFileName = fileName.toStdString();
            showImage();
            executeSNIC();
            exportResult();
            showPolygons();
        }
    }

private:
    void initUI() {
        setWindowTitle(QString::fromUtf16(u"SNIC分割"));
        setFixedSize(800, 600);

        mButton = new QPushButton("Load Image", this);
        mButton->setGeometry(10, 10, 100, 30);
        QObject::connect(mButton, &QPushButton::clicked, this, &SNICApp::loadImage);

        mGraphicsView = new CustomGraphicsView(this);
        mGraphicsScene = new QGraphicsScene(this);
        mGraphicsView->setScene(mGraphicsScene);
        mGraphicsView->setGeometry(10, 50, 780, 540);

    }

    void showImage() {
        // 加载图片
        QPixmap pixmap(mImageFileName.c_str());

        QGraphicsPixmapItem* image = new QGraphicsPixmapItem(pixmap);

        mGraphicsScene->addItem(image);
    }

    void executeSNIC() {

        const char* fileName = mImageFileName.c_str();

        if (fileName != nullptr) {
            getDataSet();

            if (mDataset != nullptr) {
                segment();
                std::cout << "分割结果标签数量: " << mSegResult->getLabelCount() << std::endl;
                
            }
            
        }
    }

    void exportResult() {
        mSegResult->exportToCSV();   // 导出结果到CSV文件
    }

    void getDataSet() {

        const char* fileName = mImageFileName.c_str();

        GDALAllRegister();

        mDataset = (GDALDataset*)GDALOpen(fileName, GA_ReadOnly);
        if (mDataset == NULL) {
            std::cout << "无法打开文件" << std::endl;
            return;
        }

        // 获取图像的基本信息
        int nRasterXSize = mDataset->GetRasterXSize();
        int nRasterYSize = mDataset->GetRasterYSize();
        int nBands = mDataset->GetRasterCount();

        std::cout << "图像宽度: " << nRasterXSize << std::endl;
        std::cout << "图像高度: " << nRasterYSize << std::endl;
        std::cout << "波段数量: " << nBands << std::endl;

    }

    void segment() {
        if (mDataset == nullptr) {
            std::cout << "无法获取数据集" << std::endl;
            return;
        }

        int w = mDataset->GetRasterXSize();
        int h = mDataset->GetRasterYSize();
        int c = mDataset->GetRasterCount();

        // 读取图像数据
        double* pinp = new double[w * h * c];
        for (int i = 0; i < c; ++i) {
            GDALRasterBand* band = mDataset->GetRasterBand(i + 1);
            band->RasterIO(GF_Read, 0, 0, w, h, pinp + i * w * h, w, h, GDT_Float64, 0, 0);
        }

        int numsuperpixels = 500; // 超像素数量
        double compactness = 20.0; // 紧凑度
        bool doRGBtoLAB = true; // 是否进行RGB到LAB的转换
        int* plabels = new int[w * h]; // 存储分割结果的标签
        int* pnumlabels = new int[1]; // 存储标签数量

        std::cout << "开始进行图像分割..." << std::endl;

        SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

        std::cout << "图像分割完成" << std::endl;

        mSegResult = new SegmentationResult(plabels, pnumlabels, w, h);

        delete[] pinp;
        delete[] plabels;
        delete[] pnumlabels;

    }

    void showPolygons() {
        mSegResult->generatePolygons();

        // 合并标签
        std::vector<int> labels = { 1, 2, 3 };
        mSegResult->mergeLabels(labels);


        for (int i = 0; i < mSegResult->getLabelCount(); ++i) {
            std::cout << "正在绘制第" << i + 1 << "个多边形..." << std::endl;

            // 获取第i个标签对应的OGRPolygon
            if (mSegResult->getPolygonByLabel(i) == nullptr) {
                continue;
            }
            const OGRPolygon* ogrPolygon = mSegResult->getPolygonByLabel(i);

            // 转换为QPolygon
            QPolygon qPolygon = convertOGRPolygonToQPolygon(ogrPolygon);

            CustomPolygonItem* polygonItem = new CustomPolygonItem(qPolygon);

            mPolygonItemMap[i] = polygonItem;

            mPolygonItemMap[i]->setLabel(i);

            mPolygonItemMap[i]->setPen(QPen(Qt::black, 1));

            // 绘制多边形
            mGraphicsScene->addItem(polygonItem);
            
            
        }


    }

    QPolygon convertOGRPolygonToQPolygon(const OGRPolygon* ogrPolygon) {
        QPolygon polygon;
        const OGRLinearRing* exteriorRing = ogrPolygon->getExteriorRing();
        int pointCount = exteriorRing->getNumPoints();
        for (int i = 0; i < pointCount; ++i) {
            OGRPoint point;
            exteriorRing->getPoint(i, &point);
            polygon << QPoint(point.getX(), point.getY());
        }
        return polygon;
    }

private:
    QPushButton* mButton;
    std::string mImageFileName;
    CustomGraphicsView* mGraphicsView;
    QGraphicsScene* mGraphicsScene;
    GDALDataset* mDataset;
    SegmentationResult* mSegResult;
    std::map<int, CustomPolygonItem*> mPolygonItemMap;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    SNICApp window;
    window.show();
    return app.exec();
}

#include "main.moc"