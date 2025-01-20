#ifndef CRAFTINFO_H
#define CRAFTINFO_H
#include <QString>
#include <QJsonObject>
class CraftInfo
{
public:
	CraftInfo();
	static CraftInfo fromJson(const QJsonObject& craftInfoObject);

	int getRank();
	int getMrank();
	bool isRankUnused();
	QString getType();
	QString getName();

private:
	QString name;
	QString type;
	bool rankUnused;
	int mrank;
	int rank;
};

#endif // CRAFTINFO_H
