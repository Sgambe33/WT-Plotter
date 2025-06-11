#ifndef DISCORDWORKER_H
#define DISCORDWORKER_H

#include <QObject>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <qdatetime.h>
#include "discord-files/cpp/discord.h"

class DiscordWorker : public QObject {
	Q_OBJECT

public:
	explicit DiscordWorker(QObject* parent = nullptr);
	~DiscordWorker();

public slots:
	void start();                  // Called to start loop/timer
	void stop();                   // Graceful shutdown
    void updateActivity(const QString& state, const QString& details, const QString& logo, const QDateTime& epochStartTime = QDateTime(), const QString& largeText = QString());

private:
	QTimer* updateTimer = nullptr;
	discord::Core* core{};
	discord::Activity activity{};
};
#endif // DISCORDWORKER_H
