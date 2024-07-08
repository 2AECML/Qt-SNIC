#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCursor>
#include <QScrollBar>

class CustomGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    CustomGraphicsView(QWidget* parent = nullptr);
    double getScaleFactor() const;
    void resetScale();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

signals:
    void mousePressed(const QPointF& pos);

private:
    void zoomIn();
    void zoomOut();
    double mScaleFactor;
    double mMinScale;
    bool mPanning;
    int mPanStartX;
    int mPanStartY;
};
