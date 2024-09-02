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
                        background-color: #003366;
                        color: #ffffff;
                        height: 30px; /* ����״̬���ĸ߶� */
                        padding: 0;   /* ����ڱ߾� */
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

    // ��ȡͼ������
    double* pinp = new double[w * h * c];
    for (int i = 0; i < c; ++i) {
        GDALRasterBand* band = mDataset->GetRasterBand(i + 1);
        CPLErr err = band->RasterIO(GF_Read, 0, 0, w, h, pinp + i * w * h, w, h, GDT_Float64, 0, 0);
        if (err) {
            qDebug() << "��ȡդ������ʱ����: " << CPLGetLastErrorMsg();
        }
    }

    int numsuperpixels = 500; // ����������
    double compactness = 20.0; // ���ն�
    bool doRGBtoLAB = true; // �Ƿ����RGB��LAB��ת��
    int* plabels = new int[w * h]; // �洢�ָ����ı�ǩ
    int* pnumlabels = new int[1]; // �洢��ǩ����

    qDebug() << "��ʼ����ͼ��ָ�...";

    SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

    qDebug() << "ͼ��ָ����";

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
