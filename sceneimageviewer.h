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
    qreal m_scaleFactor = 1.0;
    const qreal m_zoomStep = 0.1;
    const qreal m_minScale = 0.1;
    const qreal m_maxScale = 10.0;

public:
    SceneImageViewer(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pixmap);

    QSize sizeHint() const override;

protected:
    void wheelEvent(QWheelEvent *event) override;
};

#endif // SCENEIMAGEVIEWER_H
