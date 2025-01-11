#include <QByteArray>
#include <QBuffer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QDebug>
#include <QCoreApplication>
#include "replay.h"


const QByteArray Replay::MAGIC = QByteArray::fromHex("e5ac0010");

Replay::Replay(const QByteArray &buffer) {
    QDataStream stream(buffer);
    stream.setByteOrder(QDataStream::LittleEndian);

    QByteArray magic(4, 0);
    stream.readRawData(magic.data(), 4);
    if (magic != MAGIC) {
        throw std::runtime_error("Invalid magic number");
    }

    stream >> version;
    level = readString(stream, 128).replace("levels/", "").replace(".bin", "");
    levelSettings = readString(stream, 260);
    battleType = readString(stream, 128);
    environment = readString(stream, 128);
    visibility = readString(stream, 32);
    stream >> rezOffset;
    stream >> difficulty;
    stream.skipRawData(35); // Skip 35 bytes
    stream >> sessionType;
    stream.skipRawData(7); // Skip 7 bytes

    quint64 sessionIdInt;
    stream >> sessionIdInt;
    sessionId = QString::number(sessionIdInt, 16);

    stream.skipRawData(4); // Skip 4 bytes
    stream >> mSetSize;
    stream.skipRawData(32); // Skip 32 bytes
    locName = readString(stream, 128);
    stream >> startTime >> timeLimit >> scoreLimit;
    stream.skipRawData(48); // Skip 48 bytes
    battleClass = readString(stream, 128);
    battleKillStreak = readString(stream, 128);

    try {
        auto results = unpackResults(rezOffset, buffer);
        parseResults(results);
    } catch (const std::exception &e) {
        qWarning() << "Error unpacking results:" << e.what();
    }
}

Replay Replay::fromFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Failed to open file");
    }

    QByteArray fileContent = file.readAll();
    return Replay(fileContent);
}

QString Replay::readString(QDataStream &stream, int length) {
    QByteArray bytes(length, 0);
    stream.readRawData(bytes.data(), length);

    int nullIndex = bytes.indexOf('\0');
    if (nullIndex != -1) {
        bytes.truncate(nullIndex);
    }

    return QString::fromUtf8(bytes);
}

QJsonObject Replay::unpackResults(int rezOffset, const QByteArray &buffer) {
    QByteArray dataAfterRez = buffer.mid(rezOffset);

    QProcess process;
    QString executablePath = QCoreApplication::applicationDirPath() + QString("/wt_ext_cli");
    QStringList arguments{"--unpack_raw_blk", "--stdout", "--stdin", "--format", "Json"};

    process.start(executablePath, arguments);
    if (!process.waitForStarted()) {
        throw std::runtime_error("Failed to start process");
    }

    process.write(dataAfterRez);
    process.closeWriteChannel();

    if (!process.waitForFinished()) {
        throw std::runtime_error("Process did not finish");
    }

    QByteArray output = process.readAllStandardOutput();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(output);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        throw std::runtime_error("Invalid JSON output");
    }

    return jsonDoc.object();
}

void Replay::parseResults(const QJsonObject &results) {
    status = results.value("status").toString("Left the game");
    timePlayed = results.value("timePlayed").toDouble();
    authorUserId = results.value("authorUserId").toString();
    author = results.value("author").toString();

    QJsonArray playersArray = results.value("player").toArray();
    for (const auto &playerElement : playersArray) {
        QJsonObject playerObject = playerElement.toObject();
        players.append(Player::fromJson(playerObject));
    }

    QJsonObject uiScriptsData = results.value("uiScriptsData").toObject();
    QJsonObject playersInfoObject = uiScriptsData.value("playersInfo").toObject();
    for (auto it = playersInfoObject.begin(); it != playersInfoObject.end(); ++it) {
        QJsonObject playerInfoObject = it.value().toObject();
        playersInfo.append(PlayerInfo::fromJson(playerInfoObject));
    }
}

QString Replay::getLevel() const { return level; }
double Replay::getTimePlayed() const { return timePlayed; }
QString Replay::getStatus() const { return status; }
QList<Player> Replay::getPlayers() const { return players; }
QString Replay::getAuthorId() const { return authorUserId; }
QString Replay::getSessionId() const { return sessionId; }
QString Replay::getBattleType() const { return battleType; }
int Replay::getStartTime() const { return startTime; }
