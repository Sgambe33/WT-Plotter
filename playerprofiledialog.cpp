#include "playerprofiledialog.h"
#include "./ui_playerprofiledialog.h"
#include <QFileDialog>
#include <classes/utils.h>
#include <QDesktopServices>

PlayerProfileDialog::PlayerProfileDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::PlayerProfileDialog)
{
	ui->setupUi(this);
}

PlayerProfileDialog::~PlayerProfileDialog()
{
	delete ui;
}

void PlayerProfileDialog::setPlayerData(const QPair<Player, PlayerReplayData>& playerData)
{
	QSettings settings("sgambe33", "wtplotter");
	QString languageCode = settings.value("language", "en").toString();
	this->playerData = playerData;
	setWindowTitle(playerData.first.getUsername() + "'s profile");
	ui->usernameLabel->setText(playerData.first.getSquadronTag() + " " + playerData.first.getUsername());
	ui->platformLabel->setText(playerData.first.getPlatform());

	ui->lineupTable->clear();
	ui->lineupTable->setRowCount(playerData.second.getLineup().size());
	ui->lineupTable->setColumnCount(5);

	ui->lineupTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

	ui->lineupTable->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Vehicle")));
	ui->lineupTable->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Rank")));
	ui->lineupTable->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Arcade BR")));
	ui->lineupTable->setHorizontalHeaderItem(3, new QTableWidgetItem(tr("Realistic BR")));
	ui->lineupTable->setHorizontalHeaderItem(4, new QTableWidgetItem(tr("Simulator BR")));

	int id = QFontDatabase::addApplicationFont(":/fonts/wt_symbols.ttf");
	qDebug() << "Font ID:" << id;
	QString family = QFontDatabase::applicationFontFamilies(id).at(0);
	QFont customFont(family);


	int row = 0;
	for (const auto& vehicle : playerData.second.getLineup()) {
		QJsonObject obj = Utils::getJsonFromResources(":/translations/vehicles.json", vehicle);
		QTableWidgetItem* item = new QTableWidgetItem();
		item->setFont(customFont);
		item->setText(obj.value(languageCode).toString());

		ui->lineupTable->setItem(row, 0, item);
		ui->lineupTable->setItem(row, 1, new QTableWidgetItem(QString::number(obj.value("rank").toInt())));
		ui->lineupTable->setItem(row, 2, new QTableWidgetItem(QString::number(obj.value("ab_br").toDouble())));
		ui->lineupTable->setItem(row, 3, new QTableWidgetItem(QString::number(obj.value("rb_br").toDouble())));
		ui->lineupTable->setItem(row, 4, new QTableWidgetItem(QString::number(obj.value("sb_br").toDouble())));
		row++;
	}
	// Adjust column widths
	ui->lineupTable->resizeColumnsToContents();

	connect(ui->playerProfileButton, &QPushButton::clicked, [=] {
		QString url = "https://warthunder.com/en/community/searchplayers?name=" + playerData.first.getUserId();
		QDesktopServices::openUrl(QUrl(url));
		});
}
