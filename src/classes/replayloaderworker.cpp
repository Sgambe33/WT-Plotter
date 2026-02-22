#include "ReplayLoaderWorker.h"

#include <utility>

#include "logger.h"
#include "libs/WRPL_parser/include/ReplayStructs.h"
#include "libs/WRPL_parser/include/wrpl.h"

ReplayLoaderWorker::ReplayLoaderWorker(QString folderPath, QString dbFilePath, QObject *parent) : QObject(parent),
                                                                                                  m_folderPath(std::move(folderPath)),
                                                                                                  m_dbFilePath(std::move(dbFilePath)) {
}

void ReplayLoaderWorker::loadReplays() {
    DbManager localDbManager(m_dbFilePath, "replayloader");
    localDbManager.createTables();

    qint32 latestReplayEpoch = localDbManager.getLatestReplay();

    QDir dir(m_folderPath);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    dir.setNameFilters({"*.wrpl"});
    QFileInfoList fileInfoList = dir.entryInfoList();

    int total = fileInfoList.size();
    int count = 0;
    for (const QFileInfo &fileInfo: fileInfoList) {
        if (fileInfo.birthTime().toSecsSinceEpoch() <= latestReplayEpoch) {
            count++;
            emit progressUpdated(total > 0 ? static_cast<int>(100.0 * count / total) : 100);
            continue;
        }
        QString filePath = fileInfo.absoluteFilePath();
        try {
            wrpl::Replay rep = wrpl::parseClientReplay(filePath.toStdString());
            localDbManager.insertReplay(rep);
        } catch (const std::exception &e) {
            LOG_WARN("Error loading replay:" + QString(e.what()));
        }
        ++count;
        emit progressUpdated(total > 0 ? static_cast<int>(100.0 * count / total) : 100);
    }
    emit finished();
}
