// discordworker.cpp
#include "discordworker.h"
#include <QMetaEnum>
#define DISCORDPP_IMPLEMENTATION

DiscordWorker::DiscordWorker(QObject* parent) : QObject(parent) {
	updateTimer = new QTimer(this);
	//client = std::make_shared<discordpp::Client>();
//
	//connect(updateTimer, &QTimer::timeout, this, [this]() mutable {
	//	discordpp::RunCallbacks();
	//});
}

DiscordWorker::~DiscordWorker() {}

void DiscordWorker::start() {
	updateTimer->start(5000);
}

void DiscordWorker::stop() {
	updateTimer->stop();
	emit finished();
}

void DiscordWorker::updateActivity(const QString& state, const QString& details, const QString& logo, time_t epochStartTime, const QString& largeText) {
	//if (!core) return;
	//if (epochStartTime == -1) {
	//	epochStartTime = time(nullptr);
	//}
	//discordpp::Activity activity{};
    //activity.SetName("WT Plotter");
	//activity.GetParty().GetSize().SetCurrentSize(0);
	//activity.GetParty().GetSize().SetMaxSize(0);
	//activity.SetState(state.toStdString().c_str());
	//activity.SetDetails(details.toStdString().c_str());
	//activity.GetTimestamps().SetStart(epochStartTime);
	//auto& assets = activity.GetAssets();
	//assets.SetLargeImage(logo.toStdString().c_str());
	//assets.SetLargeText(largeText.toStdString().c_str());
//
	//core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
	//	if (static_cast<int>(result) != 0) {
	//		qDebug() << "Discord activity updated, result:" << static_cast<int>(result);
	//	}
	//	});
//
    //discord::Result result = core->RunCallbacks();
    //if( result != discord::Result::Ok){
    //    qDebug() << "From updateactivity: " << static_cast<int>(result);
    //    QThread::currentThread()->quit();
    //    return;
    //};
}
