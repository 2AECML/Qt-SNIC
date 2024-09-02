#ifndef CUSTOMGRAPHICSSCENE_H
#define CUSTOMGRAPHICSSCENE_H

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

#endif // CUSTOMGRAPHICSSCENE_H