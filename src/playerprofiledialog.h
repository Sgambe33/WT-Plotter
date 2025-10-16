#ifndef PLAYERPROFILEDIALOG_H
#define PLAYERPROFILEDIALOG_H

#include <QDialog>
#include <QFont>
#include <QSettings>
#include <QPair>
#include <QTableWidgetItem>
#include <QJsonObject>
#include <QFileDialog>
#include <QDesktopServices>
#include "classes/utils.h"
#include "classes/player.h"
#include "classes/playerreplaydata.h"

namespace Ui {
	class PlayerProfileDialog;
}

class Player;
class PlayerReplayData;

class PlayerProfileDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PlayerProfileDialog(QWidget* parent = nullptr);
	~PlayerProfileDialog();

	void setPlayerData(const QPair<Player, PlayerReplayData>& playerData);

private:
	Ui::PlayerProfileDialog* ui;
	QFont wtSymbols;
	QSettings* settings;
    QString playerId;
};

#endif // PLAYERPROFILEDIALOG_H
