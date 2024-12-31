#include "sceneimageviewer.h"
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QApplication>

SceneImageViewer::SceneImageViewer(QWidget *parent)
    : QGraphicsView(parent) {
    setScene(&m_scene);
    m_scene.addItem(&m_item);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
}

void SceneImageViewer::setPixmap(const QPixmap &pixmap) { // Change to const QPixmap&
    m_item.setPixmap(pixmap);
    auto offset = -QRectF(pixmap.rect()).center(); // Convert QRect to QRectF directly
    m_item.setOffset(offset);
    setSceneRect(offset.x() * 4, offset.y() * 4, -offset.x() * 8, -offset.y() * 8);
    translate(1, 1);
    update();
}

QSize SceneImageViewer::sizeHint() const {
    return {400, 400};
}

void SceneImageViewer::wheelEvent(QWheelEvent *event) {
    QPointF scenePos = mapToScene(event->position().toPoint()); // Get mouse position in scene coordinates

    if (event->angleDelta().y() > 0) {
        if (m_scaleFactor < m_maxScale) {
            m_scaleFactor += m_zoomStep;
            scale(1.0 + m_zoomStep, 1.0 + m_zoomStep);
        }
    } else {
        if (m_scaleFactor > m_minScale) {
            m_scaleFactor -= m_zoomStep;
            scale(1.0 / (1.0 + m_zoomStep), 1.0 / (1.0 + m_zoomStep));
        }
    }

    // Adjust view to keep the zoom centered on the mouse cursor
    QPointF newScenePos = mapToScene(event->position().toPoint());
    QPointF delta = newScenePos - scenePos;
    translate(delta.x(), delta.y());

    event->accept();
}
