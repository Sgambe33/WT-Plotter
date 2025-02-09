#ifndef PLAYERINFO_H
#define PLAYERINFO_H
#include <QString>
#include <QList>
#include "craftinfo.h"
class PlayerInfo
{
public:
	PlayerInfo();
	static PlayerInfo fromJson(const QJsonObject& playerInfoObject);
	QList<CraftInfo> parseCraftsInfo(const QJsonObject& craftInfoObject);

	QString getName() const;
	int getRank() const;
	QString getClanTag() const;
	QString getPlatform() const;
	QList<CraftInfo> getCraftsInfo() const;
	QString getUserId() const;
	QJsonObject getCraftLineup() const;
	QString getClanId() const;

private:
	QString name;
	int team;
	QString clanTag;
	QString platform;
	QString id;
	int slot;
	QString country;
	int mrank;
	bool autoSquad;
	int waitTime;
	QString clanId;
	int tier;
	int squad;
	int rank;
	QList<CraftInfo> craftsInfo;
};

#endif // PLAYERINFO_H
