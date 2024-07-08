
#include <vector>
#include <map>
#include <ogrsf_frmts.h>
#include <opencv2/opencv.hpp>
#include "SegmentationResult.cpp"
#include "CustomPolygonItem.h"

class PolygonManager {
public:
	PolygonManager();

    PolygonManager(SegmentationResult* segResult);

	~PolygonManager();

    void setSegmentationResult(SegmentationResult* segResult);

	void generatePolygons();

    void mergePolygons(std::vector<CustomPolygonItem>& polygonItems);

	inline int getPolygonCount() const {
		return mPolygons.size();
	}

	inline const std::map<int, OGRPolygon*>& getPolygons() {
		return mPolygons;
	}

	inline const OGRPolygon* getPolygonByLabel(int label) {
		return mPolygons[label];
	}

private:
    // 计算两点之间的距离
    double distance(const OGRPoint& p1, const OGRPoint& p2) {
        return std::sqrt(std::pow(p2.getX() - p1.getX(), 2) + std::pow(p2.getY() - p1.getY(), 2));
    }

    // 临近点简化算法
    std::vector<OGRPoint> simplifyPolygon(const std::vector<OGRPoint>& points, double tolerance) {
        std::vector<OGRPoint> simplifiedPoints;
        if (points.empty()) {
            return simplifiedPoints;
        }

        simplifiedPoints.push_back(points[0]);
        for (size_t i = 1; i < points.size(); ++i) {
            if (distance(simplifiedPoints.back(), points[i]) > tolerance) {
                simplifiedPoints.push_back(points[i]);
            }
        }

        // 确保最后一个点和第一个点之间的距离也大于容忍度
        if (distance(simplifiedPoints.back(), simplifiedPoints.front()) <= tolerance) {
            simplifiedPoints.pop_back();
        }

        return simplifiedPoints;
    }
	
private:
    SegmentationResult* mSegResult;
	std::map<int, OGRPolygon*> mPolygons;
	std::map<int, cv::Rect> mBoundingBoxes;
};