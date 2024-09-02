#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CustomGraphicsView.h"
#include "CustomGraphicsScene.h"
#include "SegmentationResult.h"
#include "PolygonManager.h"
#include <QMainWindow>
#include <QPushButton>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <string>
#include <map>
#include <memory>

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

private:
    void initUI();
    void showImage();
    void executeSNIC();
    void exportResult();
    void getDataSet();
    void showPolygons();
    QPolygon convertOGRPolygonToQPolygon(const OGRPolygon* ogrPolygon);

private:
    QPushButton* mButton;
    std::string mImageFileName;
    CustomGraphicsView* mGraphicsView;
    CustomGraphicsScene* mGraphicsScene;
    std::unique_ptr<GDALDataset, GDALDatasetDeleter> mDataset;
    std::unique_ptr<SegmentationResult> mSegResult;
    std::unique_ptr<PolygonManager> mPolygonManager;
    std::map<int, CustomPolygonItem*> mPolygonItemMap;
};

#endif // MAINWINDOW_H
