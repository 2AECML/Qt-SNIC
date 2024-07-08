
#include <vector>
#include <map>
#include <ogrsf_frmts.h>
#include <opencv2/opencv.hpp>
#include <QObject>
#include "CustomGraphicsScene.h"
#include "SegmentationResult.h"
#include "CustomPolygonItem.h"

class PolygonManager : public QObject{
    Q_OBJECT

public:
	PolygonManager();

    PolygonManager(CustomGraphicsScene* scene);

    PolygonManager(CustomGraphicsScene* scene, SegmentationResult* segResult);

	~PolygonManager();

    void setGraphicsScene(CustomGraphicsScene* scene);

    void setSegmentationResult(SegmentationResult* segResult);

	void generatePolygons();

    void showAllPolygons();

    void mergePolygons(std::vector<CustomPolygonItem*> polygonItems);

	inline int getPolygonCount() const {
		return mOGRPolygons.size();
	}

	inline const std::map<int, OGRPolygon*>& getPolygons() {
		return mOGRPolygons;
	}

	inline const OGRPolygon* getPolygonByLabel(int label) {
		return mOGRPolygons[label];
	}

private:
    // 计算两点之间的距离
    inline double distance(const OGRPoint& p1, const OGRPoint& p2) {
        return std::sqrt(std::pow(p2.getX() - p1.getX(), 2) + std::pow(p2.getY() - p1.getY(), 2));
    }

    // 临近点简化算法
    std::vector<OGRPoint> simplifyPolygon(const std::vector<OGRPoint>& points, double tolerance);

    QPolygon convertOGRPolygonToQPolygon(const OGRPolygon* ogrPolygon);

    void handlePolygonSelected(CustomPolygonItem* polygonItem);

    void handlePolygonDeselected(CustomPolygonItem* polygonItem);

    void handleStartMerge();
	
private:
    CustomGraphicsScene* mGraphicsScene;
    SegmentationResult* mSegResult;
	std::map<int, OGRPolygon*> mOGRPolygons;
    std::map<int, CustomPolygonItem*> mPolygonItems;
    std::vector<CustomPolygonItem*> mSelectedPolygonItems;
};