#include "CustomGraphicsScene.h"
#include <iostream>

CustomGraphicsScene::CustomGraphicsScene(QObject* parent) : 
    QGraphicsScene(parent)
{
}

CustomGraphicsScene::~CustomGraphicsScene()
{
}

void CustomGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (!sceneRect().contains(event->scenePos())) {
        event->ignore();
        return;
    }
    QGraphicsScene::mousePressEvent(event);
    if (event->button() == Qt::RightButton) {
        emit startMerge();
    }
}
