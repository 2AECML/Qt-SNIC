#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CustomGraphicsView.h"
#include "CustomGraphicsScene.h"
#include "SegmentationResult.h"
#include "PolygonManager.h"
#include "CustomThread.h"
#include <QMainWindow>
#include <QPushButton>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QProgressBar>
#include <string>
#include <map>
#include <memory>
#include <list>

struct GDALDatasetDeleter {
    void operator()(GDALDataset* dataset) const {
        if (dataset) {
            GDALClose(dataset);
        }
    }
};

extern "C" {
    void SNIC_main(double* pinp, int w, int h, int c, int numsuperpixels, double compactness, bool doRGBtoLAB, int* plabels, int* pnumlabels);
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void loadImage();
    void exportResult();
    void segmentImage();
    void changeDefaultBorderColor();
    void changeHoveredBorderColor();
    void changeSelectedBorderColor();

private:
    void initUI();
    void showImage();
    void clearImage();
    void executeSNIC();
    void getDataSet();
    void showPolygons();

private:
    QPushButton* mButton;
    std::string mImageFileName;
    CustomGraphicsView* mGraphicsView;
    CustomGraphicsScene* mGraphicsScene;
    QProgressBar* mProgressBar;

    std::unique_ptr<GDALDataset, GDALDatasetDeleter> mDataset;
    std::unique_ptr<SegmentationResult> mSegResult;
    QGraphicsPixmapItem* mImageItem;
    std::unique_ptr<PolygonManager> mPolygonManager;

    std::list<std::unique_ptr<CustomThread>> mProcessingThreadList;

signals:
    void doingGetDataset();
    void doingSegment();
    void doingInitResult();
};

#endif // MAINWINDOW_H
