#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QPixmap>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include "classes/position.h"
#include "sceneimageviewer.h"
#include "classes/replay.h"

class Worker : public QObject
{
	Q_OBJECT

public:
	explicit Worker(SceneImageViewer* imageViewer, QObject* parent = nullptr);
	~Worker();

	void setMatchStartTime(long matchStartTime);
	bool isMatchRunning();
	bool isPlayerOnTank();
	void fetchAndDisplayMap();
	void fetchMapObjects();
	QPixmap getDrawedMapImage();
	QPixmap getOriginalMapImage() const;
	void setOriginalMapImage(const QPixmap& originalMapImage);
	void clearMarkers();
	void addPosition(const Position& position);
	void addPOI(const Position& position);
	bool havePOIBeenDrawnFunc() const;

public slots:
	void startTimer();
	void stopTimer();
	void performTask();

signals:
	void updatePixmap(const QPixmap& pixmap);
	void refreshReplays();
	void changeStackedWidget2(int index);
	void updateProgressBar(double progress);
	void updateStatusLabel(QString msg);

private:
	void onTimeout();
	void updatePOI();
	bool shouldUpdateMarkers();
	void updateMarkers();
	bool shouldEndMatch();
	void endMatch();
	void restartScheduler();
	void drawMarkers(QPixmap& displayImage);
	void drawMarkers(QPixmap& displayImage, QPainter& painter, const QList<Position>& positionCache);
	void drawSpecialMarkers(QPixmap& displayImage);
	void drawCaptureZoneMarker(QPixmap& displayImage, QPainter& painter, const Position& pos);
	static void drawRespawnBaseTank(QPixmap& displayImage, QPainter& painter, const QList<Position>& group);
	QJsonObject fetchJsonElement(QString url);
	QJsonArray fetchJsonArray(QString url);
	QPixmap fetchMapImage();
	Position getPositionFromJsonElement(QJsonObject element);
	bool shouldLoadMap();


	QPixmap originalMapImage;
	QPixmap drawedMapImage;
	QList<Position> positionCache;
	QList<Position> poi;
	static bool havePOIBeenDrawn;
	SceneImageViewer* imageViewer;
	QTimer* m_timer;
	qint64 matchStartTime;
	static QString DATA_URL;
	static QString MAP_URL;
	static QString MAP_INFO;
	static QString INDICATORS;
	QNetworkAccessManager* networkManager;

	QString currentMap;
	QString currentVehicle;
};

#endif // WORKER_H