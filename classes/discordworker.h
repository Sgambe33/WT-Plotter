// discordworker.h
#pragma once

#include <QObject>
#include <QTimer>
#include "discord-files/cpp/discord.h"

class DiscordWorker : public QObject {
    Q_OBJECT

public:
    explicit DiscordWorker(QObject* parent = nullptr);
    ~DiscordWorker();

public slots:
    void start();                  // Called to start loop/timer
    void stop();                   // Graceful shutdown
    void updateActivity(const QString& state, const QString& details, const QString& logo, time_t epochStartTime = -1, const QString& largeText = QString());

signals:
    void finished();               // To signal the thread to exit

private:
    QTimer* updateTimer = nullptr;
    discord::Core* core{};
	discord::Activity activity{};
};
