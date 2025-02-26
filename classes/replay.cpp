#include "replay.h"
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

const QByteArray Replay::MAGIC = QByteArray::fromHex("e5ac0010");

Replay::Replay(const QByteArray& buffer) {
	QDataStream stream(buffer);
	stream.setByteOrder(QDataStream::LittleEndian);

	QByteArray magic(4, 0);
	stream.readRawData(magic.data(), 4);
	if (magic != MAGIC) {
		throw std::runtime_error("Invalid magic number, maybe not a replay file?");
	}

	stream >> m_version;
	m_level = readString(stream, 128).replace("levels/", "").replace(".bin", "");
	m_levelSettings = readString(stream, 260);
	m_battleType = readString(stream, 128);
	m_environment = readString(stream, 128);
	m_visibility = readString(stream, 32);
	stream >> m_rezOffset;
	quint8 difficultyTemp;
	stream >> difficultyTemp;
	difficultyTemp &= 0x0F;
	m_difficulty = static_cast<Difficulty>(difficultyTemp);
	stream.skipRawData(35); // Skip 35 bytes
	stream >> m_sessionType;
	stream.skipRawData(7); // Skip 7 bytes

	quint64 sessionIdInt;
	stream >> sessionIdInt;
	m_sessionId = QString::number(sessionIdInt, 16);

	stream.skipRawData(4); // Skip 4 bytes
	stream >> m_mSetSize;
	stream.skipRawData(32); // Skip 32 bytes
	m_locName = readString(stream, 128);
	stream >> m_startTime >> m_timeLimit >> m_scoreLimit;
	stream.skipRawData(48); // Skip 48 bytes
	m_battleClass = readString(stream, 128);
	m_battleKillStreak = readString(stream, 128);

	try {
		auto results = unpackResults(m_rezOffset, buffer);
		parseResults(results);
	}
	catch (const std::exception& e) {
		qWarning() << "Error unpacking results:" << e.what();
	}
}

Replay::Replay()
{
}

Replay Replay::fromFile(const QString& filePath) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		throw std::runtime_error("Failed to open file");
	}

	QByteArray fileContent = file.readAll();
	return Replay(fileContent);
}

QString Replay::readString(QDataStream& stream, int length) {
	QByteArray bytes(length, 0);
	stream.readRawData(bytes.data(), length);

	int nullIndex = bytes.indexOf('\0');
	if (nullIndex != -1) {
		bytes.truncate(nullIndex);
	}

	return QString::fromUtf8(bytes);
}

QJsonObject Replay::unpackResults(int offset, const QByteArray& buffer) {
	QByteArray dataAfterRez = buffer.mid(offset);

	QProcess process;
	QString executablePath = QCoreApplication::applicationDirPath() + QString("/wt_ext_cli");
	QStringList arguments{ "--unpack_raw_blk", "--stdout", "--stdin", "--format", "Json" };

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

void Replay::parseResults(const QJsonObject& results) {
	this->m_status = results.value("status").toString("left");
	this->m_timePlayed = results.value("timePlayed").toDouble();
	this->m_authorUserId = results.value("authorUserId").toString();
	this->m_author = results.value("author").toString();

	if (this->m_authorUserId == "" || this->m_author == "") {
		this->m_authorUserId = "-1";
		this->m_author = "server";
	}

	QJsonArray playersArray = results.value("player").toArray();
	QJsonObject uiScriptsData = results.value("uiScriptsData").toObject();
	QJsonObject playersInfoObject = uiScriptsData.value("playersInfo").toObject();

	for (const auto& playerElement : playersArray) {
		QJsonObject playerObject = playerElement.toObject();
		for (auto it = playersInfoObject.begin(); it != playersInfoObject.end(); ++it) {
			QJsonObject playerInfoObject = it.value().toObject();
			if (playerInfoObject.value("id").toInteger() == playerObject.value("userId").toString().toULongLong()) {
				Player p = Player::fromJson(playerInfoObject);
				PlayerReplayData prd = PlayerReplayData::fromJson(playerObject);

				prd.setWaitTime(playerInfoObject.value("wait_time").toDouble());
				QJsonObject crafts = playerInfoObject.value("crafts").toObject();
				QList<QString> lineup;
				for (auto it1 = crafts.constBegin(); it1 != crafts.constEnd(); ++it1) {
					lineup.append(it1.value().toString());
				}
				prd.setLineup(lineup);
				this->m_players.insert(p, prd);
				break;
			}
		}
	}
}

int Replay::getVersion() const { return m_version; }
QString Replay::getSessionId() const { return m_sessionId; }
QString Replay::getLevel() const { return m_level; }
QString Replay::getLevelSettings() const { return m_levelSettings; }
QString Replay::getBattleType() const { return m_battleType; }
QString Replay::getEnvironment() const { return m_environment; }
QString Replay::getVisibility() const { return m_visibility; }
int Replay::getRezOffset() const { return m_rezOffset; }
Difficulty Replay::getDifficulty() const { return m_difficulty; }
quint8 Replay::getSessionType() const { return m_sessionType; }
int Replay::getSetSize() const { return m_mSetSize; }
QString Replay::getLocName() const { return m_locName; }
int Replay::getStartTime() const { return m_startTime; }
int Replay::getTimeLimit() const { return m_timeLimit; }
int Replay::getScoreLimit() const { return m_scoreLimit; }
QString Replay::getBattleClass() const { return m_battleClass; }
QString Replay::getBattleKillStreak() const { return m_battleKillStreak; }
QString Replay::getStatus() const { return m_status; }
double Replay::getTimePlayed() const { return m_timePlayed; }
QString Replay::getAuthorUserId() const { return m_authorUserId; }
QString Replay::getAuthor() const { return m_author; }
QMap<Player, PlayerReplayData> Replay::getPlayers() const { return m_players; }

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

void Replay::setDifficulty(Difficulty difficulty)
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

void Replay::setPlayers(QMap<Player, PlayerReplayData> players)
{
	this->m_players = players;
}
