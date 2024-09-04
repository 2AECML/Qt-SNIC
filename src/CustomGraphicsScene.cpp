#include "CustomGraphicsScene.h"
#include <iostream>

CustomGraphicsScene::CustomGraphicsScene(QObject* parent) : 
    QGraphicsScene(parent) {
}

CustomGraphicsScene::~CustomGraphicsScene() {
}

void CustomGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (!sceneRect().contains(event->scenePos())) {
        event->ignore();
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        CustomPolygonItem* item = getBottomPolygonItemAt(event->scenePos());
        if (item) {
            item->isSelected() ? item->setSelected(false) : item->setSelected(true);
        }
    }
    else if (event->button() == Qt::RightButton) {
        emit startMerge();
    }

    QGraphicsScene::mousePressEvent(event);
}

void CustomGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (!sceneRect().contains(event->scenePos())) {
        event->ignore();
        return;
    }

    if (event->buttons() & Qt::LeftButton) {
        CustomPolygonItem* item = getBottomPolygonItemAt(event->scenePos());
        if (item && !item->isSelected()) {
            item->setSelected(true);
        }
    }
    
    QGraphicsScene::mouseMoveEvent(event);
}

CustomPolygonItem* CustomGraphicsScene::getBottomPolygonItemAt(const QPointF& pos) {
    QList<QGraphicsItem*> itemsAtPos = items(pos);
    QList<CustomPolygonItem*> polygonItems;

    for (QGraphicsItem* item : itemsAtPos) {
        CustomPolygonItem* polygonItem = dynamic_cast<CustomPolygonItem*>(item);

        if (polygonItem) {
            polygonItems.append(polygonItem);
        }
    }

    std::cout << std::endl;

    if (!polygonItems.isEmpty()) {
        // �� z ֵ��������
        std::sort(polygonItems.begin(), polygonItems.end(), [](CustomPolygonItem* a, CustomPolygonItem* b) {
            return a->zValue() < b->zValue();
            });

        //std::cout << "�ײ�����: " << polygonItems.first()->getLabel() << std::endl;
        return polygonItems.last();  // ������ײ����
    }

    return nullptr;  // ���û���ҵ� CustomPolygonItem���򷵻� nullptr
}
