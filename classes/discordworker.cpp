#include "discordworker.h"
#include <QMetaEnum>
DiscordWorker::DiscordWorker(QObject* parent) : QObject(parent) {
	updateTimer = new QTimer(this);
	if (discord::Core::Create(1338259195455344650, DiscordCreateFlags_NoRequireDiscord, &core) != discord::Result::Ok) {
		QThread::currentThread()->quit();
		return;
	}
	if (core) {
		connect(updateTimer, &QTimer::timeout, this, [this]() mutable {
			discord::Result result = core->RunCallbacks();
			if (result != discord::Result::Ok) {
				qDebug() << "From connect: " << static_cast<int>(result);

				QThread::currentThread()->quit();
				return;
			}});
	}
}

DiscordWorker::~DiscordWorker() {
	stop();
}

void DiscordWorker::start() {
	updateTimer->start(5000);
}

void DiscordWorker::stop() {
	if (QThread::currentThread() != this->thread()) {
		QMetaObject::invokeMethod(this, "stop", Qt::QueuedConnection);
		return;
	}
	if (updateTimer->isActive()) {
		updateTimer->stop();
	}
}

void DiscordWorker::updateActivity(const QString& state, const QString& details, const QString& logo, const QDateTime& epochStartTime, const QString& largeText) {
	if (!core) return;
	QDateTime validStartTime = epochStartTime.isValid() ? epochStartTime : QDateTime::currentDateTime();
	discord::Activity activity{};
	activity.SetName("War Thunder");
	activity.GetParty().GetSize().SetCurrentSize(0);
	activity.GetParty().GetSize().SetMaxSize(0);
	activity.SetState(state.toStdString().c_str());
	activity.SetDetails(details.toStdString().c_str());
	activity.GetTimestamps().SetStart(validStartTime.toSecsSinceEpoch());
	auto& assets = activity.GetAssets();
	assets.SetLargeImage(logo.toStdString().c_str());
	assets.SetLargeText(largeText.toStdString().c_str());

	core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
		if (static_cast<int>(result) != 0) {
			qDebug() << "Discord activity updated, result:" << static_cast<int>(result);
		}
		});

	discord::Result result = core->RunCallbacks();
	if (result != discord::Result::Ok) {
		qDebug() << "From updateactivity: " << static_cast<int>(result);
		QThread::currentThread()->quit();
		return;
	};
}
