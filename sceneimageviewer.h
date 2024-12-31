#ifndef SCENEIMAGEVIEWER_H
#define SCENEIMAGEVIEWER_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>

class SceneImageViewer : public QGraphicsView {
    Q_OBJECT

    QGraphicsScene m_scene;
    QGraphicsPixmapItem m_item;
    qreal m_scaleFactor = 1.0; // Track the current scale factor
    const qreal m_zoomStep = 0.1; // The zoom increment for each wheel step
    const qreal m_minScale = 0.1; // Minimum zoom level
    const qreal m_maxScale = 10.0; // Maximum zoom level

public:
    SceneImageViewer(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pixmap); // Change to const QPixmap&

    QSize sizeHint() const override;

protected:
    void wheelEvent(QWheelEvent *event) override;
};

#endif // SCENEIMAGEVIEWER_H
