#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include "replay.h"
#include "player.h"
#include "utils.h"

class DbManager : public QObject
{
    Q_OBJECT
public:
    explicit DbManager(const QString& path, const QString connName, QObject* parent = nullptr);
    ~DbManager();

    void createTables();
    bool insertReplay(const Replay& replay);
    qint64 getLatestReplay();
    Replay getReplayBySessionId(QString sessionId);
    QMap<QDate, QList<Replay>> fetchReplaysGroupedByDate();


private:
    void prepareQueries();
    void createIndexes();


    QSqlDatabase m_db;
    QSqlQuery m_insertReplayQuery;
    QSqlQuery m_insertPlayerQuery;
    QSqlQuery m_insertPlayerCraftQuery;
    QSqlQuery m_insertPlayerDataQuery;
};

#endif // DBMANAGER_H