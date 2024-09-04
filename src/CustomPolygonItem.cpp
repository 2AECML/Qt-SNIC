#include "CustomPolygonItem.h"
#include <QGraphicsScene>

QPen CustomPolygonItem::mDefaultPen = QPen(QColor("#000000"), 1);
QPen CustomPolygonItem::mHoveredPen = QPen(QColor("#F0F8FF"), 1.5);
QPen CustomPolygonItem::mSelectedPen = QPen(QColor("#4169E1"), 1.8);

CustomPolygonItem::CustomPolygonItem(const QPolygon& polygon, QGraphicsItem* parent) 
    : mLabel(0)
    , QGraphicsPolygonItem(polygon, parent) {

    setAcceptHoverEvents(true);

    setPen(mDefaultPen);
}

void CustomPolygonItem::setLabel(int label) {
    mLabel = label;
}

int CustomPolygonItem::getLabel() {
    return mLabel;
}

void CustomPolygonItem::setSelected(bool isSelected) {
    emit isSelected ? polygonSelected(this) : polygonDeselected(this);
    mIsSelected = isSelected;
    this->setPen(isSelected ? mSelectedPen : mDefaultPen);
}

void CustomPolygonItem::setHovered(bool isHovered) {
    mIsHovered = isHovered;

    if (isHovered) {
        setPen(mHoveredPen);
    }
    else if (mIsSelected) {
        setPen(mSelectedPen);
    }
    else {
        setPen(mDefaultPen);
    }
}

bool CustomPolygonItem::isSelected() const {
    return mIsSelected;
}

bool CustomPolygonItem::isHovered() const {
    return mIsHovered;
}

void CustomPolygonItem::setDefaultColor(const QColor& color) {
    mDefaultPen.setColor(color);
    if (!mIsSelected && !mIsHovered) {
        setPen(mDefaultPen);
    }
}

void CustomPolygonItem::setHoveredColor(const QColor& color) {
    mHoveredPen.setColor(color);
    if (mIsHovered) {
        setPen(mHoveredPen);
    }
}

void CustomPolygonItem::setSelectedColor(const QColor& color) {
    mSelectedPen.setColor(color);
    if (mIsSelected) {
        setPen(mSelectedPen);
    }
}

QPen CustomPolygonItem::getDefaultPen() const {
    return mDefaultPen;
}

QPen CustomPolygonItem::getHoveredPen() const {
    return mHoveredPen;
}

QPen CustomPolygonItem::getSelectedPen() const {
    return mSelectedPen;
}

void CustomPolygonItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    if (!mIsSelected) {
        setHovered(true);
    }
    QGraphicsPolygonItem::hoverEnterEvent(event);
}

void CustomPolygonItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    if (!mIsSelected) {
        setHovered(false);
    }
    QGraphicsPolygonItem::hoverLeaveEvent(event);
}

