#include "CustomGraphicsView.h"

CustomGraphicsView::CustomGraphicsView(QWidget* parent)
    : QGraphicsView(parent),
    mScaleFactor(1.0),
    mMinScale(0.2),
    mPanning(false),
    mPanStartX(0),
    mPanStartY(0) {}

double CustomGraphicsView::getScaleFactor() const {
    return mScaleFactor;
}

void CustomGraphicsView::resetScale() {
    mScaleFactor = 1.0;
    setTransform(QTransform::fromScale(mScaleFactor, mScaleFactor));
}

void CustomGraphicsView::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0) {
        zoomIn();
    }
    else {
        zoomOut();
    }
    event->accept();
}

void CustomGraphicsView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        mPanning = true;
        mPanStartX = event->x();
        mPanStartY = event->y();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
    //else if (event->button() == Qt::LeftButton) {
    //    // 获取点击的屏幕坐标
    //    QPointF screenPos = event->pos();
    //    // 将屏幕坐标转换为场景坐标
    //    QPointF scenePos = mapToScene(screenPos.toPoint());
    //    // 发射信号，传递场景坐标
    //    emit mousePressed(scenePos);
    //    event->accept();
    //}
    else {
        QGraphicsView::mousePressEvent(event);
    }
}

void CustomGraphicsView::mouseMoveEvent(QMouseEvent* event) {
    if (mPanning) {
        int dx = event->x() - mPanStartX;
        int dy = event->y() - mPanStartY;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - dx);
        verticalScrollBar()->setValue(verticalScrollBar()->value() - dy);
        mPanStartX = event->x();
        mPanStartY = event->y();
        event->accept();
    }
    else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void CustomGraphicsView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        mPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
    else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void CustomGraphicsView::zoomIn() {
    scale(1.25, 1.25);
    mScaleFactor *= 1.25;
}

void CustomGraphicsView::zoomOut() {
    scale(0.8, 0.8);
    mScaleFactor *= 0.8;
    if (mScaleFactor < mMinScale) {
        mScaleFactor = mMinScale;
        setTransform(QTransform::fromScale(mMinScale, mMinScale));
    }
}

#include "CustomGraphicsView.moc"