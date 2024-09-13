#include "PolygonManager.h"
#include "CustomThread.h"
#include <QDebug>
#include <QPainterPath>

PolygonManager::PolygonManager() : mGraphicsScene(nullptr), mSegResult(nullptr) {
}

PolygonManager::PolygonManager(CustomGraphicsScene* scene, SegmentationResult* segResult)
    : mGraphicsScene(scene)
    , mSegResult(segResult) {
    connect(mGraphicsScene, &CustomGraphicsScene::startMerge, this, &PolygonManager::handleStartMerge);
}

PolygonManager::~PolygonManager() {
    
}

void PolygonManager::setGraphicsScene(CustomGraphicsScene* scene) {
    mGraphicsScene = scene;
}

void PolygonManager::setSegmentationResult(SegmentationResult* segResult) {
    mSegResult = segResult;
}

void PolygonManager::generatePolygons() {
    if (mSegResult == nullptr) {
        return;
    }

    OGRRegisterAll();

    std::cout << "开始生成分割结果的多边形..." << std::endl;

    const std::vector<std::vector<int>>& labels = mSegResult->getLabels();
    int labelCount = mSegResult->getLabelCount();

    int width = labels[0].size();
    int height = labels.size();

    cv::Mat labelsMat(height, width, CV_32SC1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            labelsMat.at<int>(y, x) = labels[y][x];
        }
    }

    cv::Mat nonZeroLabelsMat = labelsMat.clone();
    nonZeroLabelsMat.setTo(labelCount + 1, labelsMat == 0);

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(nonZeroLabelsMat, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); ++i) {
        const std::vector<cv::Point>& contour = contours[i];
        int label = nonZeroLabelsMat.at<int>(contour[0].y, contour[0].x);

        if (label == labelCount + 1) {
            label = 0;
        }

        if (mOGRPolygons.find(label) == mOGRPolygons.end()) {
            mOGRPolygons[label] = std::make_unique<OGRPolygon>();
        }

        OGRLinearRing* ring = new OGRLinearRing();
        for (const auto& point : contour) {
            ring->addPoint(point.x, point.y);
        }
        ring->closeRings();

        mOGRPolygons[label]->addRingDirectly(ring);
    }

    std::cout << "生成多边形完成" << std::endl;

    int index = 0;
    for (const auto& pair : mOGRPolygons) {
        if (index++ >= 10) {
            break;
        }
        OGRLinearRing* ring = pair.second->getExteriorRing();
        for (int i = 0; i < ring->getNumPoints(); ++i) {
            OGRPoint point;
            ring->getPoint(i, &point);
        }
    }
}

void PolygonManager::showAllPolygons() {
    for (int i = 0; i < mSegResult->getLabelCount(); ++i) {
        if (getPolygonByLabel(i) == nullptr) {
            continue;
        }
        const OGRPolygon* ogrPolygon = getPolygonByLabel(i);
        QPolygon qPolygon = convertOGRPolygonToQPolygon(ogrPolygon);
        auto polygonItem = std::make_unique<CustomPolygonItem>(qPolygon);

        connect(polygonItem.get(), &CustomPolygonItem::polygonSelected, this, &PolygonManager::handlePolygonSelected);
        connect(polygonItem.get(), &CustomPolygonItem::polygonDeselected, this, &PolygonManager::handlePolygonDeselected);

        mPolygonItems[i] = std::move(polygonItem);
        mPolygonItems[i]->setLabel(i);
        mGraphicsScene->addItem(mPolygonItems[i].get());
    }
}

void PolygonManager::mergePolygons(std::vector<CustomPolygonItem*> polygonItems) {
    if (mSegResult == nullptr || polygonItems.size() < 2) {
        return;
    }

    std::cout << "正在合并所选择的多边形..." << std::endl;

    int mergedLabel = getMergedLabel(polygonItems);
    removeOldPolygons(polygonItems);
    cv::Rect newBoundingBox = mSegResult->getBoundingBoxByLabel(mergedLabel);
    generateNewPolygons(newBoundingBox, std::vector<int>{mergedLabel});

    std::cout << "生成合并后的多边形完成" << std::endl;
}

int PolygonManager::getMergedLabel(const std::vector<CustomPolygonItem*>& polygonItems) {
    std::vector<int> mergeLabels;
    for (CustomPolygonItem* item : polygonItems) {
        int label = item->getLabel();
        mergeLabels.push_back(label);
    }
    int mergedLabel = mSegResult->mergeLabels(mergeLabels);
    return mergedLabel;
}

void PolygonManager::removeOldPolygons(const std::vector<CustomPolygonItem*>& polygonItems) {
    for (CustomPolygonItem* item : polygonItems) {
        int label = item->getLabel();
        if (mOGRPolygons.find(label) != mOGRPolygons.end()) {
            mOGRPolygons.erase(label);
            mGraphicsScene->removeItem(mPolygonItems[label].get());
            mPolygonItems.erase(label);
        }
    }
}

cv::Rect PolygonManager::computeNewBoundingBox(CustomPolygonItem* targetItem) {
    int targetLabel = targetItem->getLabel();
    const cv::Rect& newBoundingBox = mSegResult->getBoundingBoxByLabel(targetLabel);
    return newBoundingBox;
}

void PolygonManager::generateNewPolygons(const cv::Rect& boundingBox, const std::vector<int>& mergeLabels) {
    int startX = boundingBox.x;
    int startY = boundingBox.y;
    int endX = boundingBox.x + boundingBox.width;
    int endY = boundingBox.y + boundingBox.height;

    const std::vector<std::vector<int>>& labels = mSegResult->getLabels();
    int labelCount = mSegResult->getLabelCount();
    int width = labels[0].size();
    int height = labels.size();

    cv::Mat labelsMat(height, width, CV_32SC1);
    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            labelsMat.at<int>(y, x) = labels[y][x];
        }
    }

    cv::Mat nonZeroLabelsMat = labelsMat.clone();
    nonZeroLabelsMat.setTo(labelCount + 1, labelsMat == 0);

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(nonZeroLabelsMat, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); ++i) {
        const std::vector<cv::Point>& contour = contours[i];
        int label = nonZeroLabelsMat.at<int>(contour[0].y, contour[0].x);

        if (label == labelCount + 1) {
            label = 0;
        }

        if (std::find(mergeLabels.begin(), mergeLabels.end(), label) == mergeLabels.end()) {
            continue;
        }

        if (mOGRPolygons.find(label) == mOGRPolygons.end()) {
            mOGRPolygons[label] = std::make_unique<OGRPolygon>();
        }

        OGRLinearRing* ring = new OGRLinearRing();
        for (const auto& point : contour) {
            ring->addPoint(point.x, point.y);
        }
        ring->closeRings();

        mOGRPolygons[label]->addRingDirectly(ring);

        QPolygon newPolygon = convertOGRPolygonToQPolygon(mOGRPolygons[label].get());
        auto polygonItem = std::make_unique<CustomPolygonItem>(newPolygon);
        connect(polygonItem.get(), &CustomPolygonItem::polygonSelected, this, &PolygonManager::handlePolygonSelected);
        connect(polygonItem.get(), &CustomPolygonItem::polygonDeselected, this, &PolygonManager::handlePolygonDeselected);

        mPolygonItems[label] = std::move(polygonItem);
        mPolygonItems[label]->setLabel(label);
        mGraphicsScene->addItem(mPolygonItems[label].get());

        break;  // 确保只生成一次多边形
    }
}

void PolygonManager::setDefaultBorderColor(QColor color) {
    for (auto& pair : mPolygonItems) {
        pair.second->setDefaultColor(color);
    }
}

void PolygonManager::setHoveredBorderColor(QColor color) {
    for (auto& pair : mPolygonItems) {
        pair.second->setHoveredColor(color);
    }
}

void PolygonManager::setSelectedBorderColor(QColor color) {
    for (auto& pair : mPolygonItems) {
        pair.second->setSelectedColor(color);
    }
}

QPolygon PolygonManager::convertOGRPolygonToQPolygon(const OGRPolygon* ogrPolygon) const {
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

void PolygonManager::handlePolygonSelected(CustomPolygonItem* polygonItem) {
    std::cout << "多边形" << polygonItem->getLabel() << "被选中" << std::endl;
    mSelectedPolygonItems.push_back(polygonItem);
    std::cout << "目前总共有" << mSelectedPolygonItems.size() << "个多边形被选中:";
    for (auto& selectedItem : mSelectedPolygonItems) {
        std::cout << selectedItem->getLabel() << ", ";
    }
    std::cout << std::endl;
}

void PolygonManager::handlePolygonDeselected(CustomPolygonItem* polygonItem) {
    std::cout << "多边形 " << polygonItem->getLabel() << "被取消选中" << std::endl;
    mSelectedPolygonItems.erase(std::remove(mSelectedPolygonItems.begin(), mSelectedPolygonItems.end(), polygonItem));
    std::cout << "目前总共有" << mSelectedPolygonItems.size() << "个多边形被选中:";
    for (auto& selectedItem : mSelectedPolygonItems) {
        std::cout << selectedItem->getLabel() << ", ";
    }
    std::cout << std::endl;
}

void PolygonManager::handleStartMerge() {
    if (mSelectedPolygonItems.empty() || mSelectedPolygonItems.size() < 2) {
        return;
    }
    mergePolygons(mSelectedPolygonItems);
    mSelectedPolygonItems.clear();
}
