#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CustomGraphicsView.h"
#include "CustomGraphicsScene.h"
#include "SegmentationResult.h"
#include "PolygonManager.h"
#include "CustomThread.h"
#include "ImageProcessor.h"
#include <QMainWindow>
#include <QPushButton>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QProgressBar>
#include <string>
#include <memory>

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
    void showOptionsDialog();

private:
    void initUI();
    void showImage();
    void clearImage();
    void showPolygons();
    void loadStyle();

private:
    CustomGraphicsView* mGraphicsView;
    CustomGraphicsScene* mGraphicsScene;
    QProgressBar* mProgressBar;
    QGraphicsPixmapItem* mImageItem;
    std::string mImageFileName;

    std::unique_ptr<ImageProcessor> mImageProcessor;
    std::unique_ptr<PolygonManager> mPolygonManager;
    std::list<std::unique_ptr<CustomThread>> mProcessingThreadList;

signals:
    void doingGetDataset();
    void doingSegment();
    void doingInitResult();
};

#endif // MAINWINDOW_H
