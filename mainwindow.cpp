﻿#include "mainwindow.h"
#include "classes/utils.h"
#include "./ui_mainwindow.h"
#include "classes/replay.h"
#include "sceneimageviewer.h"
#include <qfontdatabase.h>
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

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::MainWindow),
	model(new QStandardItemModel(this)),
	m_thread(nullptr),
	m_worker(nullptr),
	m_dbmanager(QString(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/wtplotter/replays.sqlite3"))
{
	ui->setupUi(this);
	QSettings settings("sgambe33", "wtplotter");

	emit refreshReplays();

	ui->splitter->setStretchFactor(0, 2);
	ui->splitter->setStretchFactor(1, 3);

	QPixmap img(":/map_images/unknownmap.png");
	img = img.scaled(125, 125, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	ui->mapImage->setPixmap(img);

	QStatusBar* statusBar = ui->statusbar;
	QWidget* buttonBox = new QWidget(this);
	QHBoxLayout* layout = new QHBoxLayout(buttonBox);
	layout->setContentsMargins(0, 0, 0, 0);

	QPushButton* replayButton = new QPushButton("Replays", buttonBox);
	QPushButton* plotterButton = new QPushButton("Plotter", buttonBox);

	connect(replayButton, &QPushButton::clicked, [=] {
		ui->stackedWidget_2->setCurrentIndex(0);
		ui->replayTreeView->setDisabled(false);
		plotterButton->setDisabled(false);
		replayButton->setDisabled(true);
		stopPlotter();
		});

	connect(plotterButton, &QPushButton::clicked, [=] {
		ui->stackedWidget_2->setCurrentIndex(2);
		ui->replayTreeView->setDisabled(true);
		plotterButton->setDisabled(true);
		replayButton->setDisabled(false);
		startPlotter();
		});

	layout->addWidget(replayButton);
	layout->addWidget(plotterButton);

	statusBar->addPermanentWidget(buttonBox);

	ui->replayTreeView->setDisabled(true);
	plotterButton->setDisabled(true);

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
	if (dialog.exec()) {
		refreshReplays();
	}
}

void MainWindow::openAboutDialog()
{
	QString aboutText = R"(
        <p>WT Plotter is a tool for reading War Thunder replays and record match development. This 
        project is developed by <strong>Sgambe33</strong> and is fully open source.You can find the 
        source code and contribute to the project on <a href='https://github.com/sgambe33/wt-plotter'>
        GitHub</a>.</p>
        <p>Thank you for using WT Plotter!</p>
    )";
	QMessageBox::about(this, "About WT Plotter", aboutText);
}

void MainWindow::setCustomFont(const QString& fontPath, QWidget* widget) {
	int id = QFontDatabase::addApplicationFont(fontPath);
	if (id != -1) {
		QString family = QFontDatabase::applicationFontFamilies(id).at(0);
		QFont customFont(family);
		widget->setFont(customFont);
		for (auto child : widget->findChildren<QWidget*>()) {
			child->setFont(customFont);
		}
	}
	else {
		qDebug() << "Failed to load font from" << fontPath;
	}
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
		populateReplayTreeView(ui->replayTreeView, settings.value("replayFolderPath").toString());
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
	model->clear();
	model->setHorizontalHeaderLabels({ tr("File Name") });

	QMap<QDate, QList<QFileInfo>> filesByDate;

	QDir dir(directoryPath);
	dir.setFilter(QDir::Files | QDir::NoSymLinks);
	dir.setNameFilters({ "*.wrpl" });
	QFileInfoList fileInfoList = dir.entryInfoList();

	for (const QFileInfo& fileInfo : fileInfoList)
	{
		QDate creationDate = fileInfo.birthTime().date();
		filesByDate[creationDate].append(fileInfo);
	}

	for (auto it = filesByDate.cbegin(); it != filesByDate.cend(); ++it)
	{
		QStandardItem* dateItem = new QStandardItem(it.key().toString("yyyy-MM-dd"));
		dateItem->setFlags(dateItem->flags() & ~Qt::ItemIsEditable);
		for (const QFileInfo& fileInfo : it.value())
		{
			QStandardItem* fileNameItem = new QStandardItem(fileInfo.fileName());
			fileNameItem->setData(fileInfo.absoluteFilePath(), Qt::UserRole);
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
	QString filePath = index.data(Qt::UserRole).toString();

	if (!filePath.isEmpty())
	{
		executeCommand(filePath);
	}
}

void MainWindow::executeCommand(const QString& filePath)
{
	Replay rep = Replay::fromFile(filePath);

	QPixmap mapPixmap(":/map_images/" + rep.getLevel() + "_tankmap_thumb.png");
	if (mapPixmap.isNull()) {
		mapPixmap = QPixmap(":/map_images/unknownmap.png");
	}
	ui->mapImage->setPixmap(mapPixmap);

	ui->mapNameLabel->setText(QString("Map: ") + rep.getLevel());

	QDateTime startTime = QDateTime::fromSecsSinceEpoch(rep.getStartTime());
	QString formattedStartTime = startTime.toString("hh:mm:ss");
	ui->startTimeLabel->setText(QString("Start time: ") + formattedStartTime);
	ui->timePlayedLabel->setText(QString("Time played: ") + Utils::replayLengthToString(rep.getTimePlayed()));
	ui->resultLabel->setText(QString("Result: ") + rep.getStatus());

	QList<Player> players = rep.getPlayers();

	QList<Player> allies;
	QList<Player> axis;
	for (const Player& player : players)
	{
		if (player.getTeam() == 1)
		{
			allies.append(player);
		}
		else if (player.getTeam() == 2)
		{
			axis.append(player);
		}
	}

	std::sort(allies.begin(), allies.end(), [](const Player& p1, const Player& p2)
		{
			return p1.getScore() > p2.getScore();
		});
	std::sort(axis.begin(), axis.end(), [](const Player& p1, const Player& p2)
		{
			return p1.getScore() > p2.getScore();
		});

	populateTeamTable(ui->alliesTable, allies);
	populateTeamTable(ui->axisTable, axis);

	m_dbmanager.insertReplay(rep);
}

void MainWindow::populateTeamTable(QTableWidget* table, const QList<Player>& players)
{
	table->clear();
	table->setRowCount(players.size());
	table->setColumnCount(10);

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

	table->setHorizontalHeaderItem(0, new QTableWidgetItem(QString("Name")));
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

	for (int i = 0; i < players.size(); ++i)
	{
		const Player& player = players[i];
		table->setItem(i, 0, new QTableWidgetItem(player.getClanTag() + " " + player.getName()));

		QTableWidgetItem* scoreItem = new QTableWidgetItem(QString::number(player.getScore()));
		scoreItem->setToolTip("Score");
		table->setItem(i, 1, scoreItem);

		QTableWidgetItem* killsItem = new QTableWidgetItem(QString::number(player.getKills()));
		killsItem->setToolTip("Air kills");
		table->setItem(i, 2, killsItem);

		QTableWidgetItem* groundKillsItem = new QTableWidgetItem(QString::number(player.getGroundKills()));
		groundKillsItem->setToolTip("Ground kills");
		table->setItem(i, 3, groundKillsItem);

		QTableWidgetItem* navalKillsItem = new QTableWidgetItem(QString::number(player.getNavalKills()));
		navalKillsItem->setToolTip("Naval kills");
		table->setItem(i, 4, navalKillsItem);

		QTableWidgetItem* assistsItem = new QTableWidgetItem(QString::number(player.getAssists()));
		assistsItem->setToolTip("Assists");
		table->setItem(i, 5, assistsItem);

		QTableWidgetItem* captureZoneItem = new QTableWidgetItem(QString::number(player.getCaptureZone()));
		captureZoneItem->setToolTip("Captured zones");
		table->setItem(i, 6, captureZoneItem);

		QTableWidgetItem* aiKillsItem = new QTableWidgetItem(QString::number(player.getAiKills() + player.getAiGroundKills() + player.getAiNavalKills()));
		aiKillsItem->setToolTip("AI kills");
		table->setItem(i, 7, aiKillsItem);

		QTableWidgetItem* awardDamageItem = new QTableWidgetItem(QString::number(player.getAwardDamage()));
		awardDamageItem->setToolTip("Awarded damage");
		table->setItem(i, 8, awardDamageItem);

		QTableWidgetItem* damageZoneItem = new QTableWidgetItem(QString::number(player.getDamageZone()));
		damageZoneItem->setToolTip("Bombing damage");
		table->setItem(i, 9, damageZoneItem);

		QTableWidgetItem* deathsItem = new QTableWidgetItem(QString::number(player.getDeaths()));
		deathsItem->setToolTip("Deaths");
		table->setItem(i, 10, deathsItem);
	}

	table->resizeColumnsToContents();
}