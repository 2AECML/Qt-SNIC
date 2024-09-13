#include "MainWindow.h"
#include "snicoptionsdialog.h"
#include <QFileDialog>
#include <QDebug>
#include <gdal_priv.h>
#include <QMenuBar>
#include <QApplication>
#include <QStatusBar>
#include <QTimer>
#include <QTextCodeC>
#include <QColorDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , mGraphicsView(nullptr)
    , mGraphicsScene(nullptr)
    , mProgressBar(nullptr)
    , mImageItem(nullptr)
    , mImageProcessor(new ImageProcessor())
    , mPolygonManager(nullptr) {

    initUI();
    loadStyle();
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
    QAction* segmentOptionAction = new QAction("Options", this);
    connect(segmentAction, &QAction::triggered, this, &MainWindow::segmentImage);
    connect(segmentOptionAction, &QAction::triggered, this, &MainWindow::showOptionsDialog);
    segmentMenu->addAction(segmentAction);
    segmentMenu->addAction(segmentOptionAction);

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

}

void MainWindow::loadImage() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.xpm *.jpg *.bmp *.tiff *.tif *.hdr *.h5 *.netcdf);;All Files (*)");
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

void MainWindow::exportResult() {
    auto task = [this]() {
        mImageProcessor->getSegmentationResult()->exportToCSV();
    };

    // �����������Զ����߳�
    mProcessingThreadList.emplace_back(std::make_unique<CustomThread>(task, this));

    auto& processingThread = mProcessingThreadList.back();

    connect(processingThread.get(), &CustomThread::finished, this, [this]() {
        
    });
    processingThread->start();
}

void MainWindow::segmentImage() {
    mImageProcessor->setImageFileName(mImageFileName);

    mProgressBar->setVisible(true); // ��ʾ������
    mProgressBar->setValue(0); // ��ʼ������Ϊ0

    auto task = [this]() {
        mImageProcessor->executeSNIC();
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

    connect(mImageProcessor.get(), &ImageProcessor::doingGetDataset, this, [this]() {
        mProgressBar->setValue(10); // ���½�������ֵ
    });

    connect(mImageProcessor.get(), &ImageProcessor::doingSegment, this, [this]() {
        mProgressBar->setValue(30); // ���½�������ֵ
    });

    connect(mImageProcessor.get(), &ImageProcessor::doingInitResult, this, [this]() {
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

void MainWindow::showOptionsDialog() {
    SNICOptionsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        SNICOptions options;
        options.superpixelNumber = dialog.getSuperpixelNumber();
        options.compactness = dialog.getCompactness();

        QMessageBox::information(this, "Options",
            QString("Superpixels: %1\nCompactness: %2")
            .arg(options.superpixelNumber)
            .arg(options.compactness));

        mImageProcessor->setSNICOptions(options);
    }
}

void MainWindow::showPolygons() {
    mPolygonManager.reset(new PolygonManager(mGraphicsScene, mImageProcessor->getSegmentationResult().get()));
    mPolygonManager->generatePolygons();
    mPolygonManager->showAllPolygons();
}

void MainWindow::loadStyle() {
    QFile file(":/main.qss"); // �� QSS �ļ�
    if (file.open(QFile::ReadOnly)) { // ����ɹ���
        std::cout << "load style sheet" << std::endl;
        QString styleSheet = file.readAll(); // ��ȡ��ʽ������
        this->setStyleSheet(styleSheet); // ������ʽ��
    }
}

