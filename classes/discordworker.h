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
    void updateActivity(QString status);  // Slot to receive updates from main thread

signals:
    void finished();               // To signal the thread to exit

private:
    QTimer* updateTimer = nullptr;
    discord::Core* core{};
	discord::Activity activity{};
};
