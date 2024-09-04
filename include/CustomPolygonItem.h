#ifndef CUSTOMPOLYGONITEM_H
#define CUSTOMPOLYGONITEM_H

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
    void setSelected(bool isSelected = true);
    void setHovered(bool isHovered = true);
    bool isSelected() const;
    bool isHovered() const;
    void setDefaultColor(const QColor& color);
    void setHoveredColor(const QColor& color);
    void setSelectedColor(const QColor& color);
    QPen getDefaultPen() const;
    QPen getHoveredPen() const;
    QPen getSelectedPen() const;

signals:
    void polygonSelected(CustomPolygonItem* polygon);
    void polygonDeselected(CustomPolygonItem* polygon);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    int mLabel;
    static QPen mDefaultPen;
    static QPen mHoveredPen;
    static QPen mSelectedPen;
    bool mIsSelected = false;
    bool mIsHovered = false;
};

#endif // CUSTOMPOLYGONITEM_H