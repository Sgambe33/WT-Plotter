#ifndef REPLAY_H
#define REPLAY_H

#include "constants.cpp"
#include "player.h"
#include "playerreplaydata.h"
#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QPair>

class Replay {
public:
	explicit Replay(const QByteArray& buffer);
	Replay();
	~Replay();

	static Replay fromFile(const QString& filePath);

	int getVersion() const;
	QString getSessionId() const;
	QString getLevel() const;
	QString getBattleType() const;
	Constants::Difficulty getDifficulty() const;
	int getStartTime() const;
	QString getStatus() const;
	double getTimePlayed() const;
	QString getAuthorUserId() const;
	QList<QPair<Player, PlayerReplayData>> getPlayers() const;

	void setSessionId(QString sessionId);
	void setAuthorUserId(QString authorUserId);
	void setStartTime(int startTime);
	void setLevel(QString level);
	void setBattleType(QString battleType);
	void setDifficulty(Constants::Difficulty difficulty);
	void setStatus(QString status);
	void setTimePlayed(double timePlayed);
	void setPlayers(QList<QPair<Player, PlayerReplayData>> players);

private:
	static const QByteArray MAGIC;
	int m_version;
	QString m_sessionId;
	QString m_level;
	QString m_levelSettings;
	QString m_battleType;
	QString m_environment;
	QString m_visibility;
	int m_rezOffset;
	Constants::Difficulty m_difficulty;
	quint8 m_sessionType;
	int m_mSetSize;
	QString m_locName;
	int m_startTime;
	int m_timeLimit;
	int m_scoreLimit;
	QString m_battleClass;
	QString m_battleKillStreak;
	QString m_status;
	double m_timePlayed;
	QString m_authorUserId;
	QString m_author;
	QList<QPair<Player, PlayerReplayData>> m_players;

	QString readString(QDataStream& stream, int length);
	QJsonObject unpackResults(int rezOffset, const QByteArray& buffer);
	void parseResults(const QJsonObject& results);
	void processMissingData(QList<PlayerReplayData>& playerReplayDataList, QJsonArray playersArray, QList<Player>& playerList, QJsonObject& playersInfoObject);
};

#endif // REPLAY_H
