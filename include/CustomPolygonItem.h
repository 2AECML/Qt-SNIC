#pragma once

#include <QGraphicsPolygonItem>
#include <QGraphicsSceneMouseEvent>
#include <QObject>
#include <QPen>
#include <iostream>

class CustomPolygonItem : public QObject, public QGraphicsPolygonItem {
    Q_OBJECT

public:
    CustomPolygonItem(const QPolygon& polygon, QGraphicsItem* parent = nullptr);
    void setLabel(int label);
    int getLabel();

signals:
    void polygonSelected(CustomPolygonItem* polygon);
    void polygonDeselected(CustomPolygonItem* polygon);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    int mLabel;
    QPen mOriginalPen;
    bool mIsSelected = false;
};

