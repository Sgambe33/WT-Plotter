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
#include "ui/sceneimageviewer.h"

class Worker : public QObject
{
	Q_OBJECT

public:
	explicit Worker(SceneImageViewer* imageViewer, QObject* parent = nullptr);
	~Worker();

	bool isMatchRunning();
	bool isPlayerOnTank();
	void fetchAndDisplayMap();
	void fetchMapObjects();
	QPixmap getOriginalMapImage() const;
	void setOriginalMapImage(const QPixmap& originalMapImage);
	void clearMarkers();
	void addPosition(const Position& position);
	void addPOI(const Position& position);

public slots:
	void startTimer();
	void stopTimer();
	void performTask();

signals:
	void sendActivityToDiscord(const QString& state, const QString& details, const QString& logo, time_t epochStartTime= -1, const QString& largeText = QString());
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
	QImage fetchMapImage();
	Position getPositionFromJsonElement(QJsonObject element);
	bool shouldLoadMap();
	void setActivityFromWorker(const QString& state, const QString& details, const QString& logo, time_t epochStartTime = -1, const QString& largeText = QString());


    QPixmap m_originalMapImage;
    QPixmap m_drawedMapImage;
    QList<Position> m_positionCache;
    QList<Position> m_poi;
	static bool havePOIBeenDrawn;
	SceneImageViewer* imageViewer;
	QTimer* m_timer;
    qint64 m_matchStartTime;
	static QString DATA_URL;
	static QString MAP_URL;
	static QString MAP_INFO;
	static QString INDICATORS;
	static QString STATE;
	QNetworkAccessManager* networkManager;

	QString currentMap;
	QString currentVehicle;
	QElapsedTimer activityTimer;
	bool showAltActivity = false;
	time_t matchStartEpoch = 0;
};

#endif // WORKER_H
