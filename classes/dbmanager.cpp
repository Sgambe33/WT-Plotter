#include "dbmanager.h"
#include <QSqlError>
#include <QDebug>
#include <QJsonDocument>

DbManager::DbManager(const QString& path, const QString connName, QObject* parent)
	: QObject(parent), m_db(QSqlDatabase::addDatabase("QSQLITE", connName))
{
	m_db.setDatabaseName(path);

	if (!m_db.open()) {
		qCritical() << "Database connection error:" << m_db.lastError().text();
	}
	else {
		qInfo() << "Database connected successfully";
		prepareQueries();
		createTables();
	}
}

DbManager::~DbManager()
{
	if (m_db.isOpen()) {
		m_db.close();
	}
}

void DbManager::prepareQueries()
{
	// Replay table insert
	m_insertReplayQuery = QSqlQuery(m_db);
	m_insertReplayQuery.prepare(R"(
        INSERT OR IGNORE INTO Replay 
        (session_id, author_id, start_time, map, game_mode, difficulty, status, time_played)
        VALUES 
        (:session_id, :author_id, :start_time, :map, :game_mode, :difficulty, :status, :time_played)
    )");

	// Player table insert
	m_insertPlayerQuery = QSqlQuery(m_db);
	m_insertPlayerQuery.prepare(R"(
        INSERT OR IGNORE INTO Player 
        (player_id, username, squadron_tag, squadron_id, platform)
        VALUES 
        (:player_id, :username, :squadron_tag, :squadron_id, :platform)
    )");

	// PlayerReplayData table insert
	m_insertPlayerDataQuery = QSqlQuery(m_db);
	m_insertPlayerDataQuery.prepare(R"(
        INSERT OR IGNORE INTO PlayerReplayData 
        (session_id, player_id, air_kills, ground_kills, naval_kills, team_kills,
         ai_air_kills, ai_ground_kills, ai_naval_kills, assists, deaths, captured_zones,
         damage_to_zones, score, award_damage, missile_evades, team, squad_id, auto_squad, wait_time, lineup)
        VALUES 
        (:session_id, :player_id, :air_kills, :ground_kills, :naval_kills, :team_kills,
         :ai_air_kills, :ai_ground_kills, :ai_naval_kills, :assists, :deaths, :captured_zones,
         :damage_to_zones, :score, :award_damage, :missile_evades, :team, :squad_id, :auto_squad, :wait_time, :lineup)
    )");
}

void DbManager::createTables()
{
	QSqlQuery query(m_db);
	query.exec("PRAGMA journal_mode = WAL");  // Better concurrency
	query.exec("PRAGMA synchronous = NORMAL");

	const QStringList tableDefinitions = {
		R"(
            CREATE TABLE IF NOT EXISTS Player (
                player_id INTEGER PRIMARY KEY,
                username TEXT NOT NULL,
                squadron_tag TEXT,
                squadron_id INTEGER,
                platform TEXT NOT NULL
            )
        )",
		R"(
            CREATE TABLE IF NOT EXISTS Replay (
                session_id TEXT PRIMARY KEY,
                author_id INTEGER,
                start_time INTEGER,
                map TEXT,
                game_mode TEXT ,
                difficulty TEXT,
                status TEXT,
                time_played REAL,
                FOREIGN KEY (author_id) REFERENCES Player(player_id) ON DELETE CASCADE
            )
        )",
		R"(
            CREATE TABLE IF NOT EXISTS PlayerReplayData (
                session_id TEXT NOT NULL,
                player_id INTEGER NOT NULL,
                air_kills INTEGER DEFAULT 0,
                ground_kills INTEGER DEFAULT 0,
                naval_kills INTEGER DEFAULT 0,
                team_kills INTEGER DEFAULT 0,
                ai_air_kills INTEGER DEFAULT 0,
                ai_ground_kills INTEGER DEFAULT 0,
                ai_naval_kills INTEGER DEFAULT 0,
                assists INTEGER DEFAULT 0,
                deaths INTEGER DEFAULT 0,
                captured_zones INTEGER DEFAULT 0,
                damage_to_zones INTEGER DEFAULT 0,
                score INTEGER DEFAULT 0,
                award_damage INTEGER DEFAULT 0,
                missile_evades INTEGER DEFAULT 0,
                team INTEGER DEFAULT 0,
                squad_id INTEGER DEFAULT 0,
				wait_time REAL NOT NULL DEFAULT 0,
                auto_squad INTEGER DEFAULT 0,
				lineup TEXT NOT NULL,
                PRIMARY KEY (session_id, player_id),
                FOREIGN KEY (player_id) REFERENCES Player(player_id) ON DELETE CASCADE,
                FOREIGN KEY (session_id) REFERENCES Replay(session_id) ON DELETE CASCADE
            )
        )"

	};

	for (const QString& tableSql : tableDefinitions) {
		if (!query.exec(tableSql)) {
			qCritical() << "Table creation failed:" << query.lastError().text();
		}
	}
}

bool DbManager::insertReplay(const Replay& replay)
{
	if (!m_db.transaction()) {
		qCritical() << "Transaction start failed:" << m_db.lastError().text();
		return false;
	}

	try {
		const QMap<Player, PlayerReplayData> players = replay.getPlayers();
		for (const Player& player : players.keys()) {
			m_insertPlayerQuery.bindValue(":player_id", player.getUserId().toULongLong());
			m_insertPlayerQuery.bindValue(":username", player.getUsername());
			m_insertPlayerQuery.bindValue(":squadron_tag", player.getSquadronTag());
			m_insertPlayerQuery.bindValue(":squadron_id", player.getSquadronId());
			m_insertPlayerQuery.bindValue(":platform", player.getPlatform());

			if (!m_insertPlayerQuery.exec()) {
				throw std::runtime_error("Player insert failed");
			}
		}


		m_insertReplayQuery.bindValue(":session_id", replay.getSessionId());
		m_insertReplayQuery.bindValue(":author_id", replay.getAuthorUserId().toULongLong());
		m_insertReplayQuery.bindValue(":start_time", replay.getStartTime());
		m_insertReplayQuery.bindValue(":map", replay.getLevel());
		m_insertReplayQuery.bindValue(":game_mode", replay.getBattleType());
		m_insertReplayQuery.bindValue(":difficulty", Utils::difficultyToString(replay.getDifficulty()));
		m_insertReplayQuery.bindValue(":status", replay.getStatus());
		m_insertReplayQuery.bindValue(":time_played", replay.getTimePlayed());
	
		if (!m_insertReplayQuery.exec()) {
			qCritical() << "Replay insert failed:" << m_insertReplayQuery.lastError().text();
			throw std::runtime_error("Replay insert failed");
		}

		for (const PlayerReplayData& player : players.values()) {
			m_insertPlayerDataQuery.bindValue(":session_id", replay.getSessionId());
			m_insertPlayerDataQuery.bindValue(":player_id", player.getUserId().toULongLong());
			m_insertPlayerDataQuery.bindValue(":air_kills", player.getKills());
			m_insertPlayerDataQuery.bindValue(":ground_kills", player.getGroundKills());
			m_insertPlayerDataQuery.bindValue(":naval_kills", player.getNavalKills());
			m_insertPlayerDataQuery.bindValue(":team_kills", player.getTeamKills());
			m_insertPlayerDataQuery.bindValue(":ai_air_kills", player.getAiKills());
			m_insertPlayerDataQuery.bindValue(":ai_ground_kills", player.getAiGroundKills());
			m_insertPlayerDataQuery.bindValue(":ai_naval_kills", player.getAiNavalKills());
			m_insertPlayerDataQuery.bindValue(":assists", player.getAssists());
			m_insertPlayerDataQuery.bindValue(":deaths", player.getDeaths());
			m_insertPlayerDataQuery.bindValue(":captured_zones", player.getCaptureZone());
			m_insertPlayerDataQuery.bindValue(":damage_to_zones", player.getDamageZone());
			m_insertPlayerDataQuery.bindValue(":score", player.getScore());
			m_insertPlayerDataQuery.bindValue(":award_damage", player.getAwardDamage());
			m_insertPlayerDataQuery.bindValue(":missile_evades", player.getMissileEvades());
			m_insertPlayerDataQuery.bindValue(":team", player.getTeam());
			m_insertPlayerDataQuery.bindValue(":squad_id", player.getSquad());
			m_insertPlayerDataQuery.bindValue(":auto_squad", player.getAutoSquad());
			m_insertPlayerDataQuery.bindValue(":wait_time", player.getWaitTime());
			m_insertPlayerDataQuery.bindValue(":lineup", player.getLineup());

			if (!m_insertPlayerDataQuery.exec()) {
				throw std::runtime_error("Player data insert failed");
			}
		}

		if (!m_db.commit()) {
			throw std::runtime_error("Commit failed");
		}
		return true;

	}
	catch (const std::exception& e) {
		m_db.rollback();
		qCritical() << "Database error:" << e.what();
		return false;
	}
}

QMap<QDate, QList<Replay>> DbManager::fetchReplaysGroupedByDate()
{
	QMap<QDate, QList<Replay>> replayMap;
	QSqlQuery query(m_db);

	query.prepare(R"(
        SELECT session_id, author_id, start_time, map, game_mode, 
               difficulty, status, time_played
        FROM Replay
        ORDER BY start_time DESC
    )");

	if (!query.exec()) {
		qWarning() << "Failed to fetch replays:" << query.lastError().text();
		return replayMap;
	}

	while (query.next()) {
		Replay replay;

		replay.setSessionId(query.value("session_id").toString());
		replay.setAuthorUserId(QString::number(query.value("author_id").toULongLong()));
		replay.setStartTime(query.value("start_time").toLongLong());
		replay.setLevel(query.value("map").toString());
		replay.setBattleType(query.value("game_mode").toString());
		replay.setDifficulty(Utils::stringToDifficulty(query.value("difficulty").toString()));
		replay.setStatus(query.value("status").toString());
		replay.setTimePlayed(query.value("time_played").toFloat());

		QDate dateKey = QDateTime::fromSecsSinceEpoch(replay.getStartTime()).date();

		replayMap[dateKey].append(replay);
	}

	for (auto& replays : replayMap) {
		std::sort(replays.begin(), replays.end(), [](const Replay& a, const Replay& b) {
			return a.getStartTime() < b.getStartTime();
			});
	}

	return replayMap;
}

qint64 DbManager::getLatestReplay()
{
	QSqlQuery query("SELECT start_time FROM Replay ORDER BY start_time DESC LIMIT 1", m_db);
	if (query.exec() && query.next()) {
		return query.value(0).toLongLong();
	}
	return 0;
}


Replay DbManager::getReplayBySessionId(QString sessionId)
{
	QSqlQuery query(m_db);
	query.prepare(R"(SELECT * FROM Replay WHERE session_id = :session_id)");
	query.bindValue(":session_id", sessionId);

	if (!query.exec() || !query.next()) {
		qWarning() << "Failed to fetch replay with session_id:" << sessionId << query.lastError().text();
		return Replay();
	}

	Replay replay;
	replay.setSessionId(query.value("session_id").toString());
	replay.setAuthorUserId(QString::number(query.value("author_id").toULongLong()));
	replay.setStartTime(query.value("start_time").toLongLong());
	replay.setLevel(query.value("map").toString());
	replay.setBattleType(query.value("game_mode").toString());
	replay.setDifficulty(Utils::stringToDifficulty(query.value("difficulty").toString()));
	replay.setStatus(query.value("status").toString());
	replay.setTimePlayed(query.value("time_played").toFloat());

	//Get all players data for that replay
	query.prepare(R"(SELECT * FROM PlayerReplayData JOIN Player ON PlayerReplayData.player_id=Player.player_id WHERE session_id = :session_id)");
	query.bindValue(":session_id", sessionId);

	if (!query.exec() || !query.next()) {
		qWarning() << "Failed to fetch playerReplay with session_id:" << sessionId << query.lastError().text();
		return Replay();
	}

	QMap<Player, PlayerReplayData> players;
	while (query.next()) {
		Player player;
		PlayerReplayData playerData;

		player.setUserId(query.value("player_id").toString());
		player.setUsername(query.value("username").toString());
		player.setSquadronTag(query.value("squadron_tag").toString());
		player.setSquadronId(query.value("squadron_id").toString());
		player.setPlatform(query.value("platform").toString());

		playerData.setUserId(query.value("player_id").toString());
		playerData.setKills(query.value("air_kills").toInt());
		playerData.setGroundKills(query.value("ground_kills").toInt());
		playerData.setNavalKills(query.value("naval_kills").toInt());
		playerData.setTeamKills(query.value("team_kills").toInt());
		playerData.setAiKills(query.value("ai_air_kills").toInt());
		playerData.setAiGroundKills(query.value("ai_ground_kills").toInt());
		playerData.setAiNavalKills(query.value("ai_naval_kills").toInt());
		playerData.setAssists(query.value("assists").toInt());
		playerData.setDeaths(query.value("deaths").toInt());
		playerData.setCaptureZone(query.value("captured_zones").toInt());
		playerData.setDamageZone(query.value("damage_to_zones").toInt());
		playerData.setScore(query.value("score").toInt());
		playerData.setAwardDamage(query.value("award_damage").toInt());
		playerData.setMissileEvades(query.value("missile_evades").toInt());
		playerData.setTeam(query.value("team").toInt());
		playerData.setSquad(query.value("squad_id").toInt());
		playerData.setAutoSquad(query.value("auto_squad").toBool());
		//playerData.setLineup(query.value("lineup").toStringList()); //TODO: Check if this works
		playerData.setWaitTime(query.value("wait_time").toDouble());

		players.insert(player, playerData);
		
	}
	replay.setPlayers(players);

	return replay;
}