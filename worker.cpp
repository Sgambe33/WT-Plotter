#include "worker.h"
#include "classes/utils.h"
#include <QDateTime>
#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImageWriter>
#include <QThread>
#include <QFile>
#include <QFileInfoList>
#include <QDir>
#include <QSettings>
#include "classes/replay.h"
#include <chrono>
#include <iostream>
#include <QMessageBox>

bool Worker::havePOIBeenDrawn = false;
QString Worker::DATA_URL = "http://localhost:8111/map_obj.json";
QString Worker::MAP_URL = "http://localhost:8111/map.img";
QString Worker::MAP_INFO = "http://localhost:8111/map_info.json";
QString Worker::INDICATORS = "http://localhost:8111/indicators";

Worker::Worker(SceneImageViewer* imageViewer, QObject* parent)
	: QObject(parent),
	m_timer(new QTimer(this)),
	matchStartTime(0),
	networkManager(new QNetworkAccessManager(this))
{
	connect(m_timer, &QTimer::timeout, this, &Worker::onTimeout);
}

Worker::~Worker()
{
	stopTimer();
}

void Worker::startTimer()
{
	if (QThread::currentThread() != this->thread()) {
		QMetaObject::invokeMethod(this, "startTimer", Qt::QueuedConnection);
		return;
	}

	m_timer->start(1000);
}

void Worker::stopTimer()
{
	if (QThread::currentThread() != this->thread()) {
		QMetaObject::invokeMethod(this, "stopTimer", Qt::QueuedConnection);
		return;
	}
	if (m_timer->isActive()) {
		m_timer->stop();
		qDebug() << "Timer stopped.";
	}
}

void Worker::performTask()
{
	startTimer();
}

void Worker::onTimeout()
{
	try
	{
		if (shouldLoadMap())
		{
			fetchAndDisplayMap();
			emit changeStackedWidget2(2);
			emit updateStatusLabel(QString("Map loaded..."));
		}
		else if (shouldUpdateMarkers())
		{
			if (matchStartTime == 0)
			{
				matchStartTime = QDateTime::currentMSecsSinceEpoch();
				updatePOI();
				emit updateStatusLabel(QString("Match started..."));
			}
			updateMarkers();
		}
		else if (shouldEndMatch())
		{
			matchStartTime = 0;

			emit updateStatusLabel(QString("Match ended..."));

			QPixmap drawedMapImage = getOriginalMapImage();
			drawSpecialMarkers(drawedMapImage);
			drawMarkers(drawedMapImage);
			this->drawedMapImage = drawedMapImage;

			emit changeStackedWidget2(1);

			emit updatePixmap(drawedMapImage);

			endMatch();
			restartScheduler();
		}
		else {
			emit updateStatusLabel(tr("Awaiting match start..."));
		}
	}
	catch (const std::exception& e)
	{
		qDebug() << "An error occurred during scheduled task execution:" << e.what();
	}
}

void Worker::updatePOI() {
	try {
		QJsonArray objectArray = fetchJsonArray(DATA_URL);
		for (const QJsonValue& value : objectArray) {
			QJsonObject element = value.toObject();
			if (element.contains("x") && element.contains("y")
				&& element["x"].toDouble() > 0
				&& element["x"].toDouble() < 1
				&& element["y"].toDouble() > 0
				&& element["y"].toDouble() < 1) {
				Position position = getPositionFromJsonElement(element);
				if (!position.isValid()) continue;

				if (position.isCaptureZone() || position.isRespawnBaseTank()) {
					addPOI(position);
				}
			}
		}
	}
	catch (const std::exception& e) {
		qDebug() << "Exception while fetching map objects:" << e.what();
		throw std::runtime_error(e.what());
	}
}

bool Worker::shouldLoadMap()
{
	return getOriginalMapImage().isNull() && isMatchRunning();
}

bool Worker::shouldUpdateMarkers()
{
	return !getOriginalMapImage().isNull() && isMatchRunning();
}

void Worker::updateMarkers()
{
	//TODO: Check if player is spectating
	if (isPlayerOnTank())
	{
		fetchMapObjects();
	}
}

bool Worker::shouldEndMatch()
{
	return !getOriginalMapImage().isNull() && !isMatchRunning();
}

void Worker::endMatch()
{
	QString currEpoch = QString::number(QDateTime::currentMSecsSinceEpoch());
	try {
		QSettings settings("sgambe33", "wtplotter");
		QString replayDir = settings.value("replayFolderPath", "").toString();
		QString plotDir = settings.value("plotSavePath", "").toString();
		bool autosave = settings.value("autosave", false).toBool();
		QFile latestReplay;
		int retries = 60;

		if (autosave) {
			Utils::saveImage(this->drawedMapImage);
		}

		do {
			latestReplay.setFileName(Utils::getLatestReplay(QDir(replayDir)).fileName());
			if (!latestReplay.exists()) {
				QThread::sleep(1);
			}
		} while (!latestReplay.exists() && retries-- > 0);

		if (latestReplay.exists()) {
			qInfo() << "Latest replay file:" << latestReplay.fileName();
			Replay replayData = Replay::fromFile(latestReplay.fileName());
			QString uploader = replayData.getAuthorUserId();
			if (!uploader.isEmpty()) {
				Utils::uploadReplay(replayData, uploader, this->positionCache, this->poi);
			}
			else {
				qCritical() << "Failed to get user UID. Data will not be validated against replay.";
			}
			emit refreshReplays();
		}
		else {
			qWarning() << "No replay file found after match end. Data will not be validated against replay.";
		}

		qInfo() << "Position cache exported and plot saved to disk with timestamp:" << currEpoch;
	}
	catch (const std::exception& e) {
		throw std::runtime_error(e.what());
	}

	clearMarkers();
	setOriginalMapImage(QPixmap());
	this->drawedMapImage = QPixmap();
	qInfo() << "Match ended, markers cleared.";
}

void Worker::restartScheduler()
{
	stopTimer();
	startTimer();
}

void Worker::setMatchStartTime(long matchStartTime)
{
	this->matchStartTime = matchStartTime;
}

bool Worker::isMatchRunning()
{
	try {
		QJsonObject mapInfo = fetchJsonElement(MAP_INFO);
		if (!mapInfo.isEmpty()) {
			return mapInfo.contains("valid") && mapInfo["valid"].toBool();
		}
		return false;
	}
	catch (const std::exception& e) {
		qCritical() << "Exception while fetching map info:" << e.what();
		return false;
	}
}

bool Worker::isPlayerOnTank()
{
	try {
		QJsonObject response = fetchJsonElement(INDICATORS);
		if (!response.isEmpty()) {
			bool result = response.contains("valid") && response["valid"].toBool();
			result = result && (response.contains("army") && response["army"].toString().compare("tank", Qt::CaseInsensitive) == 0);
			return result;
		}
		return false;
	}
	catch (const std::exception& e) {
		qCritical() << "Exception while fetching indicators:" << e.what();
		return false;
	}
}

void Worker::fetchAndDisplayMap()
{
	if (!isMatchRunning())
		return;

	QPixmap mapImage = fetchMapImage();
	if (!mapImage.isNull()) {
		setOriginalMapImage(mapImage);
	}
}

void Worker::fetchMapObjects()
{
	try {
		QJsonArray objectArray = fetchJsonArray(DATA_URL);
		for (const QJsonValue& value : objectArray) {
			QJsonObject element = value.toObject();
			if (element.contains("x") && element.contains("y")
				&& element["x"].toDouble() > 0
				&& element["x"].toDouble() < 1
				&& element["y"].toDouble() > 0
				&& element["y"].toDouble() < 1) {
				Position position = getPositionFromJsonElement(element);
				if (!position.isValid()) continue;

				if (position.isCaptureZone() || position.isRespawnBaseTank()) {
					continue;
				}
				else {
					addPosition(position);
				}
			}
		}
	}
	catch (const std::exception& e) {
		qDebug() << "Exception while fetching map objects:" << e.what();
		throw std::runtime_error(e.what());
	}
}

QPixmap Worker::getDrawedMapImage()
{
	return drawedMapImage;
}

QPixmap Worker::getOriginalMapImage() const
{
	return originalMapImage;
}

void Worker::setOriginalMapImage(const QPixmap& originalMapImage)
{
	this->originalMapImage = originalMapImage;
}

void Worker::clearMarkers()
{
	positionCache.clear();
	poi.clear();
	havePOIBeenDrawn = false;
}

void Worker::addPosition(const Position& position)
{
	positionCache.append(position);
}

void Worker::addPOI(const Position& position)
{
	if (!havePOIBeenDrawn) {
		poi.append(position);
	}
}

bool Worker::havePOIBeenDrawnFunc() const
{
	return havePOIBeenDrawn;
}

void Worker::drawMarkers(QPixmap& displayImage)
{
	if (displayImage.isNull()) {
		qCritical() << "Error: displayImage is null.";
		return;
	}

	QPainter painter(&displayImage);
	painter.setPen(Qt::NoPen);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

	drawMarkers(displayImage, painter, positionCache);
}

void Worker::drawMarkers(QPixmap& displayImage, QPainter& painter, const QList<Position>& positionCache)
{
	for (const Position& pos : positionCache) {
		double x = pos.x();
		double y = pos.y();
		QString color = pos.color();
		QString type = pos.type();
		QColor markerColor(color);
		painter.setBrush(markerColor);

		if (type != "aircraft" && type != "airfield" && type != "respawn_base_tank" && type != "respawn_base_ship" && type != "respawn_base_aircraft" && type != "capture_zone") {
			int markerSize = 2;
			int px = static_cast<int>(x * displayImage.width());
			int py = static_cast<int>(y * displayImage.height());
			painter.drawRect(px - markerSize / 2, py - markerSize / 2, markerSize, markerSize);
		}
	}
}

void Worker::drawSpecialMarkers(QPixmap& displayImage)
{
	if (displayImage.isNull()) {
		qCritical() << "Error: displayImage is null.";
		return;
	}

	QPainter painter(&displayImage);
	painter.setPen(Qt::NoPen);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	QMap<QString, QList<Position>> respawnBaseTankGroups;

	for (const Position& pos : poi) {
		if (pos.type() == "capture_zone") {
			drawCaptureZoneMarker(displayImage, painter, pos);
		}
		else if (pos.type() == "respawn_base_tank") {
			respawnBaseTankGroups[pos.color()].append(pos);
		}
	}

	if (!poi.isEmpty() && !respawnBaseTankGroups.isEmpty()) {
		havePOIBeenDrawn = true;
	}

	for (const QList<Position>& group : respawnBaseTankGroups) {
		if (group.size() >= 5) {
			drawRespawnBaseTank(displayImage, painter, group);
		}
	}
}

void Worker::drawCaptureZoneMarker(QPixmap& displayImage, QPainter& painter, const Position& pos)
{
	double x = pos.x();
	double y = pos.y();
	int px = static_cast<int>(x * displayImage.width());
	int py = static_cast<int>(y * displayImage.height());
	painter.setPen(Qt::yellow);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(px - 10, py - 10, 20, 20);
}

void Worker::drawRespawnBaseTank(QPixmap& displayImage, QPainter& painter, const QList<Position>& group)
{
	for (const Position& pos : group) {
		QColor markerColor("#ff00ff");
		painter.setPen(markerColor);
		painter.setBrush(Qt::NoBrush);
		int markerSize = 4;
		int px1 = static_cast<int>(pos.x() * displayImage.width());
		int py1 = static_cast<int>(pos.y() * displayImage.height());
		painter.drawRect(px1 - markerSize / 2, py1 - markerSize / 2, markerSize, markerSize);
	}
}

QJsonObject Worker::fetchJsonElement(QString url)
{
	QNetworkReply* reply = networkManager->get(QNetworkRequest(QUrl(url)));
	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();

	QJsonObject jsonObject;
	if (reply->error() == QNetworkReply::NoError) {
		QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
		jsonObject = doc.object();
	}
	else {
		qCritical() << "Network error:" << reply->errorString();
	}
	reply->deleteLater();
	return jsonObject;
}

QJsonArray Worker::fetchJsonArray(QString url)
{
	QNetworkReply* reply = networkManager->get(QNetworkRequest(QUrl(url)));
	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();

	if (reply->error() != QNetworkReply::NoError) {
		qCritical() << "Network error:" << reply->errorString();
		throw std::runtime_error(reply->errorString().toStdString());
	}

	QByteArray responseData = reply->readAll();
	QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
	if (!jsonDoc.isArray()) {
		qCritical() << "Error: JSON response is not an array.";
		throw std::runtime_error("JSON response is not an array.");
	}

	return jsonDoc.array();
}

QPixmap Worker::fetchMapImage()
{
	QNetworkReply* reply = networkManager->get(QNetworkRequest(QUrl(MAP_URL)));
	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();

	QPixmap pixmap;
	if (reply->error() == QNetworkReply::NoError) {
		pixmap.loadFromData(reply->readAll());
	}
	else {
		qCritical() << "Network error:" << reply->errorString();
	}
	reply->deleteLater();
	return pixmap;
}

Position Worker::getPositionFromJsonElement(QJsonObject element)
{
	double x = element["x"].toDouble();
	double y = element["y"].toDouble();
	QString color = element["color"].toString();
	QString type = element["type"].toString();
	QString icon = element["icon"].toString();
	qint64 timeSinceBeginning = (QDateTime::currentMSecsSinceEpoch() - this->matchStartTime) / 1000;

	return Position(x, y, color, type, icon, timeSinceBeginning);
}