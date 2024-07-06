// CustomPolygonItem.cpp

#include <QGraphicsPolygonItem>
#include <QGraphicsSceneMouseEvent>
#include <iostream>
#include <QPen>

class CustomPolygonItem : public QGraphicsPolygonItem {
public:
	CustomPolygonItem(const QPolygon& polygon, QGraphicsItem* parent = nullptr) :
		QGraphicsPolygonItem(polygon, parent) {
		mOriginalPen = QPen(Qt::black, 0.5);
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
				this->setPen(QPen(Qt::blue, 0.1));
				std::cout << "label:" << mLabel << std::endl;
				std::cout << "����" << this->polygon().size() << "���߽��" << std::endl;
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

private:
	int mLabel;
	QPen mOriginalPen;
	bool mIsSelected = false;
};