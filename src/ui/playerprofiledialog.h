#ifndef PLAYERPROFILEDIALOG_H
#define PLAYERPROFILEDIALOG_H

#include <QDialog>
#include <QFont>
#include <QSettings>
#include <QTableWidgetItem>
#include <QJsonObject>
#include <QHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDesktopServices>
#include <QUrl>
#include "../classes/utils.h"
#include "Structs.h"

struct UiPlayerData;

namespace Ui {
	class PlayerProfileDialog;
}

class PlayerProfileDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PlayerProfileDialog(QWidget* parent = nullptr);
	~PlayerProfileDialog();

	void setPlayerData(const UiPlayerData& playerData);

private:
	Ui::PlayerProfileDialog* ui;
	QFont wtSymbols;
	QSettings* settings;
	QString playerId;

	QHash<QString, QJsonObject> m_vehicleDatabase;
	void loadVehicleDatabase();
};

#endif // PLAYERPROFILEDIALOG_H