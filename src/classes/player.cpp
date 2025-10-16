#include "player.h"

Player::Player(){}

Player Player::fromJson(const QJsonObject& playerInfoObject) {
	Player player;

	auto getString = [&playerInfoObject](const QString& key, const QString& defaultValue = "") {
		return playerInfoObject.contains(key) && playerInfoObject[key].isString() ? playerInfoObject[key].toString() : defaultValue;
	};

	auto getIntAsString = [&playerInfoObject](const QString& key, const QString& defaultValue = "") {
		return playerInfoObject.contains(key) && playerInfoObject[key].isDouble() ? QString::number(playerInfoObject[key].toInt()) : defaultValue;
	};

	player.m_userId = getIntAsString("id");
	player.m_username = getString("name");

	player.m_squadronId = getIntAsString("clanId");
	if (player.m_squadronId == "-1") {
		player.m_squadronId = QString(); // Set to empty string instead of NULL
	}

	player.m_squadronTag = getString("clanTag");
	if (player.m_squadronTag.isEmpty()) {
		player.m_squadronTag = QString(); // Set to empty string instead of NULL
	}

	player.m_platform = getString("platform");

	return player;
}

QString Player::getUserId() const { return m_userId; }
QString Player::getUsername() const { return m_username; }
QString Player::getSquadronId() const { return m_squadronId; }
QString Player::getSquadronTag() const { return m_squadronTag; }
QString Player::getPlatform() const { return m_platform; }

void Player::setUserId(QString userId)
{
	m_userId = userId;
}

void Player::setUsername(QString username)
{
	m_username = username;
}

void Player::setSquadronId(QString squadronId)
{
	m_squadronId = squadronId;
}

void Player::setSquadronTag(QString squadronTag)
{
	m_squadronTag = squadronTag;
}

void Player::setPlatform(QString platform)
{
	m_platform = platform;
}
