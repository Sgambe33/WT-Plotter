#ifndef REPLAY_H
#define REPLAY_H

#include <QString>
#include <QByteArray>
#include <QList>
#include <QJsonObject>
#include "player.h"
#include "playerinfo.h"

class Replay {
public:
	explicit Replay(const QByteArray& buffer);

	static Replay fromFile(const QString& filePath);

	QString getLevel() const;
	double getTimePlayed() const;
	QString getStatus() const;
	QList<Player> getPlayers() const;
	QList<PlayerInfo> getPlayersInfo() const;
	QString getAuthorId() const;
	QString getSessionId() const;
	QString getBattleType() const;
	int getStartTime() const;
	QString getAuthor() const;

private:
	static const QByteArray MAGIC;
	int version;
	QString level;
	QString levelSettings;
	QString battleType;
	QString environment;
	QString visibility;
	int rezOffset;
	quint8 difficulty;
	quint8 sessionType;
	QString sessionId;
	int mSetSize;
	QString locName;
	int startTime;
	int timeLimit;
	int scoreLimit;
	QString battleClass;
	QString battleKillStreak;
	QString status;
	double timePlayed;
	QString authorUserId;
	QString author;
	QList<Player> players;
	QList<PlayerInfo> playersInfo;

	QString readString(QDataStream& stream, int length);
	QJsonObject unpackResults(int rezOffset, const QByteArray& buffer);
	void parseResults(const QJsonObject& results);
};

#endif // REPLAY_H
