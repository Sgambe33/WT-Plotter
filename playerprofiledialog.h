#ifndef PLAYERPROFILEDIALOG_H
#define PLAYERPROFILEDIALOG_H

#include <QDialog>
#include <QPair>
#include "classes/player.h"
#include "classes/playerreplaydata.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PlayerProfileDialog; }
QT_END_NAMESPACE

class PlayerProfileDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PlayerProfileDialog(QWidget* parent = nullptr);
	~PlayerProfileDialog();
	void setPlayerData(const QPair<Player, PlayerReplayData>& playerData);

//private slots:

//signals:


private:
	Ui::PlayerProfileDialog* ui;
	QPair<Player, PlayerReplayData> playerData;
};

#endif // PLAYERPROFILEDIALOG_H

