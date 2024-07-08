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
        }
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

    std::cout << "���ɶ�������" << std::endl;

    int index = 0;
    for (auto& pair : mOGRPolygons) {
        if (index++ >= 10) {
            break;
        }
        OGRLinearRing* ring = pair.second->getExteriorRing();
        //std::cout << pair.first << "�ű�ǩ�Ķ���α߽磺" << std::endl;
        //std::cout << "����" << ring->getNumPoints() << "���㣺" << std::endl;
        for (int i = 0; i < ring->getNumPoints(); ++i) {
            OGRPoint point;
            ring->getPoint(i, &point);
            //std::cout << point.getX() << "," << point.getY() << std::endl;
        }
    }
}

void PolygonManager::showAllPolygons() {
    for (int i = 0; i < mSegResult->getLabelCount(); ++i) {
        //std::cout << "���ڻ��Ƶ�" << i + 1 << "�������..." << std::endl;

        // ��ȡ��i����ǩ��Ӧ��OGRPolygon
        if (getPolygonByLabel(i) == nullptr) {
            continue;
        }
        const OGRPolygon* ogrPolygon = getPolygonByLabel(i);

        // ת��ΪQPolygon
        QPolygon qPolygon = convertOGRPolygonToQPolygon(ogrPolygon);

        CustomPolygonItem* polygonItem = new CustomPolygonItem(qPolygon);

        QObject::connect(polygonItem, &CustomPolygonItem::polygonSelected, this, &PolygonManager::handlePolygonSelected);

        QObject::connect(polygonItem, &CustomPolygonItem::polygonDeselected, this, &PolygonManager::handlePolygonDeselected);

        mPolygonItems[i] = polygonItem;

        mPolygonItems[i]->setLabel(i);

        mPolygonItems[i]->setPen(QPen(Qt::black, 1));

        // ���ƶ����
        mGraphicsScene->addItem(polygonItem);
    }

}

void PolygonManager::mergePolygons(std::vector<CustomPolygonItem*> polygonItems)
{
    if (mSegResult == nullptr) {
        return;
    }

    std::cout << "���ںϲ���ѡ��Ķ����..." << std::endl;

    // �Ⱥϲ�labels
    std::vector<int> mergeLabels;
    for (CustomPolygonItem* item : polygonItems) {
        int label = item->getLabel();
        mergeLabels.push_back(label);
    }
    mSegResult->mergeLabels(mergeLabels);

    // �ٸ��ݺϲ����labels�����µĶ���β��Ƴ��ɵĶ����
    // ɾ���ϲ�ǰ�Ķ����
    for (int i = 0; i < polygonItems.size(); ++i) {
        int label = polygonItems[i]->getLabel();
        if (mOGRPolygons.find(label) != mOGRPolygons.end()) {
            delete mOGRPolygons[label];
            mOGRPolygons.erase(label);
            mGraphicsScene->removeItem(mPolygonItems[label]);
        }
    }

    // ȷ��Ҫ���¼��Ķ���εķ�Χ
    CustomPolygonItem* targetItem = polygonItems[0];
    int targetLabel = targetItem->getLabel();
    const cv::Rect& newBoundingBox = mSegResult->getBoundingBoxByLabel(targetLabel);
    int startX = newBoundingBox.x;
    int startY = newBoundingBox.y;
    int endX = newBoundingBox.x + newBoundingBox.width;
    int endY = newBoundingBox.y + newBoundingBox.height;

    //std::cout << "��������¼�ⷶΧ��" << cv::Rect(startX, startY, endX - startX, endY - startY) << std::endl;

    // ���ɺϲ���Ķ����
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

    // ����0ֵ��0ֵ�滻Ϊһ������ı���ֵ
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

        // ���ҵ�����������ѡ������޹�������
        if (std::find(mergeLabels.begin(), mergeLabels.end(), label) == mergeLabels.end()) {
            continue;
        }

        // ������ı���ֵת����0
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

    std::cout << "���ɺϲ���Ķ�������" << std::endl;

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

    // ȷ�����һ����͵�һ����֮��ľ���Ҳ�������̶�
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
    std::cout << "�����" << polygonItem->getLabel() << "��ѡ��" << std::endl;
    mSelectedPolygonItems.push_back(polygonItem);
    std::cout << "Ŀǰ�ܹ���" << mSelectedPolygonItems.size() << "������α�ѡ��:";
    for (auto& selectedItem : mSelectedPolygonItems) {
        std::cout << selectedItem->getLabel() << ", ";
    }
    std::cout << std::endl;
}

void PolygonManager::handlePolygonDeselected(CustomPolygonItem* polygonItem) {
    std::cout << "����� " << polygonItem->getLabel() << "��ȡ��ѡ��" << std::endl;
    mSelectedPolygonItems.erase(std::remove(mSelectedPolygonItems.begin(), mSelectedPolygonItems.end(), polygonItem));
    std::cout << "Ŀǰ�ܹ���" << mSelectedPolygonItems.size() << "������α�ѡ��:";
    for (auto& selectedItem : mSelectedPolygonItems) {
        std::cout << selectedItem->getLabel() << ", ";
    }
    std::cout << std::endl;
}

void PolygonManager::handleStartMerge() {
    mergePolygons(mSelectedPolygonItems);
    mSelectedPolygonItems.clear();
}

