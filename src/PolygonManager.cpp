#include "PolygonManager.h"

PolygonManager::PolygonManager() : mSegResult(nullptr)
{
}

PolygonManager::PolygonManager(SegmentationResult* segResult) :
    mSegResult(segResult)
{
}

PolygonManager::~PolygonManager()
{
}

void PolygonManager::setSegmentationResult(SegmentationResult* segResult)
{
    mSegResult = segResult;
}


void PolygonManager::generatePolygons()
{
    if (mSegResult == nullptr) {
        return;
    }

    OGRRegisterAll();

    std::cout << "开始生成分割结果的多边形..." << std::endl;

    const std::vector<std::vector<int>>& labels = mSegResult->getLabels();
    int labelCount = mSegResult->getLabelCount();

    int width = labels[0].size();
    int height = labels.size();

    // 将 labels 转换为 OpenCV 的 Mat 对象
    cv::Mat labelsMat(height, width, CV_32SC1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            labelsMat.at<int>(y, x) = labels[y][x];

            int label = labels[y][x];
            if (mBoundingBoxes.find(label) == mBoundingBoxes.end()) {
                mBoundingBoxes[label] = cv::Rect(x, y, 1, 1);
            }
            else {
                cv::Rect& rect = mBoundingBoxes[label];
                rect.x = std::min(rect.x, x);
                rect.y = std::min(rect.y, y);
                rect.width = std::max(rect.width, x - rect.x + 1);
                rect.height = std::max(rect.height, y - rect.y + 1);
            }
        }
    }

    for (const auto& pair : mBoundingBoxes) {
        std::cout << "标签 " << pair.first << " 的外包矩形: " << pair.second << std::endl;
    }

    // 将0值替换为一个非零的背景值
    cv::Mat nonZeroLabelsMat = labelsMat.clone();
    nonZeroLabelsMat.setTo(labelCount + 1, labelsMat == 0);

    // 找到轮廓
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(nonZeroLabelsMat, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

    // 将轮廓转换为 OGRPolygon 对象
    for (size_t i = 0; i < contours.size(); ++i) {
        const std::vector<cv::Point>& contour = contours[i];
        int label = nonZeroLabelsMat.at<int>(contour[0].y, contour[0].x);

        // 将非零的背景值转换回0
        if (label == labelCount + 1) {
            label = 0;
        }

        if (mPolygons.find(label) == mPolygons.end()) {
            mPolygons[label] = new OGRPolygon();
        }

        OGRLinearRing* ring = new OGRLinearRing();
        for (const auto& point : contour) {
            ring->addPoint(point.x, point.y);
        }
        ring->closeRings();

        mPolygons[label]->addRingDirectly(ring);
    }

    std::cout << "生成多边形完成" << std::endl;

    int index = 0;
    for (auto& pair : mPolygons) {
        if (index++ >= 10) {
            break;
        }
        OGRLinearRing* ring = pair.second->getExteriorRing();
        std::cout << pair.first << "号标签的多边形边界：" << std::endl;
        std::cout << "共有" << ring->getNumPoints() << "个点：" << std::endl;
        for (int i = 0; i < ring->getNumPoints(); ++i) {
            OGRPoint point;
            ring->getPoint(i, &point);
            std::cout << point.getX() << "," << point.getY() << std::endl;
        }
    }
}

void PolygonManager::mergePolygons(std::vector<CustomPolygonItem>& polygonItems)
{
}
