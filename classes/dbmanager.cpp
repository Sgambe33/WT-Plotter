#include "dbmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariant>
#include <QJsonDocument>

DbManager::DbManager(const QString path)
{
	m_db = QSqlDatabase::addDatabase("QSQLITE");
	m_db.setDatabaseName(path);

	if (!m_db.open())
	{
		qDebug() << "Error: connection with database failed";
	}
	else
	{
		qDebug() << "Database: connection ok";
	}
}

void DbManager::createTables()
{
	QSqlQuery query;

	QString createPlayerTable = R"(
        CREATE TABLE IF NOT EXISTS Player (
            player_id BIGINT UNSIGNED NOT NULL PRIMARY KEY,
            username TEXT NOT NULL,
            squadron_tag TEXT,
            squadron_id BIGINT,
            platform TEXT NOT NULL
        )
    )";

	QString createPlayerCraftTable = R"(
        CREATE TABLE IF NOT EXISTS PlayerCraft (
            session_id TEXT NOT NULL,
            player_id BIGINT NOT NULL,
            lineup TEXT NOT NULL, -- JSON stored as TEXT for compatibility
            PRIMARY KEY (session_id, player_id),
            FOREIGN KEY (player_id) REFERENCES Player(player_id),
            FOREIGN KEY (session_id) REFERENCES Replay(session_id)
        )
    )";

	QString createPlayerReplayDataTable = R"(
        CREATE TABLE IF NOT EXISTS PlayerReplayData (
            session_id TEXT NOT NULL,
            player_id BIGINT UNSIGNED NOT NULL,
            air_kills SMALLINT UNSIGNED DEFAULT 0,
            ground_kills SMALLINT UNSIGNED DEFAULT 0,
            naval_kills SMALLINT UNSIGNED DEFAULT 0,
            team_kills SMALLINT UNSIGNED DEFAULT 0,
            ai_air_kills SMALLINT UNSIGNED DEFAULT 0,
            ai_ground_kills SMALLINT UNSIGNED DEFAULT 0,
            ai_naval_kills SMALLINT UNSIGNED DEFAULT 0,
            assists SMALLINT UNSIGNED DEFAULT 0,
            deaths SMALLINT UNSIGNED DEFAULT 0,
            captured_zones SMALLINT UNSIGNED DEFAULT 0,
            damage_to_zones SMALLINT UNSIGNED DEFAULT 0,
            score SMALLINT UNSIGNED DEFAULT 0,
            award_damage SMALLINT UNSIGNED DEFAULT 0,
            missile_evades SMALLINT UNSIGNED DEFAULT 0,
            team TINYINT DEFAULT 0,
            squad_id TINYINT UNSIGNED DEFAULT 0,
            auto_squad TINYINT UNSIGNED DEFAULT 0,
			PRIMARY KEY (session_id, player_id),
            FOREIGN KEY (player_id) REFERENCES Player(player_id),
            FOREIGN KEY (session_id) REFERENCES Replay(session_id)
        )
    )";

	QString createReplayTable = R"(
        CREATE TABLE IF NOT EXISTS Replay (
            session_id TEXT NOT NULL PRIMARY KEY,
            author_id BIGINT UNSIGNED NOT NULL,
            start_time DATETIME NOT NULL,
            map TEXT NOT NULL,
            game_mode TEXT NOT NULL,
            status TEXT NOT NULL,
            time_played FLOAT NOT NULL,
            FOREIGN KEY (author_id) REFERENCES Player(player_id)
        )
    )";

	QStringList queries = { createPlayerTable, createPlayerCraftTable, createPlayerReplayDataTable, createReplayTable };

	for (const QString& queryStr : queries)
	{
		if (!query.exec(queryStr))
		{
			qDebug() << "Error creating table:" << query.lastError().text();
		}
		else
		{
			qDebug() << "Table created or already exists.";
		}
	}
}

bool DbManager::insertReplay(const Replay& replay)
{
	QSqlQuery query;
	m_db.transaction();

	query.prepare(R"(
        INSERT OR IGNORE INTO Replay (session_id, author_id, start_time, map, game_mode, status, time_played)
        VALUES (:session_id, :author_id, :start_time, :map, :game_mode, :status, :time_played)
    )");
	query.bindValue(":session_id", replay.getSessionId());
	query.bindValue(":author_id", replay.getAuthorId().toULongLong());
	query.bindValue(":start_time", replay.getStartTime());
	query.bindValue(":map", replay.getLevel());
	query.bindValue(":game_mode", replay.getBattleType());
	query.bindValue(":status", replay.getStatus());
	query.bindValue(":time_played", replay.getTimePlayed());

	if (!query.exec()) {
		qDebug() << "Insert Replay Error:" << query.lastError();
		m_db.rollback();
		return false;
	}

	QList<Player> players = replay.getPlayers();
	QList<PlayerInfo> playersInfo = replay.getPlayersInfo();
	for (const PlayerInfo& playerInfo : playersInfo) {
		if (!insertPlayer(playerInfo)) {
			m_db.rollback();
			return false;
		}

		QJsonObject craftLineup = playerInfo.getCraftLineup(); // Assuming getCraftLineup() provides the player's lineup
		if (!insertPlayerCraft(replay.getSessionId(), static_cast<quint64>(playerInfo.getUserId().toULongLong()), craftLineup)) {
			m_db.rollback(); // Rollback on error
			return false;
		}
	}

	for (const Player player : players) {
		if (!insertPlayerReplayData(replay.getSessionId(), player)) {
			m_db.rollback(); // Rollback on error
			return false;
		}
	}

	m_db.commit(); // Commit the transaction
	return true;
}

bool DbManager::insertPlayer(const PlayerInfo& player)
{
	QSqlQuery query;
	query.prepare(R"(
        INSERT OR IGNORE INTO Player (player_id, username, squadron_tag, squadron_id, platform)
        VALUES (:player_id, :username, :squadron_tag, :squadron_id, :platform)
    )");
	query.bindValue(":player_id", player.getUserId().toULongLong());
	query.bindValue(":username", player.getName());
	query.bindValue(":squadron_tag", player.getClanTag());
	query.bindValue(":squadron_id", player.getClanId());
	query.bindValue(":platform", player.getPlatform());

	if (!query.exec()) {
		qDebug() << "Insert Player Error:" << query.lastError();
		return false;
	}
	return true;
}

bool DbManager::insertPlayerCraft(const QString& sessionId, quint64 playerId, const QJsonObject& craftLineup)
{
	QSqlQuery query;
	query.prepare(R"(
        INSERT OR IGNORE INTO PlayerCraft (session_id, player_id, lineup)
        VALUES (:session_id, :player_id, :lineup)
    )");
	query.bindValue(":session_id", sessionId);
	query.bindValue(":player_id", playerId);
	query.bindValue(":lineup", QString(QJsonDocument(craftLineup).toJson(QJsonDocument::Compact)));

	if (!query.exec()) {
		qDebug() << "Insert PlayerCraft Error:" << query.lastError();
		return false;
	}
	return true;
}

bool DbManager::insertPlayerReplayData(const QString& sessionId, const Player& player)
{
	QSqlQuery query;
	query.prepare(R"(
        INSERT OR IGNORE INTO PlayerReplayData (
            session_id, player_id, air_kills, ground_kills, naval_kills, team_kills,
            ai_air_kills, ai_ground_kills, ai_naval_kills, assists, deaths, captured_zones,
            damage_to_zones, score, award_damage, missile_evades, team, squad_id, auto_squad
        )
        VALUES (
            :session_id, :player_id, :air_kills, :ground_kills, :naval_kills, :team_kills,
            :ai_air_kills, :ai_ground_kills, :ai_naval_kills, :assists, :deaths, :captured_zones,
            :damage_to_zones, :score, :award_damage, :missile_evades, :team, :squad_id, :auto_squad
        )
    )");
	query.bindValue(":session_id", sessionId);
	query.bindValue(":player_id", player.getUserId().toULongLong());
	query.bindValue(":air_kills", player.getKills());
	query.bindValue(":ground_kills", player.getGroundKills());
	query.bindValue(":naval_kills", player.getNavalKills());
	query.bindValue(":team_kills", player.getTeamKills());
	query.bindValue(":ai_air_kills", player.getAiKills());
	query.bindValue(":ai_ground_kills", player.getAiGroundKills());
	query.bindValue(":ai_naval_kills", player.getAiNavalKills());
	query.bindValue(":assists", player.getAssists());
	query.bindValue(":deaths", player.getDeaths());
	query.bindValue(":captured_zones", player.getCaptureZone());
	query.bindValue(":damage_to_zones", player.getDamageZone());
	query.bindValue(":score", player.getScore());
	query.bindValue(":award_damage", player.getAwardDamage());
	query.bindValue(":missile_evades", player.getMissileEvades());
	query.bindValue(":team", player.getTeam());
	query.bindValue(":squad_id", player.getSquadId());
	query.bindValue(":auto_squad", player.getAutoSquad());

	if (!query.exec()) {
		qDebug() << "Insert PlayerReplayData Error:" << query.lastError();
		return false;
	}
	return true;
}

qint64 DbManager::getLatestReplay(){
	QSqlQuery query;
	query.prepare("SELECT * FROM Replay ORDER BY start_time DESC LIMIT 1");
	if (!query.exec())
	{
		qDebug() << "Error getting latest replay:" << query.lastError();
	}
	else
	{

		while (query.next())
		{
			return query.value("start_time").toInt();
		}
	}
	return 0;
}