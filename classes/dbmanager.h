#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QString>
#include <QSqlDatabase>
#include <QJsonObject>
#include "replay.h"
#include "player.h"

class DbManager
{
public:
    explicit DbManager(const QString path);
    void createTables();
    bool insertReplay(const Replay& replay);
    qint64 getLatestReplay();


private:
    QSqlDatabase m_db;
    bool insertPlayer(const PlayerInfo& player);
    bool insertPlayerCraft(const QString& sessionId, quint64 playerId, const QJsonObject& craftLineup);
    bool insertPlayerReplayData(const QString& sessionId, const Player& player);
};

#endif // DBMANAGER_H
