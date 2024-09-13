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
            thread->quit(); // �����߳�ֹͣ
            thread->wait(); // �ȴ��߳̽���
        }
    }
    mProcessingThreadList.clear(); // ����б�unique_ptr ���Զ�ɾ���̶߳���
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

    setStyleSheet(R"(/* ��������Ӧ�ó���ı�����ɫ������ */
                    QWidget {
                        background-color: #f5f5f5;
                        font-family: Arial, sans-serif;
                        font-size: 14px;
                        color: #333;
                    }

                    /* �����ڵı����� */
                    QMainWindow {
                        background-color: #ffffff;
                    }

                    /* �˵��� */
                    QMenuBar {
                        background-color: #003366;
                        color: #ffffff;
                        height: 40px; /* ���ò˵����ĸ߶� */
                        padding: 0;   /* ����ڱ߾� */
                    }

                    QMenuBar::item {
                        background-color: #003366;
                        color: #ffffff;
                        padding: 0 10px; /* ���Ӳ˵�����ڱ߾� */
                    }

                    QMenuBar::item:selected {
                        background-color: #0055cc;
                        color: #ffffff;
                    }

                    /* �˵� */
                    QMenu {
                        background-color: #ffffff;
                        border: 1px solid #cccccc;
                        padding: 5px;
                    }

                    QMenu::item:selected {
                        background-color: #0055cc;
                        color: #ffffff;
                    }

                    /* ��ť */
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

                    /* ��ͨ�ı��� */
                    QLineEdit {
                        background-color: #ffffff;
                        border: 1px solid #cccccc;
                        border-radius: 5px;
                        padding: 5px;
                    }

                    /* �����ı��� */
                    QTextEdit {
                        background-color: #ffffff;
                        border: 1px solid #cccccc;
                        border-radius: 5px;
                        padding: 5px;
                    }

                    /* �����е�ͼ����ͼ */
                    QGraphicsView {
                        border: 1px solid #cccccc;
                        background-color: #ffffff;
                    }

                    /* ״̬�� */
                    QStatusBar {
                        color: #ffffff;
                        height: 30px; /* ����״̬���ĸ߶� */
                        padding: 0;   /* ����ڱ߾� */
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
                        height: 30px; /* ����״̬���ĸ߶� */
                        padding: 0;   /* ����ڱ߾� */
                        border: none; /* ��ֹ���½����� */
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

    // ��ȡͼ������
    double* pinp = new double[w * h * c];
    for (int i = 0; i < c; ++i) {
        GDALRasterBand* band = mDataset->GetRasterBand(i + 1);
        CPLErr err = band->RasterIO(GF_Read, 0, 0, w, h, pinp + i * w * h, w, h, GDT_Float64, 0, 0);
        if (err) {
            qDebug() << "��ȡդ������ʱ����: " << CPLGetLastErrorMsg();
        }
    }

    emit doingSegment();

    int numsuperpixels = 500; // ����������
    double compactness = 20.0; // ���ն�
    bool doRGBtoLAB = true; // �Ƿ����RGB��LAB��ת��
    int* plabels = new int[w * h]; // �洢�ָ����ı�ǩ
    int* pnumlabels = new int[1]; // �洢��ǩ����

    qDebug() << "��ʼ����ͼ��ָ�...";

    SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

    qDebug() << "ͼ��ָ����";

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

    // �����������Զ����߳�
    mProcessingThreadList.emplace_back(std::make_unique<CustomThread>(task, this));

    auto& processingThread = mProcessingThreadList.back();

    connect(processingThread.get(), &CustomThread::finished, this, [this]() {
        
    });
    processingThread->start();
}

void MainWindow::segmentImage() {
    mProgressBar->setVisible(true); // ��ʾ������
    mProgressBar->setValue(0); // ��ʼ������Ϊ0

    auto task = [this]() {
        executeSNIC();
    };

    // �����������Զ����߳�
    mProcessingThreadList.emplace_back(std::make_unique<CustomThread>(task, this));

    auto& processingThread = mProcessingThreadList.back();

    connect(processingThread.get(), &CustomThread::finished, this, [this]() {
        showPolygons();

        mProgressBar->setValue(100); // ���½�������ֵ
        // ʹ�� QTimer �ӳ��������ؽ�����
        QTimer::singleShot(2000, [this]() {
            mProgressBar->setVisible(false);
        });
    });

    connect(this, &MainWindow::doingGetDataset, this, [this]() {
        mProgressBar->setValue(10); // ���½�������ֵ
    });

    connect(this, &MainWindow::doingSegment, this, [this]() {
        mProgressBar->setValue(30); // ���½�������ֵ
    });

    connect(this, &MainWindow::doingInitResult, this, [this]() {
        mProgressBar->setValue(60); // ���½�������ֵ
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
        qDebug() << "�޷����ļ�";
        return;
    }

    // ��ȡͼ��Ļ�����Ϣ
    int nRasterXSize = mDataset->GetRasterXSize();
    int nRasterYSize = mDataset->GetRasterYSize();
    int nBands = mDataset->GetRasterCount();

    qDebug() << "ͼ����: " << nRasterXSize;
    qDebug() << "ͼ��߶�: " << nRasterYSize;
    qDebug() << "��������: " << nBands;
}

void MainWindow::showPolygons() {

    mPolygonManager.reset(new PolygonManager(mGraphicsScene, mSegResult.get()));

    mPolygonManager->generatePolygons();

    mPolygonManager->showAllPolygons();
}

