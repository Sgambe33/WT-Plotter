#include "mainwindow.h"
#include "classes/utils.h"
#include "./ui_mainwindow.h"
#include "classes/replay.h"
#include "sceneimageviewer.h"
#include "worker.h"
#include "preferencesdialog.h"
#include <QPainter>
#include <QTreeView>
#include <QStandardItemModel>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QThread>
#include <QStandardItem>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QLabel>
#include <QDebug>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include <QSqlDatabase>
#include <classes/replayloaderworker.h>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QPalette>
#include <playerprofiledialog.h>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::MainWindow),
	model(new QStandardItemModel(this)),
	m_thread(nullptr),
	m_worker(nullptr),
	appTranslator(new QTranslator(this)),
	m_dbmanager(QString(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/wtplotter/replays.sqlite3"), "mainwindow")
{
	ui->setupUi(this);
	QSettings settings("sgambe33", "wtplotter");

	emit refreshReplays();

	ui->splitter->setStretchFactor(0, 2);
	ui->splitter->setStretchFactor(1, 3);

	QPixmap img(":/map_images/unknownmap.png");
	img = img.scaled(125, 125, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	ui->mapImage->setPixmap(img);

	connect(ui->replayButton, &QPushButton::clicked, [=] {
		ui->stackedWidget_2->setCurrentIndex(0);
		ui->replayTreeView->setDisabled(false);
		ui->plotterButton->setDisabled(false);
		ui->replayButton->setDisabled(true);
		stopPlotter();
		});

	connect(ui->plotterButton, &QPushButton::clicked, [=] {
		ui->stackedWidget_2->setCurrentIndex(2);
		ui->replayTreeView->setDisabled(true);
		ui->plotterButton->setDisabled(true);
		ui->replayButton->setDisabled(false);
		startPlotter();
		});

	connect(ui->refreshButton, &QPushButton::clicked, [=] {
		emit refreshReplays();
		});

	connect(ui->expandButton, &QPushButton::clicked, [=] {
		ui->replayTreeView->expandAll();
		});

	connect(ui->collapseButton, &QPushButton::clicked, [=] {
		ui->replayTreeView->collapseAll();
		});


	ui->replayTreeView->setDisabled(true);
	ui->plotterButton->setDisabled(true);
	ui->openServerReplayButton->setDisabled(true);

	Utils::checkAppVersion();

	startPlotter();
	connect(ui->replayTreeView, &QTreeView::clicked, this, &MainWindow::onTreeItemClicked);
	connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::openPreferencesDialog);
	connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::openAboutDialog);
	connect(ui->actionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);

	loadReplaysFromFolder();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::openPreferencesDialog()
{
	PreferencesDialog dialog(this);
	connect(&dialog, &PreferencesDialog::languageChanged, this, &MainWindow::onLanguageChanged);
	if (dialog.exec()) {
		refreshReplays();
	}
}

void MainWindow::openAboutDialog()
{
	QString aboutText = tr(R"(
        <p>WT Plotter is a tool for reading War Thunder replays and record match development. This 
        project is developed by <strong>Sgambe33</strong> and is fully open source.You can find the 
        source code and contribute to the project on <a href='https://github.com/sgambe33/wt-plotter'>
        GitHub</a>.</p>
        <p>Thank you for using WT Plotter!</p>
    )");
	QMessageBox::about(this, tr("About WT Plotter"), aboutText);
}

void MainWindow::startPlotter() {
	qDebug() << "Starting plotter...";
	m_thread = new QThread();
	m_worker = new Worker(ui->mappa);

	connect(m_thread, &QThread::started, m_worker, &Worker::performTask);
	connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::stopPlotter);
	connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
	connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);
	connect(m_worker, &Worker::updatePixmap, ui->mappa, &SceneImageViewer::setPixmap);
	connect(m_worker, &Worker::updateStatusLabel, this, &MainWindow::updateStatusLabel);
	connect(m_worker, &Worker::updateProgressBar, this, &MainWindow::updateProgressBar);
	connect(m_worker, &Worker::changeStackedWidget2, this, &MainWindow::changeStackedWidget2);

	m_worker->moveToThread(m_thread);
	m_thread->start();
}

void MainWindow::updatePixmap(const QPixmap& pixmap)
{
	ui->mappa->setPixmap(pixmap);
}

void MainWindow::refreshReplays() {
	QSettings settings("sgambe33", "wtplotter");

	if (!settings.value("replayFolderPath").isNull()) {
		loadReplaysFromFolder();
	}
}

void MainWindow::loadReplaysFromFolder() {
	QSettings settings("sgambe33", "wtplotter");
	QString folderPath = settings.value("replayFolderPath").toString();
	if (folderPath.isEmpty())
		return;

	ui->stackedWidget_1->setCurrentIndex(1);
	ui->replayLoadingProgressBar->setRange(0, 100);
	ui->replayLoadingProgressBar->setValue(0);
	ui->replayLoadingProgressBar->setTextVisible(true);

	QThread* thread = new QThread();
	ReplayLoaderWorker* worker = new ReplayLoaderWorker(folderPath, QString(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/wtplotter/replays.sqlite3"));
	worker->moveToThread(thread);

	connect(thread, &QThread::started, worker, &ReplayLoaderWorker::loadReplays);
	connect(worker, &ReplayLoaderWorker::progressUpdated, ui->replayLoadingProgressBar, &QProgressBar::setValue);
	connect(worker, &ReplayLoaderWorker::finished, this, &MainWindow::onReplayLoaderFinished);
	connect(thread, &QThread::finished, worker, &QObject::deleteLater);
	connect(thread, &QThread::finished, thread, &QObject::deleteLater);

	thread->start();
}

void MainWindow::onReplayLoaderFinished()
{
	ui->stackedWidget_1->setCurrentIndex(0);
	QSettings settings("sgambe33", "wtplotter");
	populateReplayTreeView(ui->replayTreeView, settings.value("replayFolderPath").toString());
}

void MainWindow::changeStackedWidget1(int index)
{
	ui->stackedWidget_1->setCurrentIndex(index);
}

void MainWindow::changeStackedWidget2(int index)
{
	ui->stackedWidget_2->setCurrentIndex(index);
}

void MainWindow::updateProgressBar(double progress) {
	ui->progressBar->setValue(progress);
}

void MainWindow::updateStatusLabel(QString msg) {
	ui->statusLabel->setText(msg);
}

void MainWindow::stopPlotter() {
	if (m_thread && m_worker) {
		QMetaObject::invokeMethod(m_worker, "stopTimer", Qt::QueuedConnection);

		m_thread->quit();
		m_thread->wait();

		m_thread = nullptr;
		m_worker = nullptr;

		qDebug() << "Killing plotter and thread...";
	}
}

void MainWindow::populateReplayTreeView(QTreeView* replayTreeView, const QString& directoryPath)
{
	QSettings settings("sgambe33", "wtplotter");
	QString languageCode = settings.value("language", "en").toString();
	model->clear();
	model->setHorizontalHeaderLabels({ tr("File Name") });

	QMap<QDate, QList<Replay>> replaysByDate = m_dbmanager.fetchReplaysGroupedByDate();
	for (auto it = replaysByDate.cbegin(); it != replaysByDate.cend(); ++it)
	{
		QStandardItem* dateItem = new QStandardItem(it.key().toString("yyyy-MM-dd"));
		dateItem->setFlags(dateItem->flags() & ~Qt::ItemIsEditable);
		for (const Replay& replay : it.value())
		{
			QJsonObject obj;
			if (replay.getLevel().endsWith("_snow")) {
				obj = Utils::getJsonFromResources(":/translations/locations.json", replay.getLevel().replace("_snow", ""));
			}
			else {
				obj = Utils::getJsonFromResources(":/translations/locations.json", replay.getLevel());
			}

			ui->mapNameLabel->setText(tr("Map: ") + obj.value(languageCode).toString());
			QStandardItem* fileNameItem = new QStandardItem(Utils::epochSToFormattedTime(replay.getStartTime()) + " - " + obj.value(languageCode).toString("Uknown map"));
			fileNameItem->setData(replay.getSessionId(), Qt::UserRole);
			fileNameItem->setFlags(fileNameItem->flags() & ~Qt::ItemIsEditable);
			dateItem->appendRow({ fileNameItem });
		}
		model->appendRow(dateItem);
	}

	replayTreeView->setModel(model);
	replayTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void MainWindow::onTreeItemClicked(const QModelIndex& index)
{
	QString sessionId = index.data(Qt::UserRole).toString();
	executeCommand(sessionId);
}

void MainWindow::executeCommand(const QString& sessionId)
{
	QSettings settings("sgambe33", "wtplotter");
	QString languageCode = settings.value("language", "en").toString();

	qDebug() << languageCode;

	Replay rep = m_dbmanager.getReplayBySessionId(sessionId);

	if (rep.getSessionId().isEmpty()) {
		ui->openServerReplayButton->setDisabled(true);
		return;
	}

	disconnect(ui->openServerReplayButton, &QPushButton::clicked, nullptr, nullptr);

	connect(ui->openServerReplayButton, &QPushButton::clicked, [=] {
		QString url = "https://warthunder.com/en/tournament/replay/" + rep.getSessionId();
		QDesktopServices::openUrl(QUrl(url));
		});

	ui->openServerReplayButton->setDisabled(false);

	QPixmap mapPixmap(":/map_images/unknownmap.png");
	if (rep.getLevel().contains("avg")) {
		mapPixmap = ":/map_images/" + rep.getLevel() + "_tankmap_thumb.png";
	}
	else {
		mapPixmap = ":/map_images/" + rep.getLevel() + "_map_thumb.png";
	}
	if (mapPixmap.isNull()) {
		mapPixmap = QPixmap(":/map_images/unknownmap.png");
	}

	ui->mapImage->setPixmap(mapPixmap);
	QJsonObject obj;
	if (rep.getLevel().endsWith("_snow")) {
		obj = Utils::getJsonFromResources(":/translations/locations.json", rep.getLevel().replace("_snow", ""));
	}
	else {
		obj = Utils::getJsonFromResources(":/translations/locations.json", rep.getLevel());
	}

	ui->sessionIdLabel->setText(tr("Session ID: ") + rep.getSessionId());
	ui->mapNameLabel->setText(tr("Map: ") + obj.value(languageCode).toString("Uknown map"));
	ui->difficultyLabel->setText(tr("Difficulty: ") + Utils::difficultyToStringLocaleAware(rep.getDifficulty()));
	ui->startTimeLabel->setText(tr("Start time: ") + Utils::epochSToFormattedTime(rep.getStartTime()));
	ui->timePlayedLabel->setText(tr("Time played: ") + Utils::replayLengthToString(rep.getTimePlayed()));
	ui->resultLabel->setText(tr("Result: ") + rep.getStatus());

	QList<QPair<Player, PlayerReplayData>> players = rep.getPlayers();	
	this->allies->clear();
	this->axis->clear();
	for (const auto& playerPair : players) {
		const PlayerReplayData& playerData = playerPair.second;

		if (playerData.getTeam() == 1) {
			this->allies->append(playerPair);
		}
		else if (playerData.getTeam() == 2) {
			this->axis->append(playerPair);
		}
	}

	std::sort(this->allies->begin(), this->allies->end(), [](const QPair<Player, PlayerReplayData>& p1, const QPair<Player, PlayerReplayData>& p2) {
		return p1.second.getScore() > p2.second.getScore();
		});

	std::sort(this->axis->begin(), this->axis->end(), [](const QPair<Player, PlayerReplayData>& p1, const QPair<Player, PlayerReplayData>& p2) {
		return p1.second.getScore() > p2.second.getScore();
		});

	ui->alliesTable->clear();
	ui->axisTable->clear();

	populateTeamTable(ui->alliesTable, allies, true);
	populateTeamTable(ui->axisTable, axis, false);
}



void MainWindow::populateTeamTable(QTableWidget* table, const QList<QPair<Player, PlayerReplayData>>* players, bool allies)
{
	QPalette palette = qApp->palette();
	bool isDarkTheme = palette.color(QPalette::Window).lightness() < 128;

	table->clear();
	table->setRowCount(players->size());
	table->setColumnCount(11);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);

	QPixmap scorePixmap(":/icons/score.png");
	QPixmap killsPixmap(":/icons/kills.png");
	QPixmap groundKillsPixmap(":/icons/groundKills.png");
	QPixmap navalKillsPixmap(":/icons/navalKills.png");
	QPixmap assistsPixmap(":/icons/assists.png");
	QPixmap capturedZonesPixmap(":/icons/capturedZones.png");
	QPixmap aiKillsPixmap(":/icons/aiKills.png");
	QPixmap awardDamagePixmap(":/icons/awardDamage.png");
	QPixmap damageZonePixmap(":/icons/damageZone.png");
	QPixmap deathsPixmap(":/icons/deaths.png");

	if (isDarkTheme) {
		scorePixmap = Utils::invertIconColors(QIcon(scorePixmap)).pixmap(32, 32);
		killsPixmap = Utils::invertIconColors(QIcon(killsPixmap)).pixmap(32, 32);
		groundKillsPixmap = Utils::invertIconColors(QIcon(groundKillsPixmap)).pixmap(32, 32);
		navalKillsPixmap = Utils::invertIconColors(QIcon(navalKillsPixmap)).pixmap(32, 32);
		assistsPixmap = Utils::invertIconColors(QIcon(assistsPixmap)).pixmap(32, 32);
		capturedZonesPixmap = Utils::invertIconColors(QIcon(capturedZonesPixmap)).pixmap(32, 32);
		aiKillsPixmap = Utils::invertIconColors(QIcon(aiKillsPixmap)).pixmap(32, 32);
		awardDamagePixmap = Utils::invertIconColors(QIcon(awardDamagePixmap)).pixmap(32, 32);
		damageZonePixmap = Utils::invertIconColors(QIcon(damageZonePixmap)).pixmap(32, 32);
		deathsPixmap = Utils::invertIconColors(QIcon(deathsPixmap)).pixmap(32, 32);
	}

	table->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Username")));
	table->setHorizontalHeaderItem(1, new QTableWidgetItem(QIcon(scorePixmap), ""));
	table->setHorizontalHeaderItem(2, new QTableWidgetItem(QIcon(killsPixmap), ""));
	table->setHorizontalHeaderItem(3, new QTableWidgetItem(QIcon(groundKillsPixmap), ""));
	table->setHorizontalHeaderItem(4, new QTableWidgetItem(QIcon(navalKillsPixmap), ""));
	table->setHorizontalHeaderItem(5, new QTableWidgetItem(QIcon(assistsPixmap), ""));
	table->setHorizontalHeaderItem(6, new QTableWidgetItem(QIcon(capturedZonesPixmap), ""));
	table->setHorizontalHeaderItem(7, new QTableWidgetItem(QIcon(aiKillsPixmap), ""));
	table->setHorizontalHeaderItem(8, new QTableWidgetItem(QIcon(awardDamagePixmap), ""));
	table->setHorizontalHeaderItem(9, new QTableWidgetItem(QIcon(damageZonePixmap), ""));
	table->setHorizontalHeaderItem(10, new QTableWidgetItem(QIcon(deathsPixmap), ""));

	int row = 0;
	for (int i = 0; i < players->size(); i++) {
		const Player& player = players->at(i).first;
		const PlayerReplayData& prd = players->at(i).second;

		QTableWidgetItem* item = new QTableWidgetItem();
		if (player.getUsername().contains("@psn")) {
			item->setIcon(QIcon(":/icons/psn.png"));
		}
		else if (player.getUsername().contains("@live")) {
			item->setIcon(QIcon(":/icons/xbox.png"));
		}
		else {
			item->setIcon(QIcon(":/icons/pc.png"));
		}

		if (isDarkTheme) {
			item->setIcon(Utils::invertIconColors(item->icon()));
		}

		item->setText(player.getSquadronTag() + " " + player.getUsername().replace("@psn", "").replace("@live", ""));
		table->setItem(row, 0, item);

		table->setItem(row, 1, new QTableWidgetItem(QString::number(prd.getScore())));
		table->item(row, 1)->setToolTip(tr("Score"));

		QTableWidgetItem* killsItem = new QTableWidgetItem(QString::number(prd.getKills()));
		killsItem->setToolTip(tr("Air kills"));
		table->setItem(row, 2, killsItem);

		QTableWidgetItem* groundKillsItem = new QTableWidgetItem(QString::number(prd.getGroundKills()));
		groundKillsItem->setToolTip(tr("Ground kills"));
		table->setItem(row, 3, groundKillsItem);

		QTableWidgetItem* navalKillsItem = new QTableWidgetItem(QString::number(prd.getNavalKills()));
		navalKillsItem->setToolTip(tr("Naval kills"));
		table->setItem(row, 4, navalKillsItem);

		QTableWidgetItem* assistsItem = new QTableWidgetItem(QString::number(prd.getAssists()));
		assistsItem->setToolTip(tr("Assists"));
		table->setItem(row, 5, assistsItem);

		QTableWidgetItem* captureZoneItem = new QTableWidgetItem(QString::number(prd.getCaptureZone()));
		captureZoneItem->setToolTip(tr("Captured zones"));
		table->setItem(row, 6, captureZoneItem);

		QTableWidgetItem* aiKillsItem = new QTableWidgetItem(QString::number(prd.getAiKills() + prd.getAiGroundKills() + prd.getAiNavalKills()));
		aiKillsItem->setToolTip(tr("AI kills"));
		table->setItem(row, 7, aiKillsItem);

		QTableWidgetItem* awardDamageItem = new QTableWidgetItem(QString::number(prd.getAwardDamage()));
		awardDamageItem->setToolTip(tr("Awarded damage"));
		table->setItem(row, 8, awardDamageItem);

		QTableWidgetItem* damageZoneItem = new QTableWidgetItem(QString::number(prd.getDamageZone()));
		damageZoneItem->setToolTip(tr("Bombing damage"));
		table->setItem(row, 9, damageZoneItem);

		QTableWidgetItem* deathsItem = new QTableWidgetItem(QString::number(prd.getDeaths()));
		deathsItem->setToolTip(tr("Deaths"));
		table->setItem(row, 10, deathsItem);

		row++;
	}
	static bool alliesTableIsConnected = false;
	static bool axisTableIsConnected = false;
	if (!alliesTableIsConnected && allies) {
		connect(ui->alliesTable, &QTableWidget::itemDoubleClicked, [this](QTableWidgetItem* item) {
			int row = item->row();

			if (row >= 0 && row < this->allies->size()) {
				PlayerProfileDialog dialog(this);
				dialog.setPlayerData(this->allies->at(row));
				dialog.exec();
			}
			});
		alliesTableIsConnected = true;
	}
	if (!axisTableIsConnected && !allies) {
		connect(ui->axisTable, &QTableWidget::itemDoubleClicked, [this](QTableWidgetItem* item) {
			int row = item->row();

			if (row >= 0 && row < this->axis->size()) {
				PlayerProfileDialog dialog(this);
				dialog.setPlayerData(this->axis->at(row));
				dialog.exec();
			}
			});
		axisTableIsConnected = true;
	}
	table->resizeColumnsToContents();
}

void MainWindow::onLanguageChanged(const QString& languageCode)
{
	changeLanguage(languageCode);
	qDebug() << "Language changed to:" << languageCode;
}

void MainWindow::changeLanguage(const QString& languageCode)
{
	qApp->removeTranslator(appTranslator);

	QString translationFile = QCoreApplication::applicationDirPath() + QString("/wtplotter_%1.qm").arg(languageCode);
	if (appTranslator->load(translationFile)) {
		qApp->installTranslator(appTranslator);
		qDebug() << "Loaded translation file:" << translationFile;
	}
	else {
		qWarning() << "Failed to load translation file:" << translationFile;
	}
	ui->retranslateUi(this);
}