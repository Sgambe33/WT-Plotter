#include "sceneimageviewer.h"
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QApplication>
#include <QPen>
#include <QBrush>
#include <QtMath>
#include <algorithm>

SceneImageViewer::SceneImageViewer(QWidget* parent)
	: QGraphicsView(parent) {
	setScene(&m_scene);
	m_scene.addItem(&m_item);
	setDragMode(QGraphicsView::ScrollHandDrag);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setResizeAnchor(QGraphicsView::AnchorViewCenter);
}

void SceneImageViewer::setPixmap(const QPixmap& pixmap) {
	m_item.setPixmap(pixmap);
	const auto offset = -QRectF(pixmap.rect()).center();
	m_item.setOffset(offset);
	setSceneRect(offset.x() * 4, offset.y() * 4, -offset.x() * 8, -offset.y() * 8);
	translate(1, 1);
	update();
}

void SceneImageViewer::setReplayPackets(const std::vector<MovementPacket>& packets, const double mapSize, const QPointF& bottomLeft) {
	m_replayPoints.clear();
	clearOverlay();

	const QPixmap mapPixmap = m_item.pixmap();
	if (mapPixmap.isNull() || packets.empty()) {
		return;
	}

	const double width = mapPixmap.width();
	const double height = mapPixmap.height();
	const double xOffset = -width / 2.0;
	const double yOffset = -height / 2.0;
	const bool hasMapCoordinates = mapSize > 0.0;

	double minX = 0.0;
	double xRange = 1.0;
	double minZ = 0.0;
	double zRange = 1.0;

	if (hasMapCoordinates) {
		minX = bottomLeft.x();
		minZ = bottomLeft.y();
		xRange = mapSize;
		zRange = mapSize;
	}
	else {
		minX = packets.front().x;
		double maxX = packets.front().x;
		minZ = packets.front().z;
		double maxZ = packets.front().z;

		for (const auto& packet : packets) {
			minX = std::min(minX, packet.x);
			maxX = std::max(maxX, packet.x);
			minZ = std::min(minZ, packet.z);
			maxZ = std::max(maxZ, packet.z);
		}

		xRange = qFuzzyIsNull(maxX - minX) ? 1.0 : (maxX - minX);
		zRange = qFuzzyIsNull(maxZ - minZ) ? 1.0 : (maxZ - minZ);
	}

	m_replayPoints.reserve(static_cast<int>(packets.size()));
	for (const auto& packet : packets) {
		const double normalizedX = (packet.x - minX) / xRange;
		const double normalizedZ = (packet.z - minZ) / zRange;

		ReplayPoint replayPoint;
		replayPoint.time = packet.time;
		replayPoint.entityId = packet.entityId;
		replayPoint.scenePoint = QPointF(
			xOffset + normalizedX * width,
			yOffset + (1.0 - normalizedZ) * height
		);
		m_replayPoints.push_back(replayPoint);
	}
}

void SceneImageViewer::renderReplayFrame(int packetIndex) {
	clearOverlay();

	if (m_replayPoints.empty() || packetIndex < 0) {
		return;
	}

	const int clampedIndex = std::min(packetIndex, static_cast<int>(m_replayPoints.size() - 1));
	QHash<qulonglong, QPointF> lastPositionByEntity;

	for (int i = 0; i <= clampedIndex; ++i) {
		const ReplayPoint& point = m_replayPoints[i];
		const qulonglong entityId = static_cast<qulonglong>(point.entityId);
		if (lastPositionByEntity.contains(entityId)) {
			const QPen trailPen(colorForEntity(point.entityId), 1.6);
			auto* segment = m_scene.addLine(QLineF(lastPositionByEntity.value(entityId), point.scenePoint), trailPen);
			segment->setZValue(2);
			m_overlayItems.append(segment);
		}
		lastPositionByEntity.insert(entityId, point.scenePoint);
	}

	for (auto it = lastPositionByEntity.cbegin(); it != lastPositionByEntity.cend(); ++it) {
		const QColor entityColor = colorForEntity(static_cast<uint64_t>(it.key()));
		const QPen markerPen(Qt::black, 0.8);
		const QBrush markerBrush(entityColor);
		auto* marker = m_scene.addEllipse(it.value().x() - 3.5, it.value().y() - 3.5, 7.0, 7.0, markerPen, markerBrush);
		marker->setZValue(3);
		m_overlayItems.append(marker);
	}
}

void SceneImageViewer::clearReplay() {
	m_replayPoints.clear();
	clearOverlay();
}

QSize SceneImageViewer::sizeHint() const {
	return { 400, 400 };
}

void SceneImageViewer::wheelEvent(QWheelEvent* event) {
	const QPointF scenePos = mapToScene(event->position().toPoint());

	if (event->angleDelta().y() > 0) {
		if (m_scaleFactor < m_maxScale) {
			m_scaleFactor += m_zoomStep;
			scale(1.0 + m_zoomStep, 1.0 + m_zoomStep);
		}
	}
	else {
		if (m_scaleFactor > m_minScale) {
			m_scaleFactor -= m_zoomStep;
			scale(1.0 / (1.0 + m_zoomStep), 1.0 / (1.0 + m_zoomStep));
		}
	}

	const QPointF newScenePos = mapToScene(event->position().toPoint());
	const QPointF delta = newScenePos - scenePos;
	translate(delta.x(), delta.y());

	event->accept();
}

void SceneImageViewer::clearOverlay() {
	for (QGraphicsItem* item : m_overlayItems) {
		m_scene.removeItem(item);
		delete item;
	}
	m_overlayItems.clear();
}

QColor SceneImageViewer::colorForEntity(uint64_t entityId) {
	const int hue = static_cast<int>(entityId % 360ULL);
	return QColor::fromHsv(hue, 220, 245, 220);
}
