// CustomPolygonItem.cpp

#include <QGraphicsPolygonItem>
#include <QGraphicsSceneMouseEvent>
#include <iostream>
#include <QPen>
#include <set>

class CustomPolygonItem : public QGraphicsPolygonItem {
public:
	CustomPolygonItem(const QPolygon& polygon, QGraphicsItem* parent = nullptr) :
		QGraphicsPolygonItem(polygon, parent) {
		mOriginalPen = QPen(Qt::black, 1);
		setAcceptHoverEvents(true);
	}

	void setLabel(int label) {
		mLabel = label;
	}

protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override {
		QGraphicsPolygonItem::hoverEnterEvent(event);
		//qDebug() << "Polygon hovered!";
	}

	void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
		if (event->button() == Qt::LeftButton) {
			QGraphicsPolygonItem::mousePressEvent(event);
			std::cout << "Polygon clicked!" << std::endl;
			mIsSelected = !mIsSelected;

			if (mIsSelected) {
				this->setPen(QPen(Qt::blue, 1.2));
				std::cout << "label:" << mLabel << std::endl;
				std::cout << "包含" << this->polygon().size() << "个边界点" << std::endl;
				for (int i = 0; i < this->polygon().size(); ++i) {
					std::cout << "(" << this->polygon().at(i).x() << "," << this->polygon().at(i).y() << ")" << std::endl;
				}
			}
			else {
				this->setPen(mOriginalPen);
			}
		}
		else if (event->button() == Qt::RightButton) {
			QGraphicsPolygonItem::mousePressEvent(event);
			std::cout << (int)event->pos().x() << " " << (int)event->pos().y() << std::endl;
		}
	}

	bool isAdjacent(const QPoint& p1, const QPoint& p2) {
		return true;
	}

private:
	int mLabel;
	QPen mOriginalPen;
	bool mIsSelected = false;
};