#include "replay.h"
#include "logger.h"
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

const QByteArray Replay::MAGIC = QByteArray::fromHex("e5ac0010");

Replay::~Replay() = default;

Replay::Replay(const QByteArray& buffer) {
    QDataStream stream(buffer);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setVersion(QDataStream::Qt_6_9);

    QByteArray magic(4, 0);
    if (stream.readRawData(magic.data(), 4) != 4) {
        LOG_WARN_GLOBAL("Failed to read magic number - insufficient data");
        throw std::runtime_error("Invalid replay format (header too short)");
    }

    if (magic != MAGIC) {
        LOG_WARN_GLOBAL(QString("Invalid magic number. Expected: %1, Got: %2").arg(MAGIC.toHex().constData(),magic.toHex().constData()));
        throw std::runtime_error("Invalid magic number, not a replay file");
    }

    try {
        stream >> m_version;
        m_level = readString(stream, 128).remove("levels/").remove(".bin");
        m_levelSettings = readString(stream, 260);
        m_battleType = readString(stream, 128);
        m_environment = readString(stream, 128);
        m_visibility = readString(stream, 32);
        stream >> m_rezOffset;

        quint8 difficultyTemp;
        stream >> difficultyTemp;
        m_difficulty = static_cast<Constants::Difficulty>(difficultyTemp & 0x0F);

        stream.skipRawData(35);
        stream >> m_sessionType;
        stream.skipRawData(7);

        quint64 sessionIdInt;
        stream >> sessionIdInt;
        m_sessionId = QString::number(sessionIdInt, 16);

        if (m_sessionId == "0") {
            throw std::runtime_error("Invalid session ID - test drive/user mission detected");
        }

        stream.skipRawData(4);
        stream >> m_mSetSize;
        stream.skipRawData(32);
        m_locName = readString(stream, 128);
        stream >> m_startTime >> m_timeLimit >> m_scoreLimit;
        stream.skipRawData(48);
        m_battleClass = readString(stream, 128);
        m_battleKillStreak = readString(stream, 128);

        if (stream.status() != QDataStream::Ok) {
            throw std::runtime_error("Data stream corrupted during header parsing");
        }

        auto results = unpackResults(m_rezOffset, buffer);
        parseResults(results);
    } catch (const std::exception& e) {
        LOG_ERROR_GLOBAL(QString("Replay parsing failed: %1").arg(e.what()));
        throw;
    }
}

Replay::Replay(){}

Replay Replay::fromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Failed to open file");
    }

    QByteArray fileContent = file.readAll();
    return Replay(fileContent);
}

QString Replay::readString(QDataStream& stream, int maxLength) {
    QByteArray buffer(maxLength, 0);
    if (stream.readRawData(buffer.data(), maxLength) != maxLength) {
        LOG_ERROR_GLOBAL(QString("Failed to read full string buffer of length %1").arg(maxLength));
        throw std::runtime_error("Unexpected end of data while reading string");
    }

    const int nullPos = buffer.indexOf('\0');
    if (nullPos != -1) {
        buffer.truncate(nullPos);
    }
    return QString::fromUtf8(buffer).trimmed();
}

QJsonObject Replay::unpackResults(int offset, const QByteArray& buffer) {
    const QByteArray data = buffer.mid(offset);
    if (data.isEmpty()) {
        LOG_ERROR_GLOBAL(QString("Empty results data at offset %1").arg(offset));
        return QJsonObject();
    }

    const QString exePath = QCoreApplication::applicationDirPath() + "/wt_ext_cli" +
#ifdef Q_OS_WIN
                            ".exe";
#else
                            "";
#endif

    if (!QFileInfo::exists(exePath)) {
        LOG_WARN_GLOBAL(QString("External tool not found: %1").arg(exePath.toUtf8().constData()));
        throw std::runtime_error("Required unpacking tool not found");
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args = {"--unpack_raw_blk", "--stdout", "--stdin", "--format", "Json"};

    LOG_INFO_GLOBAL(QString("Starting unpack process: %1 %2").arg(exePath.toUtf8().constData(),args.join(' ').toUtf8().constData()));

    process.start(exePath, args);
    if (!process.waitForStarted(5000)) {
        LOG_ERROR_GLOBAL(QString("Process start failed: %1 (error code %2)").arg(process.errorString().toUtf8().constData(),process.error()));
        throw std::runtime_error("Failed to start unpacking process");
    }

    process.write(data);
    process.closeWriteChannel();

    if (!process.waitForFinished(15000)) {
        LOG_ERROR_GLOBAL(QString("Process timeout exceeded: %1").arg(process.errorString().toUtf8().constData()));
        throw std::runtime_error("Unpacking process timed out");
    }

    if (process.exitCode() != 0) {
        const QByteArray errorOutput = process.readAllStandardError();
        LOG_ERROR_GLOBAL(QString("Process failed with code %1. Error output: %2").arg((process.exitCode(),errorOutput.constData())));
        throw std::runtime_error("Unpacking process returned error");
    }

    const QByteArray output = process.readAllStandardOutput();
    if (output.isEmpty()) {
        LOG_WARN_GLOBAL("Empty output from unpacking process");
        return QJsonObject();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(output, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR_GLOBAL(QString("JSON parse error: %1 at offset %2. Data sample: %3 ...").arg((parseError.errorString().toUtf8().constData(),parseError.offset,output.constData())));
        throw std::runtime_error("Invalid JSON output from unpacker");
    }

    if (!doc.isObject()) {
        LOG_WARN_GLOBAL("Unexpected JSON structure - root is not an object");
        throw std::runtime_error("Invalid JSON structure");
    }

    return doc.object();
}

void Replay::parseResults(const QJsonObject& results) {
    m_status = results.value("status").toString("left");
    m_timePlayed = results.value("timePlayed").toDouble();
    m_authorUserId = results.value("authorUserId").toString();
    m_author = results.value("author").toString();

    // Handle server-authored replays
    if (m_authorUserId.isEmpty() || m_author.isEmpty()) {
        LOG_INFO_GLOBAL("No author information - assuming server replay");
        m_authorUserId = "-1";
        m_author = "server";
    }

    const QJsonArray playersArray = results.value("player").toArray();
    const QJsonObject uiScriptsData = results.value("uiScriptsData").toObject();
    const QJsonObject playersInfo = uiScriptsData.value("playersInfo").toObject();

    // Preprocess player info for O(1) lookups
    QHash<quint64, QJsonObject> playerInfoMap;
    for (auto it = playersInfo.constBegin(); it != playersInfo.constEnd(); ++it) {
        const QJsonObject info = it.value().toObject();
        const quint64 userId = info.value("id").toVariant().toULongLong();
        playerInfoMap.insert(userId, info);
    }

    for (const QJsonValue& playerValue : playersArray) {
        const QJsonObject playerObj = playerValue.toObject();
        const quint64 userId = playerObj.value("userId").toString().toULongLong();

        if (!playerInfoMap.contains(userId)) {
            LOG_WARN_GLOBAL(QString("Missing player info for user ID %1").arg(userId));
            continue;
        }

        const QJsonObject& infoObj = playerInfoMap.value(userId);
        try {
            Player p = Player::fromJson(infoObj);
            PlayerReplayData prd = PlayerReplayData::fromJson(playerObj);

            // Set additional replay-specific data
            prd.setWaitTime(infoObj.value("wait_time").toDouble());

            // Process vehicle lineup
            QJsonObject crafts = infoObj.value("crafts").toObject();
            QList<QString> lineup;
            for (auto craftIt = crafts.constBegin(); craftIt != crafts.constEnd(); ++craftIt) {
                lineup.append(craftIt.value().toString());
            }
            prd.setLineup(lineup);

            m_players.append(qMakePair(p, prd));
        } catch (const std::exception& e) {
            LOG_WARN_GLOBAL(QString("Failed to parse player data for user %1: %2").arg((userId, e.what())));
        }
    }

    if (m_players.isEmpty()) {
        LOG_WARN_GLOBAL("No valid player data found in replay results");
    }
}

QString Replay::getSessionId() const { return m_sessionId; }
QString Replay::getLevel() const { return m_level; }
QString Replay::getBattleType() const { return m_battleType; }
Constants::Difficulty Replay::getDifficulty() const { return m_difficulty; }
int Replay::getStartTime() const { return m_startTime; }
QString Replay::getStatus() const { return m_status; }
double Replay::getTimePlayed() const { return m_timePlayed; }
QString Replay::getAuthorUserId() const { return m_authorUserId; }
QList<QPair<Player, PlayerReplayData>> Replay::getPlayers() const { return m_players; }

void Replay::setSessionId(QString sessionId)
{
    this->m_sessionId = sessionId;
}

void Replay::setAuthorUserId(QString authorUserId)
{
    this->m_authorUserId = authorUserId;
}

void Replay::setStartTime(int startTime)
{
    this->m_startTime = startTime;
}

void Replay::setLevel(QString level)
{
    this->m_level = level;
}

void Replay::setBattleType(QString battleType)
{
    this->m_battleType = battleType;
}

void Replay::setDifficulty(Constants::Difficulty difficulty)
{
    this->m_difficulty = difficulty;
}

void Replay::setStatus(QString status)
{
    this->m_status = status;
}

void Replay::setTimePlayed(double timePlayed)
{
    this->m_timePlayed = timePlayed;
}

void Replay::setPlayers(QList<QPair<Player, PlayerReplayData>> players)
{
    this->m_players = players;
}
