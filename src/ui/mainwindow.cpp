#include "mainwindow.h"
#include "../classes/utils.h"
#include "./ui_mainwindow.h"
#include "sceneimageviewer.h"
#include "../GameTelemetryWorker.h"
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
#include "../classes/replayloaderworker.h"
#include <QStandardPaths>
#include <QDesktopServices>
#include <QPalette>
#include "playerprofiledialog.h"
#include "../version.h"
#include "../classes/logger.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      model(new QStandardItemModel(this)),
      m_discord_thread(nullptr),
      m_discord_worker(new DiscordWorker(this)),
      m_dbmanager(QString(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/wtplotter/replays.sqlite3"), "mainwindow"),
      appTranslator(new QTranslator(this)),
      settings("sgambe33", "wtplotter") {
    ui->setupUi(this);
    const int id = QFontDatabase::addApplicationFont(":/fonts/wt_symbols.ttf");
    wtSymbols = QFont(QFontDatabase::applicationFontFamilies(id).at(0));

    if (!Logger::instance().init("application.log")) {
        qWarning("Could not initialize log file!");
    }

    LOG_INFO("Application started");

    ui->splitter->setStretchFactor(0, 2);
    ui->splitter->setStretchFactor(1, 3);

    QPixmap img(":/map_images/unknownmap.png");
    img = img.scaled(125, 125, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->mapImage->setPixmap(img);

    connect(ui->refreshButton, &QPushButton::clicked, [=] {
        emit refreshReplays();
    });

    connect(ui->expandButton, &QPushButton::clicked, [=] {
        ui->replayTreeView->expandAll();
    });

    connect(ui->collapseButton, &QPushButton::clicked, [=] {
        ui->replayTreeView->collapseAll();
    });

    ui->openServerReplayButton->setDisabled(true);

    checkAppVersion();

    startDiscordPresence();
    connect(ui->replayTreeView, &QTreeView::clicked, this, &MainWindow::onTreeItemClicked);
    connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::openPreferencesDialog);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::openAboutDialog);
    connect(ui->actionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(this, &MainWindow::sendActivityToDiscord, m_discord_worker, &DiscordWorker::updateActivity, Qt::QueuedConnection);
    loadReplaysFromFolder();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::openPreferencesDialog() {
    PreferencesDialog dialog(this);
    connect(&dialog, &PreferencesDialog::languageChanged, this, &MainWindow::onLanguageChanged);
    if (dialog.exec()) {
        refreshReplays();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QApplication::quit();
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            this->hide();
            event->ignore();
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::openAboutDialog() {
    QString aboutText = tr(
        R"(<p>Current version: %1</p><p>WT Plotter is a tool for reading War Thunder replays and record match development. This project is developed by <strong>Sgambe33</strong> and is fully open source.You can find the source code and contribute to the project on <a href='https://github.com/sgambe33/wt-plotter'> GitHub</a>.</p> <p>Thank you for using WT Plotter!</p>)");

    aboutText = aboutText.arg(QString("%1.%2.%3")
        .arg(APP_VERSION_MAJOR)
        .arg(APP_VERSION_MINOR)
        .arg(APP_VERSION_PATCH));

    QMessageBox::about(this, tr("About WT Plotter"), aboutText);
}

void MainWindow::startDiscordPresence() {
    m_discord_thread = new QThread();
    m_discord_worker = new DiscordWorker(this);

    connect(m_discord_thread, &QThread::started, m_discord_worker, &DiscordWorker::start);
    connect(qApp, &QCoreApplication::aboutToQuit, m_discord_worker, &DiscordWorker::stop);
    connect(m_discord_thread, &QThread::finished, m_discord_worker, &QObject::deleteLater);

    m_discord_worker->moveToThread(m_discord_thread);
    m_discord_thread->start();
}

void MainWindow::setActivityFromMainWindow(const QString &state, const QString &details, const QString &logo, time_t epochStartTime, const QString &largeText) {
    emit sendActivityToDiscord(state, details, logo, epochStartTime, largeText);
}

void MainWindow::updatePixmap(const QPixmap &pixmap) {
    ui->mappa->setPixmap(pixmap);
}

void MainWindow::refreshReplays() {
    if (!settings.value("replayFolderPath").isNull()) {
        LOG_INFO("Refreshing replay list");
        loadReplaysFromFolder();
    }
}

void MainWindow::loadReplaysFromFolder() {
    const QString folderPath = settings.value("replayFolderPath").toString();
    if (folderPath.isEmpty())
        return;

    ui->stackedWidget_1->setCurrentIndex(1);
    ui->replayLoadingProgressBar->setRange(0, 100);
    ui->replayLoadingProgressBar->setValue(0);
    ui->replayLoadingProgressBar->setTextVisible(true);

    QThread *thread = new QThread();
    ReplayLoaderWorker *worker = new ReplayLoaderWorker(folderPath, QString(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/wtplotter/replays.sqlite3"));
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &ReplayLoaderWorker::loadReplays);
    connect(worker, &ReplayLoaderWorker::progressUpdated, ui->replayLoadingProgressBar, &QProgressBar::setValue);
    connect(worker, &ReplayLoaderWorker::finished, this, &MainWindow::onReplayLoaderFinished);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void MainWindow::onReplayLoaderFinished() {
    ui->stackedWidget_1->setCurrentIndex(0);
    populateReplayTreeView(ui->replayTreeView);
}

void MainWindow::changeStackedWidget1(int index) {
    ui->stackedWidget_1->setCurrentIndex(index);
}

void MainWindow::changeStackedWidget2(int index) {
    ui->stackedWidget_2->setCurrentIndex(index);
}

void MainWindow::updateProgressBar(double progress) {
}

void MainWindow::updateStatusLabel(QString msg) {
}

void MainWindow::populateReplayTreeView(QTreeView *replayTreeView) const {
    const QString languageCode = settings.value("language", "en").toString();
    model->clear();
    model->setHorizontalHeaderLabels({tr("File Name")});

    const QMap<QDate, QList<wrpl::Replay> > replaysByDate = m_dbmanager.fetchReplaysGroupedByDate();
    for (auto it = replaysByDate.cbegin(); it != replaysByDate.cend(); ++it) {
        auto dateItem = std::make_unique<QStandardItem>(it.key().toString("yyyy-MM-dd"));
        dateItem->setFlags(dateItem->flags() & ~Qt::ItemIsEditable);
        for (const wrpl::Replay &replay: it.value()) {
            QJsonObject obj;
            if (QString::fromStdString(replay.header.rawLevel).endsWith("_snow")) {
                obj = getJsonFromResources(":/translations/locations.json", QString::fromStdString(replay.header.rawLevel).replace("_snow", ""));
            } else {
                obj = getJsonFromResources(":/translations/locations.json", QString::fromStdString(replay.header.rawLevel));
            }

            auto fileNameItem = std::make_unique<QStandardItem>(
                epochSToFormattedTime(replay.header.startTimeEpochS) + " - " + obj.value(languageCode).toString(QString::fromStdString(replay.header.rawLevel))
            );
            fileNameItem->setData(QString::fromStdString(replay.header.sessionId), Qt::UserRole);
            fileNameItem->setFlags(fileNameItem->flags() & ~Qt::ItemIsEditable);
            dateItem->appendRow(fileNameItem.release());
        }
        model->appendRow(dateItem.release());
    }

    replayTreeView->setModel(model);
    replayTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void MainWindow::onTreeItemClicked(const QModelIndex &index) {
    const QString sessionId = index.data(Qt::UserRole).toString();
    if (!sessionId.isEmpty()) {
        executeCommand(sessionId);
    }
}

void MainWindow::executeCommand(const QString &sessionId) {
    const QString languageCode = settings.value("language", "en").toString();
    wrpl::Replay rep = m_dbmanager.getReplayBySessionId(sessionId);

    disconnect(ui->openServerReplayButton, &QPushButton::clicked, nullptr, nullptr);

    connect(ui->openServerReplayButton, &QPushButton::clicked, [=] {
        const QString url = "https://warthunder.com/en/tournament/replay/" + QString::fromStdString(rep.header.sessionId);
        QDesktopServices::openUrl(QUrl(url));
    });

    ui->openServerReplayButton->setDisabled(false);

    QPixmap mapPixmap(":/map_images/unknownmap.png");
    if (QString::fromStdString(rep.header.rawLevel).contains("avg")) {
        mapPixmap = ":/map_images/" + QString::fromStdString(rep.header.rawLevel) + "_tankmap_thumb.png";
    } else {
        mapPixmap = ":/map_images/" + QString::fromStdString(rep.header.rawLevel) + "_map_thumb.png";
    }
    if (mapPixmap.isNull()) {
        mapPixmap = QPixmap(":/map_images/unknownmap.png");
    }

    ui->mapImage->setPixmap(mapPixmap);
    QJsonObject obj;
    if (QString::fromStdString(rep.header.rawLevel).endsWith("_snow")) {
        obj = getJsonFromResources(":/translations/locations.json", QString::fromStdString(rep.header.rawLevel).replace("_snow", ""));
    } else {
        obj = getJsonFromResources(":/translations/locations.json", QString::fromStdString(rep.header.rawLevel));
    }

    ui->sessionIdLabel->setText(tr("Session ID: ") + QString::fromStdString(rep.header.sessionId));
    ui->mapNameLabel->setText(tr("Map: ") + obj.value(languageCode).toString(QString::fromStdString(rep.header.rawLevel)));
    ui->difficultyLabel->setText(tr("Difficulty: ") + difficultyToStringLocaleAware(static_cast<Constants::Difficulty>(rep.header.difficulty)));
    ui->startTimeLabel->setText(tr("Start time: ") + epochSToFormattedTime(rep.header.startTimeEpochS));
    //ui->timePlayedLabel->setText(tr("Time played: ") + replayLengthToString(rep.getTimePlayed()));
    //ui->resultLabel->setText(tr("Result: ") + tr(rep.getStatus().toStdString().c_str()));

    //QList<QPair<Player, PlayerReplayData>> players = rep.getPlayers();
    //this->alliesList->clear();
    //this->axisList->clear();
    //for (const auto& playerPair : players) {
    //	const PlayerReplayData& playerData = playerPair.second;
    //
    //	if (playerData.getTeam() == 1) {
    //        this->alliesList->append(playerPair);
    //	}
    //	else if (playerData.getTeam() == 2) {
    //        this->axisList->append(playerPair);
    //	}
    //}
    //
    //std::sort(this->alliesList->begin(), this->alliesList->end(), [](const QPair<Player, PlayerReplayData>& p1, const QPair<Player, PlayerReplayData>& p2) {
    //	return p1.second.getScore() > p2.second.getScore();
    //	});
    //
    //std::sort(this->axisList->begin(), this->axisList->end(), [](const QPair<Player, PlayerReplayData>& p1, const QPair<Player, PlayerReplayData>& p2) {
    //	return p1.second.getScore() > p2.second.getScore();
    //	});
    //
    //ui->alliesTable->clear();
    //ui->axisTable->clear();
    //
    //populateTeamTable(ui->alliesTable, alliesList, true);
    //populateTeamTable(ui->axisTable, axisList, false);
}

//void MainWindow::populateTeamTable(QTableWidget *table, const QList<QPair<Player, PlayerReplayData> > *players, bool allies) {
//    QPalette palette = qApp->palette();
//    bool isDarkTheme = palette.color(QPalette::Window).lightness() < 128;
//
//    table->clear();
//    table->setRowCount(players->size());
//    table->setColumnCount(11);
//    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
//
//    auto loadIcon = [isDarkTheme](const QString &path) {
//        QPixmap pixmap(path);
//        if (isDarkTheme) {
//            return invertIconColors(QIcon(pixmap)).pixmap(32, 32);
//        }
//        return pixmap;
//    };
//
//    QPixmap scorePixmap = loadIcon(":/icons/score.png");
//    QPixmap killsPixmap = loadIcon(":/icons/kills.png");
//    QPixmap groundKillsPixmap = loadIcon(":/icons/groundKills.png");
//    QPixmap navalKillsPixmap = loadIcon(":/icons/navalKills.png");
//    QPixmap assistsPixmap = loadIcon(":/icons/assists.png");
//    QPixmap capturedZonesPixmap = loadIcon(":/icons/capturedZones.png");
//    QPixmap aiKillsPixmap = loadIcon(":/icons/aiKills.png");
//    QPixmap awardDamagePixmap = loadIcon(":/icons/awardDamage.png");
//    QPixmap damageZonePixmap = loadIcon(":/icons/damageZone.png");
//    QPixmap deathsPixmap = loadIcon(":/icons/deaths.png");
//
//    table->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Username")));
//    table->setHorizontalHeaderItem(1, new QTableWidgetItem(QIcon(scorePixmap), ""));
//    table->setHorizontalHeaderItem(2, new QTableWidgetItem(QIcon(killsPixmap), ""));
//    table->setHorizontalHeaderItem(3, new QTableWidgetItem(QIcon(groundKillsPixmap), ""));
//    table->setHorizontalHeaderItem(4, new QTableWidgetItem(QIcon(navalKillsPixmap), ""));
//    table->setHorizontalHeaderItem(5, new QTableWidgetItem(QIcon(assistsPixmap), ""));
//    table->setHorizontalHeaderItem(6, new QTableWidgetItem(QIcon(capturedZonesPixmap), ""));
//    table->setHorizontalHeaderItem(7, new QTableWidgetItem(QIcon(aiKillsPixmap), ""));
//    table->setHorizontalHeaderItem(8, new QTableWidgetItem(QIcon(awardDamagePixmap), ""));
//    table->setHorizontalHeaderItem(9, new QTableWidgetItem(QIcon(damageZonePixmap), ""));
//    table->setHorizontalHeaderItem(10, new QTableWidgetItem(QIcon(deathsPixmap), ""));
//
//    for (int row = 0; row < players->size(); ++row) {
//        const Player &player = players->at(row).first;
//        const PlayerReplayData &prd = players->at(row).second;
//
//        QTableWidgetItem *usernameItem = new QTableWidgetItem();
//        QString platformIconPath = player.getUsername().contains("@psn") ? ":/icons/psn.png" : player.getUsername().contains("@live") ? ":/icons/xbox.png" : ":/icons/pc.png";
//        usernameItem->setIcon(QIcon(platformIconPath));
//        if (isDarkTheme) {
//            usernameItem->setIcon(invertIconColors(usernameItem->icon()));
//        }
//        usernameItem->setFont(wtSymbols);
//        usernameItem->setText(player.getSquadronTag() + " " + player.getUsername().replace("@psn", "").replace("@live", ""));
//        table->setItem(row, 0, usernameItem);
//
//        auto createItem = [](const QString &text, const QString &tooltip) {
//            QTableWidgetItem *item = new QTableWidgetItem(text);
//            item->setToolTip(tooltip);
//            return item;
//        };
//
//        table->setItem(row, 1, createItem(QString::number(prd.getScore()), tr("Score")));
//        table->setItem(row, 2, createItem(QString::number(prd.getKills()), tr("Air kills")));
//        table->setItem(row, 3, createItem(QString::number(prd.getGroundKills()), tr("Ground kills")));
//        table->setItem(row, 4, createItem(QString::number(prd.getNavalKills()), tr("Naval kills")));
//        table->setItem(row, 5, createItem(QString::number(prd.getAssists()), tr("Assists")));
//        table->setItem(row, 6, createItem(QString::number(prd.getCaptureZone()), tr("Captured zones")));
//        table->setItem(row, 7, createItem(QString::number(prd.getAiKills() + prd.getAiGroundKills() + prd.getAiNavalKills()), tr("AI kills")));
//        table->setItem(row, 8, createItem(QString::number(prd.getAwardDamage()), tr("Awarded damage")));
//        table->setItem(row, 9, createItem(QString::number(prd.getDamageZone()), tr("Bombing damage")));
//        table->setItem(row, 10, createItem(QString::number(prd.getDeaths()), tr("Deaths")));
//    }
//
//    static bool alliesTableIsConnected = false;
//    static bool axisTableIsConnected = false;
//
//    if (!alliesTableIsConnected && allies) {
//        connect(ui->alliesTable, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
//            int row = item->row();
//            if (row >= 0 && row < this->alliesList->size()) {
//                PlayerProfileDialog dialog(this);
//                dialog.setPlayerData(this->alliesList->at(row));
//                dialog.exec();
//            }
//        });
//        alliesTableIsConnected = true;
//    }
//
//    if (!axisTableIsConnected && !allies) {
//        connect(ui->axisTable, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
//            int row = item->row();
//            if (row >= 0 && row < this->axisList->size()) {
//                PlayerProfileDialog dialog(this);
//                dialog.setPlayerData(this->axisList->at(row));
//                dialog.exec();
//            }
//        });
//        axisTableIsConnected = true;
//    }
//
//    table->resizeColumnsToContents();
//}

void MainWindow::onLanguageChanged(const QString &languageCode) {
    qApp->removeTranslator(appTranslator);
    QString translationFile;

    const QDir translationsDir(QCoreApplication::applicationDirPath() + "/translations");
    if (translationsDir.exists()) {
        translationFile = translationsDir.filePath(QString("wtplotter_%1.qm").arg(languageCode));
    } else {
        translationFile = QCoreApplication::applicationDirPath() + QString("/wtplotter_%1.qm").arg(languageCode);
    }
    if (appTranslator->load(translationFile)) {
        qApp->installTranslator(appTranslator);
    } else {
        qWarning() << "Failed to load translation file:" << translationFile;
    }
    ui->retranslateUi(this);
}
