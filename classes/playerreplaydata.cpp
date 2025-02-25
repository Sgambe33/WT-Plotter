#include "playerreplaydata.h"

PlayerReplayData::PlayerReplayData() {}

PlayerReplayData PlayerReplayData::fromJson(const QJsonObject& json) {
	PlayerReplayData playerReplayData;

	auto getInt = [&json](const QString& key, int defaultValue = 0) {
		return json.contains(key) && json[key].isDouble() ? json[key].toInt() : defaultValue;
		};

	auto getBool = [&json](const QString& key, bool defaultValue = false) {
		return json.contains(key) && json[key].isBool() ? json[key].toBool() : defaultValue;
		};

	auto getString = [&json](const QString& key, const QString& defaultValue = "") {
		return json.contains(key) && json[key].isString() ? json[key].toString() : defaultValue;
		};

	playerReplayData.m_userId = getString("userId");
	playerReplayData.m_squad = getInt("squadId");
	playerReplayData.m_autosquad = getBool("autoSquad");
	playerReplayData.m_team = getInt("team");
	playerReplayData.m_kills = getInt("kills");
	playerReplayData.m_groundKills = getInt("groundKills");
	playerReplayData.m_navalKills = getInt("navalKills");
	playerReplayData.m_teamKills = getInt("teamKills");
	playerReplayData.m_aiKills = getInt("aiKills");
	playerReplayData.m_aiGroundKills = getInt("aiGroundKills");
	playerReplayData.m_aiNavalKills = getInt("aiNavalKills");
	playerReplayData.m_assists = getInt("assists");
	playerReplayData.m_deaths = getInt("deaths");
	playerReplayData.m_captureZone = getInt("captureZone");
	playerReplayData.m_damageZone = getInt("damageZone");
	playerReplayData.m_score = getInt("score");
	playerReplayData.m_awardDamage = getInt("awardDamage");
	playerReplayData.m_missileEvades = getInt("missileEvades");

	return playerReplayData;
}

QString PlayerReplayData::getUserId() const { return m_userId; }
int PlayerReplayData::getSquad() const { return m_squad; }
bool PlayerReplayData::getAutoSquad() const { return m_autosquad; }
int PlayerReplayData::getTeam() const { return m_team; }
float PlayerReplayData::getWaitTime() const { return m_waitTime; }
int PlayerReplayData::getKills() const { return m_kills; }
int PlayerReplayData::getGroundKills() const { return m_groundKills; }
int PlayerReplayData::getNavalKills() const { return m_navalKills; }
int PlayerReplayData::getTeamKills() const { return m_teamKills; }
int PlayerReplayData::getAiKills() const { return m_aiKills; }
int PlayerReplayData::getAiGroundKills() const { return m_aiGroundKills; }
int PlayerReplayData::getAiNavalKills() const { return m_aiNavalKills; }
int PlayerReplayData::getAssists() const { return m_assists; }
int PlayerReplayData::getDeaths() const { return m_deaths; }
int PlayerReplayData::getCaptureZone() const { return m_captureZone; }
int PlayerReplayData::getDamageZone() const { return m_damageZone; }
int PlayerReplayData::getScore() const { return m_score; }
int PlayerReplayData::getAwardDamage() const { return m_awardDamage; }
int PlayerReplayData::getMissileEvades() const { return m_missileEvades; }
QList<QString> PlayerReplayData::getLineup() const { return m_lineup; }

void PlayerReplayData::setUserId(QString userId)
{
	m_userId = userId;
}

void PlayerReplayData::setSquad(int squad) { this->m_squad = squad; }
void PlayerReplayData::setAutoSquad(bool autosquad) { this->m_autosquad = autosquad; }
void PlayerReplayData::setTeam(int team) { this->m_team = team; }
void PlayerReplayData::setKills(int kills) { this->m_kills = kills; }
void PlayerReplayData::setGroundKills(int groundKills) { this->m_groundKills = groundKills; }
void PlayerReplayData::setNavalKills(int navalKills) { this->m_navalKills = navalKills; }
void PlayerReplayData::setTeamKills(int teamKills) { this->m_teamKills = teamKills; }
void PlayerReplayData::setAiKills(int aiKills) { this->m_aiKills = aiKills; }
void PlayerReplayData::setAiGroundKills(int aiGroundKills) { this->m_aiGroundKills = aiGroundKills; }
void PlayerReplayData::setAiNavalKills(int aiNavalKills) { this->m_aiNavalKills = aiNavalKills; }
void PlayerReplayData::setAssists(int assists) {
	this->m_assists = assists;
}
void PlayerReplayData::setDeaths(int deaths) { this->m_deaths = deaths; }
void PlayerReplayData::setCaptureZone(int captureZone) { this->m_captureZone = captureZone; }
void PlayerReplayData::setDamageZone(int damageZone) { this->m_damageZone = damageZone; }
void PlayerReplayData::setScore(int score) { this->m_score = score; }
void PlayerReplayData::setAwardDamage(int awardDamage) { this->m_awardDamage = awardDamage; }
void PlayerReplayData::setMissileEvades(int missileEvades) { this->m_missileEvades = missileEvades; }



void PlayerReplayData::setWaitTime(float waitTime) { this->m_waitTime = waitTime; }
void PlayerReplayData::setLineup(QList<QString> lineup) { this->m_lineup = lineup; }