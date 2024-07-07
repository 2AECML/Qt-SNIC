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
        for (int i = 0; i < mSegResult->getPolygonCount(); ++i) {
            std::cout << "正在绘制第" << i + 1 << "个多边形..." << std::endl;

            // 获取第i个标签对应的OGRPolygon
            const OGRPolygon* ogrPolygon = mSegResult->getPolygonByLabel(i);

            // 转换为QPolygon
            QPolygon qPolygon = convertOGRPolygonToQPolygon(ogrPolygon);


            //QPolygon qPolygon({ QPoint(188,188), QPoint(189,188), QPoint(190,188), QPoint(191,188), QPoint(192,188), QPoint(193,188), QPoint(194,188), QPoint(195,188), QPoint(196,188), QPoint(188,189), QPoint(197,189), QPoint(198,189), QPoint(199,189), QPoint(188,190), QPoint(199,190), QPoint(188,191), QPoint(199,191), QPoint(188,192), QPoint(199,192), QPoint(188,193), QPoint(199,193), QPoint(188,194), QPoint(199,194), QPoint(188,195), QPoint(199,195), QPoint(188,196), QPoint(199,196), QPoint(188,197), QPoint(199,197), QPoint(188,198), QPoint(199,198), QPoint(189,199), QPoint(190,199), QPoint(191,199), QPoint(192,199), QPoint(193,199), QPoint(194,199), QPoint(195,199), QPoint(196,199), QPoint(197,199), QPoint(198,199), QPoint(199,199), QPoint(188,188) });

            //qDebug() << "Polygon:" << qPolygon;

            CustomPolygonItem* polygonItem = new CustomPolygonItem(qPolygon);

            mPolygonItemMap[i] = polygonItem;

            mPolygonItemMap[i]->setLabel(i);

            mPolygonItemMap[i]->setPen(QPen(Qt::black, 1));

            // 绘制多边形
            mGraphicsScene->addItem(polygonItem);
            
            
        }
        //mPolygonItemMap[100]->setPen(QPen(Qt::gray, 0.5));

        //mGraphicsScene->addPolygon(polygon1);
        //mGraphicsScene->addPolygon(polygon2);

        //QPolygon polygon1 = mPolygonItemMap[0]->polygon().toPolygon();
        //QPolygon polygon2 = mPolygonItemMap[2]->polygon().toPolygon();

        //QRegion region1(polygon1);
        //QRegion region2(polygon2);

        //QRegion region = region1.united(region2);

        //qDebug() << "Region:" << region;

        //QPolygon mergedPolygon;
        //for (const QRect& rect : region) {
        //    mergedPolygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft();
        //}

        //QGraphicsPolygonItem* mergedPolygonItem = mGraphicsScene->addPolygon(mergedPolygon);
        //mergedPolygonItem->setPen(QPen(Qt::red, 2)); // 设置画笔颜色和宽度


    }

    QPolygon convertOGRPolygonToQPolygon(const OGRPolygon* ogrPolygon) {
        QPolygon polygon;
        const OGRLinearRing* exteriorRing = ogrPolygon->getExteriorRing();
        int pointCount = exteriorRing->getNumPoints();
        for (int i = 0; i < pointCount; ++i) {
            OGRPoint point;
            exteriorRing->getPoint(i, &point);
            polygon << QPoint(point.getX(), point.getY());
            //mGraphicsScene->addItem(new QGraphicsRectItem(point.getX(), point.getY(), 1, 1));
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