#include "PolygonManager.h"

PolygonManager::PolygonManager() : mGraphicsScene(nullptr), mSegResult(nullptr)
{
}

PolygonManager::PolygonManager(CustomGraphicsScene* scene) :
    mGraphicsScene(scene),
    mSegResult(nullptr)
{
    connect(mGraphicsScene, &CustomGraphicsScene::startMerge, this, &PolygonManager::handleStartMerge);
}

PolygonManager::PolygonManager(CustomGraphicsScene* scene, SegmentationResult* segResult) :
    mGraphicsScene(scene),
    mSegResult(segResult)
{
    connect(mGraphicsScene, &CustomGraphicsScene::startMerge, this, &PolygonManager::handleStartMerge);
}

PolygonManager::~PolygonManager() {
    for (auto& pair : mOGRPolygons) {
        delete pair.second;
    }
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

    // 将 labels 转换为 OpenCV 的 Mat 对象
    cv::Mat labelsMat(height, width, CV_32SC1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            labelsMat.at<int>(y, x) = labels[y][x];
        }
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

        if (mOGRPolygons.find(label) == mOGRPolygons.end()) {
            mOGRPolygons[label] = new OGRPolygon();
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
    for (auto& pair : mOGRPolygons) {
        if (index++ >= 10) {
            break;
        }
        OGRLinearRing* ring = pair.second->getExteriorRing();
        //std::cout << pair.first << "号标签的多边形边界：" << std::endl;
        //std::cout << "共有" << ring->getNumPoints() << "个点：" << std::endl;
        for (int i = 0; i < ring->getNumPoints(); ++i) {
            OGRPoint point;
            ring->getPoint(i, &point);
            //std::cout << point.getX() << "," << point.getY() << std::endl;
        }
    }
}

void PolygonManager::showAllPolygons() {
    for (int i = 0; i < mSegResult->getLabelCount(); ++i) {
        //std::cout << "正在绘制第" << i + 1 << "个多边形..." << std::endl;

        // 获取第i个标签对应的OGRPolygon
        if (getPolygonByLabel(i) == nullptr) {
            continue;
        }
        const OGRPolygon* ogrPolygon = getPolygonByLabel(i);

        // 转换为QPolygon
        QPolygon qPolygon = convertOGRPolygonToQPolygon(ogrPolygon);

        CustomPolygonItem* polygonItem = new CustomPolygonItem(qPolygon);

        QObject::connect(polygonItem, &CustomPolygonItem::polygonSelected, this, &PolygonManager::handlePolygonSelected);

        QObject::connect(polygonItem, &CustomPolygonItem::polygonDeselected, this, &PolygonManager::handlePolygonDeselected);

        mPolygonItems[i] = polygonItem;

        mPolygonItems[i]->setLabel(i);

        mPolygonItems[i]->setPen(QPen(Qt::black, 1));

        // 绘制多边形
        mGraphicsScene->addItem(polygonItem);
    }

}

void PolygonManager::mergePolygons(std::vector<CustomPolygonItem*> polygonItems)
{
    if (mSegResult == nullptr) {
        return;
    }

    std::cout << "正在合并所选择的多边形..." << std::endl;

    // 先合并labels
    std::vector<int> mergeLabels;
    for (CustomPolygonItem* item : polygonItems) {
        int label = item->getLabel();
        mergeLabels.push_back(label);
    }
    mSegResult->mergeLabels(mergeLabels);

    // 再根据合并后的labels生成新的多边形并移除旧的多边形
    // 删除合并前的多边形
    for (int i = 0; i < polygonItems.size(); ++i) {
        int label = polygonItems[i]->getLabel();
        if (mOGRPolygons.find(label) != mOGRPolygons.end()) {
            delete mOGRPolygons[label];
            mOGRPolygons.erase(label);
            mGraphicsScene->removeItem(mPolygonItems[label]);
        }
    }

    // 确定要重新检测的多边形的范围
    CustomPolygonItem* targetItem = polygonItems[0];
    int targetLabel = targetItem->getLabel();
    const cv::Rect& newBoundingBox = mSegResult->getBoundingBoxByLabel(targetLabel);
    int startX = newBoundingBox.x;
    int startY = newBoundingBox.y;
    int endX = newBoundingBox.x + newBoundingBox.width;
    int endY = newBoundingBox.y + newBoundingBox.height;

    //std::cout << "多边形重新检测范围：" << cv::Rect(startX, startY, endX - startX, endY - startY) << std::endl;

    // 生成合并后的多边形
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

    // 若有0值将0值替换为一个非零的背景值
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

        // 若找到的轮廓与所选多边形无关则跳过
        if (std::find(mergeLabels.begin(), mergeLabels.end(), label) == mergeLabels.end()) {
            continue;
        }

        // 将非零的背景值转换回0
        if (label == labelCount + 1) {
            label = 0;
        }

        if (mOGRPolygons.find(label) == mOGRPolygons.end()) {
            mOGRPolygons[label] = new OGRPolygon();
        }

        OGRLinearRing* ring = new OGRLinearRing();
        for (const auto& point : contour) {
            ring->addPoint(point.x, point.y);
        }
        ring->closeRings();

        mOGRPolygons[label]->addRingDirectly(ring);

        QPolygon newPolygon = convertOGRPolygonToQPolygon(mOGRPolygons[label]);
        CustomPolygonItem* polygonItem = new CustomPolygonItem(newPolygon);
        mGraphicsScene->addItem(polygonItem);
    }

    std::cout << "生成合并后的多边形完成" << std::endl;

}

std::vector<OGRPoint> PolygonManager::simplifyPolygon(const std::vector<OGRPoint>& points, double tolerance)
{
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

QPolygon PolygonManager::convertOGRPolygonToQPolygon(const OGRPolygon* ogrPolygon) {
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
    mergePolygons(mSelectedPolygonItems);
    mSelectedPolygonItems.clear();
}

