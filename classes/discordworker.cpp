// discordworker.cpp
#include "discordworker.h"
#include <QDebug>

DiscordWorker::DiscordWorker(QObject* parent) : QObject(parent) {
	updateTimer = new QTimer(this);
	discord::Core::Create(53908232506183680, DiscordCreateFlags_Default, &core);
	if (core) {
		core->SetLogHook(discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
			qDebug() << "Discord SDK:" << message;
			});
	}
	activity.SetName("WT Plotter");
	activity.SetDetails("Analyzing War Thunder replays");

	connect(updateTimer, &QTimer::timeout, this, [this]() {
		core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
			if (result == discord::Result::Ok) {
				qDebug() << "Activity updated successfully!";
			}
			else {
				qDebug() << "Failed to update activity:";

			}
			});
		core->RunCallbacks();
		});
}

DiscordWorker::~DiscordWorker() {}

void DiscordWorker::start() {
	updateTimer->start(5000); // every 5 seconds or adjust as needed
}

void DiscordWorker::stop() {
	updateTimer->stop();
	emit finished();
}

void DiscordWorker::updateActivity(QString status) {
	// Send new activity to Discord SDK
	qDebug() << "New status from another thread:" << status;
	// discord_core->ActivityManager()->UpdateActivity(...);
}
