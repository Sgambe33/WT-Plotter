#ifndef PLAYERINFO_H
#define PLAYERINFO_H
#include <QString>
#include <QJsonObject>
class Player
{
public:
	Player();
	static Player fromJson(const QJsonObject& playerInfoObject);

	QString getUserId() const;
	QString getUsername() const;
	QString getSquadronId() const;
	QString getSquadronTag() const;
	QString getPlatform() const;

	void setUserId(QString userId);
	void setUsername(QString username);
	void setSquadronId(QString squadronId);
	void setSquadronTag(QString squadronTag);
	void setPlatform(QString platform);

	bool operator<(const Player& other) const {
		return this->m_userId < other.m_userId;
	}
	
private:
	QString m_userId;
	QString m_username;
	QString m_squadronId;
	QString m_squadronTag;
	QString m_platform;
};

#endif // PLAYERINFO_H
