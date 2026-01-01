#include "playerprofiledialog.h"
#include "ui_playerprofiledialog.h"

PlayerProfileDialog::PlayerProfileDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::PlayerProfileDialog),
	settings(new QSettings("sgambe33", "wtplotter", this))
{
	ui->setupUi(this);

	int id = QFontDatabase::addApplicationFont(":/fonts/wt_symbols.ttf");
	wtSymbols = QFont(QFontDatabase::applicationFontFamilies(id).at(0));

	ui->lineupTable->setColumnCount(5);
	ui->lineupTable->setHorizontalHeaderLabels({ tr("Vehicle"), tr("Rank"), tr("Arcade BR"), tr("Realistic BR"), tr("Simulator BR") });
	ui->lineupTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

	connect(ui->playerProfileButton, &QPushButton::clicked, this, [this] {
		QString url = "https://warthunder.com/en/community/searchplayers?name=" + playerData.first.getUserId();
		QDesktopServices::openUrl(QUrl(url));
		});
}

PlayerProfileDialog::~PlayerProfileDialog()
{
	delete ui;
}

void PlayerProfileDialog::setPlayerData(const QPair<Player, PlayerReplayData>& playerData)
{
	this->playerData = playerData;
	QString username = playerData.first.getUsername().replace("@psn", "").replace("@live", "");
	setWindowTitle(username);
	ui->countryLabel->setPixmap(QPixmap(":/icons/" + Utils::getJsonFromResources(":/translations/vehicles.json", playerData.second.getLineup().first()).value("country").toString("ussr") + ".png"));
	ui->usernameLabel->setText(playerData.first.getSquadronTag() + " " + username);
	ui->usernameLabel->setFont(wtSymbols);
	ui->platformLabel->setText(tr(playerData.first.getPlatform().toStdString().c_str()));

	ui->lineupTable->setRowCount(0);

	for (const auto& vehicle : playerData.second.getLineup()) {
		QJsonObject obj = Utils::getJsonFromResources(":/translations/vehicles.json", vehicle);
		int row = ui->lineupTable->rowCount();
		ui->lineupTable->insertRow(row);

		QTableWidgetItem* item = new QTableWidgetItem(obj.value(settings->value("language", "en").toString()).toString());
		item->setFont(wtSymbols);
		
		ui->lineupTable->setItem(row, 0, item);
		ui->lineupTable->setItem(row, 1, new QTableWidgetItem(QString::number(obj.value("rank").toInt())));
		ui->lineupTable->setItem(row, 2, new QTableWidgetItem(QString::number(obj.value("ab_br").toDouble())));
		ui->lineupTable->setItem(row, 3, new QTableWidgetItem(QString::number(obj.value("rb_br").toDouble())));
		ui->lineupTable->setItem(row, 4, new QTableWidgetItem(QString::number(obj.value("sb_br").toDouble())));
	}

	ui->lineupTable->resizeColumnsToContents();
}
