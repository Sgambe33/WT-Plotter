#ifndef CRAFTINFO_H
#define CRAFTINFO_H
#include <QString>
#include <QJsonObject>
class CraftInfo
{
public:
	CraftInfo();
	static CraftInfo fromJson(const QJsonObject& craftInfoObject);

	int getRank() const;
	int getMrank() const;
	bool isRankUnused() const;
	QString getType() const;
	QString getName() const;

private:
	QString name;
	QString type;
	bool rankUnused;
	int mrank;
	int rank;
};

#endif // CRAFTINFO_H
