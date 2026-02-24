#ifndef SCENEIMAGEVIEWER_H
#define SCENEIMAGEVIEWER_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QWheelEvent>
#include <QHash>
#include <QVector>
#include <QColor>

#include "libs/WRPL_parser/include/ReplayStructs.h"

class SceneImageViewer : public QGraphicsView {
	Q_OBJECT

	struct ReplayPoint {
		uint32_t time = 0;
		uint64_t entityId = 0;
		QPointF scenePoint;
	};

	QGraphicsScene m_scene;
	QGraphicsPixmapItem m_item;
	QList<QGraphicsItem*> m_overlayItems;
	QVector<ReplayPoint> m_replayPoints;
	qreal m_scaleFactor = 1.0;
	const qreal m_zoomStep = 0.1;
	const qreal m_minScale = 0.1;
	const qreal m_maxScale = 10.0;

	void clearOverlay();
	static QColor colorForEntity(uint64_t entityId);

public:
	SceneImageViewer(QWidget* parent = nullptr);

	void setPixmap(const QPixmap& pixmap);
	void setReplayPackets(const std::vector<MovementPacket>& packets, double mapSize = 0.0, const QPointF& bottomLeft = QPointF());
	void renderReplayFrame(int packetIndex);
	void clearReplay();

	QSize sizeHint() const override;

protected:
	void wheelEvent(QWheelEvent* event) override;
};

#endif // SCENEIMAGEVIEWER_H
