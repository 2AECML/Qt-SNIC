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

    std::cout << "��ʼ���ɷָ����Ķ����..." << std::endl;

    const std::vector<std::vector<int>>& labels = mSegResult->getLabels();
    int labelCount = mSegResult->getLabelCount();

    int width = labels[0].size();
    int height = labels.size();

    // �� labels ת��Ϊ OpenCV �� Mat ����
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
        std::cout << "��ǩ " << pair.first << " ���������: " << pair.second << std::endl;
    }

    // ��0ֵ�滻Ϊһ������ı���ֵ
    cv::Mat nonZeroLabelsMat = labelsMat.clone();
    nonZeroLabelsMat.setTo(labelCount + 1, labelsMat == 0);

    // �ҵ�����
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(nonZeroLabelsMat, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

    // ������ת��Ϊ OGRPolygon ����
    for (size_t i = 0; i < contours.size(); ++i) {
        const std::vector<cv::Point>& contour = contours[i];
        int label = nonZeroLabelsMat.at<int>(contour[0].y, contour[0].x);

        // ������ı���ֵת����0
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

    std::cout << "���ɶ�������" << std::endl;

    int index = 0;
    for (auto& pair : mPolygons) {
        if (index++ >= 10) {
            break;
        }
        OGRLinearRing* ring = pair.second->getExteriorRing();
        std::cout << pair.first << "�ű�ǩ�Ķ���α߽磺" << std::endl;
        std::cout << "����" << ring->getNumPoints() << "���㣺" << std::endl;
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
