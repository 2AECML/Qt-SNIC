#include "MainWindow.h"
#include <QFileDialog>
#include <QDebug>
#include <gdal_priv.h>
#include <QMenuBar>
#include <QApplication>
#include <QStatusBar>
#include <QTimer>
#include <QTextCodeC>
#include <QColorDialog>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , mDataset(nullptr)
    , mSegResult(nullptr)
    , mImageItem(nullptr)
    , mPolygonManager(nullptr) {

    initUI();
}

MainWindow::~MainWindow() {
    for (auto& thread : mProcessingThreadList) {
        if (thread) {
            thread->quit(); // 请求线程停止
            thread->wait(); // 等待线程结束
        }
    }
    mProcessingThreadList.clear(); // 清空列表，unique_ptr 会自动删除线程对象
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
    QAction* openFileAction = new QAction("Open Image", this);
    QAction* exportResultAction = new QAction("Save Result", this);
    QAction* quitAction = new QAction("Quit", this);
    connect(openFileAction, &QAction::triggered, this, &MainWindow::loadImage);
    connect(exportResultAction, &QAction::triggered, this, &MainWindow::exportResult);
    connect(quitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);
    fileMenu->addAction(openFileAction);
    fileMenu->addAction(exportResultAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    QMenu* segmentMenu = new QMenu("Segmentation", this);
    QAction* segmentAction = new QAction("Start", this);
    connect(segmentAction, &QAction::triggered, this, &MainWindow::segmentImage);
    segmentMenu->addAction(segmentAction);

    QMenu* editMenu = new QMenu("Edit", this);
    QAction* setDefaultColorAction = new QAction("Set Default Border Color", this);
    QAction* setSelectedColorAction = new QAction("Set Selected Border Color", this);
    QAction* setHoveredColorAction = new QAction("Set Hovered Border Color", this);
    connect(setDefaultColorAction, &QAction::triggered, this, &MainWindow::changeDefaultBorderColor);
    connect(setSelectedColorAction, &QAction::triggered, this, &MainWindow::changeSelectedBorderColor);
    connect(setHoveredColorAction, &QAction::triggered, this, &MainWindow::changeHoveredBorderColor);
    editMenu->addAction(setDefaultColorAction);
    editMenu->addAction(setSelectedColorAction);
    editMenu->addAction(setHoveredColorAction);

    menuBar->addMenu(fileMenu);
    menuBar->addMenu(segmentMenu);
    menuBar->addMenu(editMenu);

    mProgressBar = new QProgressBar(this);
    statusBar()->addWidget(mProgressBar);
    mProgressBar->setVisible(false);
    mProgressBar->setMaximum(100);

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
                        color: #ffffff;
                        height: 30px; /* 设置状态栏的高度 */
                        padding: 0;   /* 清除内边距 */
                    }

                    QProgressBar {
                        text-align:center;
                        background-color:#DDDDDD;
                        border: 0px solid #DDDDDD;
                        border-radius:5px;
                    }
                    
                    QProgressBar::chunk {
                        background-color:#05B8CC; 
                        border-radius: 5px;
                    }

                    QStatusBar {
                        color: #ffffff;
                        height: 30px; /* 设置状态栏的高度 */
                        padding: 0;   /* 清除内边距 */
                        border: none; /* 禁止右下角拉伸 */
                    }

                    QScrollBar:vertical {
                        border: 2px solid #cccccc;
                        background: #f5f5f5;
                        width: 12px;
                        border-radius: 6px;
                    }

                    QScrollBar::handle:vertical {
                        background: #0055cc;
                        min-height: 20px;
                        border-radius: 6px;
                    }

                    QScrollBar::add-line:vertical {
                        border: 2px solid #cccccc;
                        background: #f5f5f5;
                        height: 20px;
                        border-radius: 6px;
                        subcontrol-position: bottom;
                        subcontrol-origin: margin;
                    }

                    QScrollBar::sub-line:vertical {
                        border: 2px solid #cccccc;
                        background: #f5f5f5;
                        height: 20px;
                        border-radius: 6px;
                        subcontrol-position: top;
                        subcontrol-origin: margin;
                    }

                    QScrollBar:horizontal {
                        border: 2px solid #cccccc;
                        background: #f5f5f5;
                        height: 12px;
                        border-radius: 6px;
                    }

                    QScrollBar::handle:horizontal {
                        background: #0055cc;
                        min-width: 20px;
                        border-radius: 6px;
                    }

                    QScrollBar::add-line:horizontal {
                        border: 2px solid #cccccc;
                        background: #f5f5f5;
                        width: 20px;
                        border-radius: 6px;
                        subcontrol-position: right;
                        subcontrol-origin: margin;
                    }

                    QScrollBar::sub-line:horizontal {
                        border: 2px solid #cccccc;
                        background: #f5f5f5;
                        width: 20px;
                        border-radius: 6px;
                        subcontrol-position: left;
                        subcontrol-origin: margin;
                    }
    )");
}

void MainWindow::loadImage() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.xpm *.jpg);;All Files (*)");
    if (!fileName.isEmpty()) {
        mGraphicsView->resetCachedContent();
        clearImage();
        mPolygonManager.reset();
        mImageFileName = fileName.toStdString();
        showImage();
        mGraphicsView->resetScale();
        mGraphicsScene->setSceneRect(mGraphicsScene->itemsBoundingRect());
        mGraphicsView->fitInView(mGraphicsScene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void MainWindow::showImage() {
    QPixmap pixmap(mImageFileName.c_str());
    mImageItem = new QGraphicsPixmapItem(pixmap);
    mGraphicsScene->addItem(mImageItem);
}

void MainWindow::clearImage() {
    if (mImageItem) {
        mGraphicsScene->removeItem(mImageItem);
        delete mImageItem;
        mImageItem = nullptr;
    }
}

void MainWindow::executeSNIC() {
    if (mImageFileName.empty()) {
        qDebug() << "The split could not be completed because the image path is invalid";
        return;
    }

    emit doingGetDataset();

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

    emit doingSegment();

    int numsuperpixels = 500; // 超像素数量
    double compactness = 20.0; // 紧凑度
    bool doRGBtoLAB = true; // 是否进行RGB到LAB的转换
    int* plabels = new int[w * h]; // 存储分割结果的标签
    int* pnumlabels = new int[1]; // 存储标签数量

    qDebug() << "开始进行图像分割...";

    SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

    qDebug() << "图像分割完成";

    emit doingInitResult();

    mSegResult.reset(new SegmentationResult(plabels, pnumlabels, w, h));

    delete[] pinp;
    delete[] plabels;
    delete[] pnumlabels;
}

void MainWindow::exportResult() {
    auto task = [this]() {
        mSegResult->exportToCSV();
    };

    // 创建并启动自定义线程
    mProcessingThreadList.emplace_back(std::make_unique<CustomThread>(task, this));

    auto& processingThread = mProcessingThreadList.back();

    connect(processingThread.get(), &CustomThread::finished, this, [this]() {
        
    });
    processingThread->start();
}

void MainWindow::segmentImage() {
    mProgressBar->setVisible(true); // 显示进度条
    mProgressBar->setValue(0); // 初始化进度为0

    auto task = [this]() {
        executeSNIC();
    };

    // 创建并启动自定义线程
    mProcessingThreadList.emplace_back(std::make_unique<CustomThread>(task, this));

    auto& processingThread = mProcessingThreadList.back();

    connect(processingThread.get(), &CustomThread::finished, this, [this]() {
        showPolygons();

        mProgressBar->setValue(100); // 更新进度条的值
        // 使用 QTimer 延迟两秒隐藏进度条
        QTimer::singleShot(2000, [this]() {
            mProgressBar->setVisible(false);
        });
    });

    connect(this, &MainWindow::doingGetDataset, this, [this]() {
        mProgressBar->setValue(10); // 更新进度条的值
    });

    connect(this, &MainWindow::doingSegment, this, [this]() {
        mProgressBar->setValue(30); // 更新进度条的值
    });

    connect(this, &MainWindow::doingInitResult, this, [this]() {
        mProgressBar->setValue(60); // 更新进度条的值
    });
    
    processingThread->start();
}

void MainWindow::changeDefaultBorderColor() {
    QColor color = QColorDialog::getColor(Qt::black, this, "Set Default Border Color");
    if (color.isValid()) {
        mPolygonManager->setDefaultBorderColor(color);
    }
}

void MainWindow::changeHoveredBorderColor() {
    QColor color = QColorDialog::getColor(Qt::black, this, "Set Hovered Border Color");
    if (color.isValid()) {
        mPolygonManager->setHoveredBorderColor(color);
    }
}

void MainWindow::changeSelectedBorderColor() {
    QColor color = QColorDialog::getColor(Qt::black, this, "Set Selected Border Color");
    if (color.isValid()) {
        mPolygonManager->setSelectedBorderColor(color);
    }
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

