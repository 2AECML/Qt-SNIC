#include "MainWindow.h"

#include <QFileDialog>
#include <QDebug>
#include <gdal_priv.h>
#include <QMenuBar>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), mDataset(nullptr), mSegResult(nullptr), mPolygonManager(nullptr) {
    initUI();
}

MainWindow::~MainWindow() {
    
}

void MainWindow::initUI() {
    setWindowTitle(QString::fromUtf16(u"Segmentation"));
    resize(1024, 768); 

    mGraphicsView = new CustomGraphicsView(this);
    mGraphicsScene = new CustomGraphicsScene(this);

    mGraphicsScene->setBackgroundBrush(QBrush(QColor(65, 105, 225)));
    mGraphicsView->setCacheMode(QGraphicsView::CacheBackground);
    mGraphicsView->setScene(mGraphicsScene);
    mGraphicsView->setGeometry(10, 50, 1000, 700);

    // Setup menu bar
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu* fileMenu = new QMenu("File", this);
    QAction* openFileAction = new QAction("Open File", this);
    QAction* exportResultAction = new QAction("Export Result", this);
    QAction* quitAction = new QAction("Quit", this);

    connect(openFileAction, &QAction::triggered, this, &MainWindow::loadImage);
    connect(exportResultAction, &QAction::triggered, this, &MainWindow::exportResult);
    connect(quitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);

    fileMenu->addAction(openFileAction);
    fileMenu->addAction(exportResultAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    QMenu* editMenu = new QMenu("Edit", this);
    QAction* aboutAction = new QAction("About", this);
    //connect(aboutAction, &QAction::triggered, this, &MainWindow::about);
    editMenu->addAction(aboutAction);

    menuBar->addMenu(fileMenu);
    menuBar->addMenu(editMenu);

    setStyleSheet(R"(/* 设置整个应用程序的背景颜色和字体 */
                    QWidget {
                        background-color: #f5f5f5;
                        font-family: Arial, sans-serif;
                        font-size: 14px;
                        color: #333;
                    }

                    /* 主窗口的标题栏 */
                    QMainWindow {
                        background-color: #ffffff;
                    }

                    /* 菜单栏 */
                    QMenuBar {
                        background-color: #003366;
                        color: #ffffff;
                        height: 40px; /* 设置菜单栏的高度 */
                        padding: 0;   /* 清除内边距 */
                    }

                    QMenuBar::item {
                        background-color: #003366;
                        color: #ffffff;
                        padding: 0 10px; /* 增加菜单项的内边距 */
                    }

                    QMenuBar::item:selected {
                        background-color: #0055cc;
                        color: #ffffff;
                    }

                    /* 菜单 */
                    QMenu {
                        background-color: #ffffff;
                        border: 1px solid #cccccc;
                        padding: 5px;
                    }

                    QMenu::item:selected {
                        background-color: #0055cc;
                        color: #ffffff;
                    }

                    /* 按钮 */
                    QPushButton {
                        background-color: #0055cc;
                        color: #ffffff;
                        border: none;
                        padding: 10px 20px;
                        border-radius: 5px;
                        font-size: 14px;
                    }

                    QPushButton:hover {
                        background-color: #003d99;
                    }

                    /* 普通文本框 */
                    QLineEdit {
                        background-color: #ffffff;
                        border: 1px solid #cccccc;
                        border-radius: 5px;
                        padding: 5px;
                    }

                    /* 多行文本框 */
                    QTextEdit {
                        background-color: #ffffff;
                        border: 1px solid #cccccc;
                        border-radius: 5px;
                        padding: 5px;
                    }

                    /* 窗口中的图像视图 */
                    QGraphicsView {
                        border: 1px solid #cccccc;
                        background-color: #ffffff;
                    }

                    /* 状态栏 */
                    QStatusBar {
                        background-color: #003366;
                        color: #ffffff;
                        height: 30px; /* 设置状态栏的高度 */
                        padding: 0;   /* 清除内边距 */
                    }
                    )");
}

void MainWindow::loadImage() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.xpm *.jpg);;All Files (*)");
    if (!fileName.isEmpty()) {
        mGraphicsView->resetCachedContent();
        mGraphicsScene->clear();
        mImageFileName = fileName.toStdString();
        showImage();
        executeSNIC();
        exportResult();
        showPolygons();

        qDebug() << "mGraphicsView->sceneRect()" << mGraphicsView->sceneRect();
        qDebug() << "mGraphicsScene->sceneRect()" << mGraphicsScene->sceneRect();

        mGraphicsView->resetScale();
        mGraphicsScene->setSceneRect(mGraphicsScene->itemsBoundingRect());
        mGraphicsView->fitInView(mGraphicsScene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void MainWindow::showImage() {
    QPixmap pixmap(mImageFileName.c_str());
    QGraphicsPixmapItem* image = new QGraphicsPixmapItem(pixmap);
    mGraphicsScene->addItem(image);
}

void MainWindow::executeSNIC() {
    if (mImageFileName.empty()) {
        qDebug() << "The split could not be completed because the image path is invalid";
        return;
    }

    getDataSet();

    if (mDataset == nullptr) {
        qDebug() << "The split could not be completed because there is no data in dataset";
        return;
    }

    int w = mDataset->GetRasterXSize();
    int h = mDataset->GetRasterYSize();
    int c = mDataset->GetRasterCount();

    // 读取图像数据
    double* pinp = new double[w * h * c];
    for (int i = 0; i < c; ++i) {
        GDALRasterBand* band = mDataset->GetRasterBand(i + 1);
        CPLErr err = band->RasterIO(GF_Read, 0, 0, w, h, pinp + i * w * h, w, h, GDT_Float64, 0, 0);
        if (err) {
            qDebug() << "读取栅格数据时出错: " << CPLGetLastErrorMsg();
        }
    }

    int numsuperpixels = 500; // 超像素数量
    double compactness = 20.0; // 紧凑度
    bool doRGBtoLAB = true; // 是否进行RGB到LAB的转换
    int* plabels = new int[w * h]; // 存储分割结果的标签
    int* pnumlabels = new int[1]; // 存储标签数量

    qDebug() << "开始进行图像分割...";

    SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

    qDebug() << "图像分割完成";

    mSegResult.reset(new SegmentationResult(plabels, pnumlabels, w, h));

    delete[] pinp;
    delete[] plabels;
    delete[] pnumlabels;
}

void MainWindow::exportResult() {
    mSegResult->exportToCSV();
}

void MainWindow::getDataSet() {
    const char* cFileName = mImageFileName.c_str();
    GDALAllRegister();
    mDataset.reset(static_cast<GDALDataset*>(GDALOpen(cFileName, GA_ReadOnly)));
    if (mDataset == nullptr) {
        qDebug() << "无法打开文件";
        return;
    }

    // 获取图像的基本信息
    int nRasterXSize = mDataset->GetRasterXSize();
    int nRasterYSize = mDataset->GetRasterYSize();
    int nBands = mDataset->GetRasterCount();

    qDebug() << "图像宽度: " << nRasterXSize;
    qDebug() << "图像高度: " << nRasterYSize;
    qDebug() << "波段数量: " << nBands;

}

void MainWindow::showPolygons() {

    mPolygonManager.reset(new PolygonManager(mGraphicsScene, mSegResult.get()));

    mPolygonManager->generatePolygons();

    mPolygonManager->showAllPolygons();
}

QPolygon MainWindow::convertOGRPolygonToQPolygon(const OGRPolygon* ogrPolygon) {
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
