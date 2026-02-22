#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QSqlQuery>
#include <QMap>
#include <QList>
#include "utils.h"
#include "libs/WRPL_parser/include/ReplayStructs.h"

class DbManager : public QObject {
    Q_OBJECT

public:
    explicit DbManager(const QString &path, const QString &connName, QObject *parent = nullptr);

    ~DbManager() override;

    void createTables() const;

    bool insertReplay(const wrpl::Replay &replay);

    quint32 getLatestReplay();

    wrpl::Replay getReplayBySessionId(const QString &sessionId) const;

    bool deleteReplayBySessionId(const QString &sessionId) const;

    QMap<QDate, QList<wrpl::Replay> > fetchReplaysGroupedByDate() const;

private:
    void prepareQueries();

    QSqlDatabase m_db;
    QSqlQuery m_insertReplayMetadataQuery;
    QSqlQuery m_insertReplayDataQuery;
};

#endif // DBMANAGER_H
