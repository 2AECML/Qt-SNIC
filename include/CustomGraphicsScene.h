#pragma once

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
};