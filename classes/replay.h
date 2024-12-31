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
    explicit Replay(const QByteArray &buffer); // Constructor to initialize from binary data

    static Replay fromFile(const QString &filePath); // Factory method to create Replay from a file

    // Getters for replay properties
    QString getLevel() const;
    double getTimePlayed() const;
    QString getStatus() const;
    QList<Player> getPlayers() const;
	QString getAuthorId() const;
    QString getSessionId() const;
	QString getBattleType() const;
	int getStartTime() const;

private:
    // Magic number for validation
    static const QByteArray MAGIC;

    // Replay metadata
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
    QList<Player> players; // List of players as JSON objects
    QList<PlayerInfo> playersInfo;

    // Helper methods
    QString readString(QDataStream &stream, int length); // Reads a fixed-length string from the binary stream
    QJsonObject unpackResults(int rezOffset, const QByteArray &buffer); // Unpacks results from binary data
    void parseResults(const QJsonObject &results); // Parses JSON results
};

#endif // REPLAY_H
