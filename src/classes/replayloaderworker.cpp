#include "replayloaderworker.h"
#include "logger.h"

ReplayLoaderWorker::ReplayLoaderWorker(const QString& folderPath,
	const QString& dbFilePath,
	QObject* parent)
	: QObject(parent),
	m_folderPath(folderPath),
	m_dbFilePath(dbFilePath)
{
}

void ReplayLoaderWorker::loadReplays() {
	DbManager localDbManager(m_dbFilePath, "replayloader");
	localDbManager.createTables();
	localDbManager.deleteDanglingRecords();

	qint64 latestReplayEpoch = localDbManager.getLatestReplay();

	QDir dir(m_folderPath);
	dir.setFilter(QDir::Files | QDir::NoSymLinks);
	dir.setNameFilters({ "*.wrpl" });
	QFileInfoList fileInfoList = dir.entryInfoList();

	int total = fileInfoList.size();
	int count = 0;

	for (const QFileInfo& fileInfo : fileInfoList) {
		if (fileInfo.birthTime().toSecsSinceEpoch() <= latestReplayEpoch) {
			++count;
			emit progressUpdated(total > 0 ? static_cast<int>(100.0 * count / total) : 100);
			continue;
		}
		QString filePath = fileInfo.absoluteFilePath();
		try {
			Replay rep = Replay::fromFile(filePath);
			localDbManager.insertReplay(rep);
		}
		catch (const std::exception& e) {
			LOG_WARN(QString("Error loading replay: %1").arg(e.what()));
		}
		++count;
		emit progressUpdated(total > 0 ? static_cast<int>(100.0 * count / total) : 100);
	}
	emit finished();
}