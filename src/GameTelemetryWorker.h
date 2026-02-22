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
#include "ui/sceneimageviewer.h"

class GameTelemetryWorker : public QObject
{
	Q_OBJECT

public:
	explicit GameTelemetryWorker(SceneImageViewer* imageViewer, QObject* parent = nullptr);
	~GameTelemetryWorker();

	bool isMatchRunning();
	bool isPlayerOnTank();
	void fetchAndDisplayMap();
	QPixmap getOriginalMapImage() const;
	void setOriginalMapImage(const QPixmap& originalMapImage);

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
	void restartScheduler();
	QJsonObject fetchJsonElement(QString url);
	QJsonArray fetchJsonArray(QString url);
	QImage fetchMapImage();
	bool shouldLoadMap();
	void setActivityFromWorker(const QString& state, const QString& details, const QString& logo, time_t epochStartTime = -1, const QString& largeText = QString());


    QPixmap m_originalMapImage;
    QPixmap m_drawedMapImage;
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
