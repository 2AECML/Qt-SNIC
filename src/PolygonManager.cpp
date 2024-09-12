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
    for (auto& pair : mOGRPolygons) {
        delete pair.second;
    }

    for (auto& pair : mPolygonItems) {
        std::cout << pair.second->getLabel() << std::endl;
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

        connect(polygonItem, &CustomPolygonItem::polygonSelected, this, &PolygonManager::handlePolygonSelected);

        connect(polygonItem, &CustomPolygonItem::polygonDeselected, this, &PolygonManager::handlePolygonDeselected);

        mPolygonItems[i] = polygonItem;

        mPolygonItems[i]->setLabel(i);

        // ���ƶ����
        mGraphicsScene->addItem(mPolygonItems[i]);
    }
}

void PolygonManager::mergePolygons(std::vector<CustomPolygonItem*> polygonItems) {
    if (mSegResult == nullptr || polygonItems.size() < 2) {
        return;
    }

    std::cout << "���ںϲ���ѡ��Ķ����..." << std::endl;

    // �ϲ���ǩ
    std::vector<int> mergeLabels = getMergeLabels(polygonItems);

    // ɾ���ɵĶ����
    removeOldPolygons(polygonItems);

    // �����µļ�ⷶΧ
    cv::Rect newBoundingBox = computeNewBoundingBox(polygonItems[0]);

    // �����µĶ����
    generateNewPolygons(newBoundingBox, mergeLabels);

    std::cout << "���ɺϲ���Ķ�������" << std::endl;
}

std::vector<int> PolygonManager::getMergeLabels(const std::vector<CustomPolygonItem*>& polygonItems) {
    std::vector<int> mergeLabels;
    for (CustomPolygonItem* item : polygonItems) {
        int label = item->getLabel();
        mergeLabels.push_back(label);
    }
    mSegResult->mergeLabels(mergeLabels);
    return mergeLabels;
}

void PolygonManager::removeOldPolygons(const std::vector<CustomPolygonItem*>& polygonItems) {
    for (CustomPolygonItem* item : polygonItems) {
        int label = item->getLabel();
        if (mOGRPolygons.find(label) != mOGRPolygons.end()) {
            delete mOGRPolygons[label];
            mOGRPolygons.erase(label);
            mGraphicsScene->removeItem(mPolygonItems[label]);
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
        QObject::connect(polygonItem, &CustomPolygonItem::polygonSelected, this, &PolygonManager::handlePolygonSelected);
        QObject::connect(polygonItem, &CustomPolygonItem::polygonDeselected, this, &PolygonManager::handlePolygonDeselected);
        mPolygonItems[label] = polygonItem;
        mPolygonItems[label]->setLabel(label);
        mGraphicsScene->addItem(polygonItem);

        break;  // ȷ��ֻ����һ�ζ����
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
    //qDebug() << polygonItem->polygon();
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
    if (mSelectedPolygonItems.empty() || mSelectedPolygonItems.size() < 2) {
        return;
    }
    mergePolygons(mSelectedPolygonItems);
    mSelectedPolygonItems.clear();
}