#include "CustomPolygonItem.h"

CustomPolygonItem::CustomPolygonItem(const QPolygon& polygon, QGraphicsItem* parent) :
    QGraphicsPolygonItem(polygon, parent) {
    mOriginalPen = QPen(Qt::black, 1);
    setAcceptHoverEvents(true);
}

void CustomPolygonItem::setLabel(int label) {
    mLabel = label;
}

int CustomPolygonItem::getLabel()
{
    return mLabel;
}

void CustomPolygonItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    QGraphicsPolygonItem::hoverEnterEvent(event);
    //qDebug() << "Polygon hovered!";
}

void CustomPolygonItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QGraphicsPolygonItem::mousePressEvent(event);
        std::cout << "Polygon clicked!" << std::endl;
        mIsSelected = !mIsSelected;

        if (mIsSelected) {
            // 发射被选中信号
            emit polygonSelected(this);
            this->setPen(QPen(Qt::blue, 1.2));
            //std::cout << "label:" << mLabel << std::endl;
            //std::cout << "包含" << this->polygon().size() << "个边角点" << std::endl;
            //for (int i = 0; i < this->polygon().size(); ++i) {
            //    std::cout << "(" << this->polygon().at(i).x() << "," << this->polygon().at(i).y() << ")" << std::endl;
            //}
        }
        else {
            // 发射被取消选中信号
            emit polygonDeselected(this);
            this->setPen(mOriginalPen);
        }

        
    }
    else if (event->button() == Qt::RightButton) {
        QGraphicsPolygonItem::mousePressEvent(event);
        std::cout << (int)event->pos().x() << " " << (int)event->pos().y() << std::endl;
        
    }
}

#include "CustomPolygonItem.moc"
