#ifndef POLYGONMANAGER_H
#define POLYGONMANAGER_H

#include "CustomGraphicsScene.h"
#include "SegmentationResult.h"
#include "CustomPolygonItem.h"
#include <vector>
#include <map>
#include <ogrsf_frmts.h>
#include <opencv2/opencv.hpp>
#include <QObject>
#include <memory>

class PolygonManager : public QObject {
    Q_OBJECT

public:
    PolygonManager();

    PolygonManager(CustomGraphicsScene* scene, SegmentationResult* segResult);

    ~PolygonManager();

    void setGraphicsScene(CustomGraphicsScene* scene);

    void setSegmentationResult(SegmentationResult* segResult);

    void generatePolygons();

    void showAllPolygons();

    void mergePolygons(std::vector<CustomPolygonItem*> polygonItems);

    int getMergedLabel(const std::vector<CustomPolygonItem*>& polygonItems);

    void removeOldPolygons(const std::vector<CustomPolygonItem*>& polygonItems);

    cv::Rect computeNewBoundingBox(CustomPolygonItem* targetItem);

    void generateNewPolygons(const cv::Rect& boundingBox, const std::vector<int>& mergeLabels);

    void setDefaultBorderColor(QColor color);

    void setHoveredBorderColor(QColor color);

    void setSelectedBorderColor(QColor color);

    inline int getPolygonCount() const {
        return mOGRPolygons.size();
    }

    inline const std::map<int, std::unique_ptr<OGRPolygon>>& getPolygons() const {
        return mOGRPolygons;
    }

    inline const OGRPolygon* getPolygonByLabel(int label) const {
        auto it = mOGRPolygons.find(label);
        return (it != mOGRPolygons.end()) ? it->second.get() : nullptr;
    }

private:

    QPolygon convertOGRPolygonToQPolygon(const OGRPolygon* ogrPolygon) const;

    void handlePolygonSelected(CustomPolygonItem* polygonItem);

    void handlePolygonDeselected(CustomPolygonItem* polygonItem);

    void handleStartMerge();

private:
    CustomGraphicsScene* mGraphicsScene;
    SegmentationResult* mSegResult;

    std::map<int, std::unique_ptr<OGRPolygon>> mOGRPolygons;
    std::map<int, std::unique_ptr<CustomPolygonItem>> mPolygonItems;
    std::vector<CustomPolygonItem*> mSelectedPolygonItems;
};

#endif // POLYGONMANAGER_H
