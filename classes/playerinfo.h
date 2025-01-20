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

	QString getName();
	int getRank();
	QString getClanTag();
	QString getPlatform();
	QList<CraftInfo> getCraftsInfo();
	QString getUserId();

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
