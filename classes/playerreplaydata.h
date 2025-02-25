#ifndef PLAYERREPLAYDATA_H
#define PLAYERREPLAYDATA_H

#include <QString>
#include <QJsonObject>
#include <QList>

class PlayerReplayData {
public:
	PlayerReplayData();

	static PlayerReplayData fromJson(const QJsonObject& json);

	QString getUserId() const;
	int getSquad() const;
	bool getAutoSquad() const;
	int getTeam() const;
	float getWaitTime() const;
	int getKills() const;
	int getGroundKills() const;
	int getNavalKills() const;
	int getTeamKills() const;
	int getAiKills() const;
	int getAiGroundKills() const;
	int getAiNavalKills() const;
	int getAssists() const;
	int getDeaths() const;
	int getCaptureZone() const;
	int getDamageZone() const;
	int getScore() const;
	int getAwardDamage() const;
	int getMissileEvades() const;
	QList<QString> getLineup() const;

	void setUserId(QString userId);
	void setSquad(int squad);
	void setAutoSquad(bool autosquad);
	void setTeam(int team);
	void setWaitTime(float waitTime);
	void setKills(int kills);
	void setGroundKills(int groundKills);
	void setNavalKills(int navalKills);
	void setTeamKills(int teamKills);
	void setAiKills(int aiKills);
	void setAiGroundKills(int aiGroundKills);
	void setAiNavalKills(int aiNavalKills);
	void setAssists(int assists);
	void setDeaths(int deaths);
	void setCaptureZone(int captureZone);
	void setDamageZone(int damageZone);
	void setScore(int score);
	void setAwardDamage(int awardDamage);
	void setMissileEvades(int missileEvades);
	void setLineup(QList<QString> lineup);

private:
	QString m_userId;
	int m_squad;
	bool m_autosquad;
	int m_team;
	float m_waitTime;
	int m_kills;
	int m_groundKills;
	int m_navalKills;
	int m_teamKills;
	int m_aiKills;
	int m_aiGroundKills;
	int m_aiNavalKills;
	int m_assists;
	int m_deaths;
	int m_captureZone;
	int m_damageZone;
	int m_score;
	int m_awardDamage;
	int m_missileEvades;
	QList<QString> m_lineup;
};

#endif // PLAYERREPLAYDATA_H