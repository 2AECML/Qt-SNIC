#ifndef CUSTOMGRAPHICSSCENE_H
#define CUSTOMGRAPHICSSCENE_H

#include "CustomPolygonItem.h"
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

class CustomGraphicsScene : public QGraphicsScene {
	Q_OBJECT

public:
	CustomGraphicsScene(QObject* parent = nullptr);
	~CustomGraphicsScene();

signals:
	void startMerge();

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

private:
	CustomPolygonItem* getBottomPolygonItemAt(const QPointF& pos);
};

#endif // CUSTOMGRAPHICSSCENE_H