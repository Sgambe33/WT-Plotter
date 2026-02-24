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
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QThread>
#include <QPointer>
#include <QStandardItem>
#include <QPixmap>
#include <QIcon>
#include <QGraphicsPixmapItem>
#include <QLabel>
#include <QDebug>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QFile>
#include "../classes/replayloaderworker.h"
#include <QStandardPaths>
#include <QPalette>
#include <QSignalBlocker>
#include <algorithm>
#include <limits>
#include "playerprofiledialog.h"
#include "../version.h"
#include "../classes/logger.h"
#include "libs/WRPL_parser/include/wrpl.h"

namespace {
    QString normalizeLevelIdentifier(const QString &rawLevel) {
        return rawLevel.endsWith("_snow") ? rawLevel.left(rawLevel.size() - 5) : rawLevel;
    }

    QString resolveMapDisplayName(const QString &rawLevel, const QString &languageCode) {
        const QString normalizedLevel = normalizeLevelIdentifier(rawLevel);
        const QJsonObject obj = getJsonFromResources(":/translations/locations.json", normalizedLevel);
        return obj.value(languageCode).toString(rawLevel);
    }

    struct ServerImportResult {
        bool success = false;
        QString message;
        QString sessionId;
    };

    ServerImportResult downloadAndImportServerReplay(const QString &sessionId, const QString &dbPath) {
        if (sessionId.isEmpty()) {
            return {false, "Session ID is empty.", sessionId};
        }

        QTemporaryDir tempDir;
        if (!tempDir.isValid()) {
            return {false, "Failed to create temporary directory for replay download.", sessionId};
        }

        QNetworkAccessManager networkManager;
        std::vector<std::vector<uint8_t> > partsBytes;
        partsBytes.reserve(16);
        const QString baseUrl = QString("https://wt-game-replays.warthunder.com/0%1/").arg(sessionId);
        constexpr int maxParts = 256;
        for (int partIndex = 0; partIndex < maxParts; ++partIndex) {
            const QString partName = QString("%1.wrpl").arg(partIndex, 4, 10, QLatin1Char('0'));
            const QUrl url(baseUrl + partName);

            QNetworkReply *reply = networkManager.get(QNetworkRequest(url));
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            const bool hasError = reply->error() != QNetworkReply::NoError;
            const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QByteArray payload = reply->readAll();
            reply->deleteLater();

            if (hasError || httpCode != 200 || payload.isEmpty()) {
                if (partIndex == 0) {
                    return {false, QString("Failed to download first server replay part from %1").arg(url.toString()), sessionId};
                }
                break;
            }

            QFile partFile(tempDir.filePath(partName));
            if (!partFile.open(QIODevice::WriteOnly)) {
                return {false, QString("Failed to write replay part to temp path: %1").arg(partFile.fileName()), sessionId};
            }
            partFile.write(payload);
            partFile.close();

            partsBytes.emplace_back(payload.begin(), payload.end());
        }

        if (partsBytes.empty()) {
            return {false, "No server replay parts were downloaded.", sessionId};
        }

        wrpl::Replay replay;
        try {
            replay = wrpl::parseServerReplay(partsBytes);
        } catch (const std::exception &e) {
            return {false, QString("Failed to parse downloaded server replay: %1").arg(e.what()), sessionId};
        }
        LOG_INFO_GLOBAL("Completed download and parsing of server replay with session ID %1. Parsed replay has %2 movement packets.");

        const QString parsedSessionId = QString::fromStdString(replay.header.sessionId);
        if (parsedSessionId.compare(sessionId, Qt::CaseInsensitive) != 0) {
            return {false, QString("Session mismatch after parsing. Requested: %1, parsed: %2").arg(sessionId, parsedSessionId), sessionId};
        }

        const QString connectionName = QString("serverreplay_import_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
        DbManager db(dbPath, connectionName);
        db.createTables();

        if (!db.deleteReplayBySessionId(sessionId)) {
            return {false, QString("Failed deleting existing replay in DB for session %1").arg(sessionId), sessionId};
        }
        if (!db.insertReplay(replay)) {
            return {false, QString("Failed inserting parsed replay into DB for session %1").arg(sessionId), sessionId};
        }

        return {true, QString("Server replay imported (%1 parts).").arg(partsBytes.size()), sessionId};
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      model(new QStandardItemModel(this)),
      m_discord_thread(nullptr),
      m_discord_worker(nullptr),
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
    setupReplayControls();
    setupReplayEventTabs();
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
            const QString rawLevel = QString::fromStdString(replay.header.rawLevel);
            const QString mapName = resolveMapDisplayName(rawLevel, languageCode);

            auto fileNameItem = std::make_unique<QStandardItem>(
                epochSToFormattedTime(replay.header.startTimeEpochS) + " - " + mapName
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
    m_selectedReplay = rep;
    const QString rawLevel = QString::fromStdString(rep.header.rawLevel);

    disconnect(ui->openServerReplayButton, &QPushButton::clicked, nullptr, nullptr);
    connect(ui->openServerReplayButton, &QPushButton::clicked, this, [this, sessionId] {
        importServerReplay(sessionId);
    });
    ui->openServerReplayButton->setDisabled(false);

    disconnect(ui->playLocalReplayButton, &QPushButton::clicked, nullptr, nullptr);
    connect(ui->playLocalReplayButton, &QPushButton::clicked, this, [this] {
        changeStackedWidget2(1);
        configureReplayPlayback();
    });

    QPixmap mapPixmap(":/map_images/unknownmap.png");
    if (rawLevel.contains("avg")) {
        mapPixmap = ":/map_images/" + rawLevel + "_tankmap_thumb.png";
    } else {
        mapPixmap = ":/map_images/" + rawLevel + "_map_thumb.png";
    }
    if (mapPixmap.isNull()) {
        mapPixmap = QPixmap(":/map_images/unknownmap.png");
    }

    ui->mapImage->setPixmap(mapPixmap);
    QPixmap fullResolutionMap("C:/Users/Cosimo/Desktop/locations_maps.dxp/" + rawLevel + "_tankmap.png");
    ui->mappa->setPixmap(fullResolutionMap);
    const MapLevelConfig levelConfig = resolveMapLevelConfig(m_selectedReplay.header.rawLevel);
    ui->mappa->setReplayPackets(
        m_selectedReplay.movementPackets,
        levelConfig.isValid ? levelConfig.size : 0.0,
        levelConfig.bottomLeft
    );
    ui->mappa->renderReplayFrame(-1);
    const QString mapName = resolveMapDisplayName(rawLevel, languageCode);

    ui->sessionIdLabel->setText(tr("Session ID: ") + QString::fromStdString(rep.header.sessionId));
    ui->mapNameLabel->setText(tr("Map: ") + mapName);
    ui->difficultyLabel->setText(tr("Difficulty: ") + difficultyToStringLocaleAware(static_cast<Constants::Difficulty>(rep.header.difficulty)));
    ui->startTimeLabel->setText(tr("Start time: ") + epochSToFormattedTime(rep.header.startTimeEpochS));
    ui->timePlayedLabel->setText(tr("Time played: ") + replayLengthToString(rep.results["timePlayed"].as<double>()));

    std::string status = "left";
    const auto it = rep.results.data.find("status");
    if (it != rep.results.data.end()) {
        status = it->second.as<std::string>();
    }
    ui->resultLabel->setText(tr("Result: ") + tr(status.c_str()));

    this->axisList.clear();
    this->alliesList.clear();

    nlohmann::json j;
    to_json(j, rep.results);

    if (j.contains("player") && j["player"].is_array()) {
        nlohmann::json playersInfo;
        if (j.contains("uiScriptsData") && j["uiScriptsData"].contains("playersInfo")) {
            playersInfo = j["uiScriptsData"]["playersInfo"];
        }

        for (const auto &p: j["player"]) {
            UiPlayerData d;

            d.name = QString::fromStdString(p.value("name", ""));
            d.clanTag = QString::fromStdString(p.value("clanTag", ""));
            d.team = p.value("team", 0);
            d.score = p.value("score", 0);
            d.userId = QString::fromStdString(p.value("userId", ""));

            d.airKills = p.value("kills", 0);
            d.groundKills = p.value("groundKills", 0);
            d.navalKills = p.value("navalKills", 0);
            d.assists = p.value("assists", 0);
            d.deaths = p.value("deaths", 0);
            d.caps = p.value("captureZone", 0);
            d.damage = p.value("awardDamage", 0);
            d.bombing = p.value("damageZone", 0);

            d.aiKillsTotal = p.value("aiKills", 0) +
                             p.value("aiGroundKills", 0) +
                             p.value("aiNavalKills", 0);

            if (!d.userId.isEmpty() && !playersInfo.is_null()) {
                std::string key = "__int_" + d.userId.toStdString();
                if (playersInfo.contains(key)) {
                    const auto &info = playersInfo[key];

                    std::string country = info.value("country", "");
                    d.country = QString::fromStdString(country).replace("country_", "");

                    std::string platform = info.value("platform", "");
                    d.platform = QString::fromStdString(platform);

                    auto crafts = info.value("crafts", nlohmann::json::array());
                    QStringList lineupList;
                    for (const auto &craft: crafts.items()) {
                        try {
                            if (craft.value().is_string()) {
                                auto craftName = craft.value().get<std::string>();
                                lineupList.append(QString::fromStdString(craftName));
                            }
                        } catch (...) {
                            continue;
                        }
                    }
                    d.lineup = lineupList.join(", ");
                }
            }

            if (d.platform.isEmpty()) {
                if (d.name.contains("@psn")) d.platform = "psn";
                else if (d.name.contains("@live")) d.platform = "xbox";
                else d.platform = "pc";
            }

            d.displayName = d.clanTag.isEmpty() ? d.name : (d.clanTag + " " + d.name);
            d.displayName = d.displayName.replace("@psn", "").replace("@live", "");

            if (d.team == 1) alliesList.append(d);
            else if (d.team == 2) axisList.append(d);
        }
    }

    // 3. Sort Lists
    auto sorter = [](const UiPlayerData &a, const UiPlayerData &b) { return a.score > b.score; };
    std::sort(alliesList.begin(), alliesList.end(), sorter);
    std::sort(axisList.begin(), axisList.end(), sorter);

    // 4. Update UI
    ui->alliesTable->clear();
    ui->axisTable->clear();
    populateTeamTable(ui->alliesTable, &alliesList, true);
    populateTeamTable(ui->axisTable, &axisList, false);
    populateChatList(rep.chatPackets);

    configureReplayPlayback();
}

void MainWindow::populateTeamTable(QTableWidget *table, const QList<UiPlayerData> *players, const bool allies) {
    const QPalette palette = qApp->palette();
    bool isDarkTheme = palette.color(QPalette::Window).lightness() < 128;

    table->clear();
    table->setRowCount(players->size());
    table->setColumnCount(11);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto loadIcon = [isDarkTheme](const QString &path) {
        QPixmap pixmap(path);
        if (isDarkTheme) {
            return invertIconColors(QIcon(pixmap)).pixmap(32, 32);
        }
        return pixmap;
    };

    const QPixmap scorePixmap = loadIcon(":/icons/score.png");
    const QPixmap killsPixmap = loadIcon(":/icons/kills.png");
    const QPixmap groundKillsPixmap = loadIcon(":/icons/groundKills.png");
    const QPixmap navalKillsPixmap = loadIcon(":/icons/navalKills.png");
    const QPixmap assistsPixmap = loadIcon(":/icons/assists.png");
    const QPixmap capturedZonesPixmap = loadIcon(":/icons/capturedZones.png");
    const QPixmap aiKillsPixmap = loadIcon(":/icons/aiKills.png");
    const QPixmap awardDamagePixmap = loadIcon(":/icons/awardDamage.png");
    const QPixmap damageZonePixmap = loadIcon(":/icons/damageZone.png");
    const QPixmap deathsPixmap = loadIcon(":/icons/deaths.png");

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

    for (int row = 0; row < players->size(); ++row) {
        const UiPlayerData &player = players->at(row);
        QTableWidgetItem *usernameItem = new QTableWidgetItem();
        QString platformIconPath = player.name.contains("@psn") ? ":/icons/psn.png" : player.name.contains("@live") ? ":/icons/xbox.png" : ":/icons/pc.png";
        usernameItem->setIcon(QIcon(platformIconPath));
        if (isDarkTheme) {
            usernameItem->setIcon(invertIconColors(usernameItem->icon()));
        }
        usernameItem->setFont(wtSymbols);
        usernameItem->setText(QString(player.name).replace("@psn", "").replace("@live", ""));
        table->setItem(row, 0, usernameItem);

        auto createItem = [](const QString &text, const QString &tooltip) {
            QTableWidgetItem *item = new QTableWidgetItem(text);
            item->setToolTip(tooltip);
            return item;
        };

        table->setItem(row, 1, createItem(QString::number(player.score), tr("Score")));
        table->setItem(row, 2, createItem(QString::number(player.airKills), tr("Air kills")));
        table->setItem(row, 3, createItem(QString::number(player.groundKills), tr("Ground kills")));
        table->setItem(row, 4, createItem(QString::number(player.navalKills), tr("Naval kills")));
        table->setItem(row, 5, createItem(QString::number(player.assists), tr("Assists")));
        table->setItem(row, 6, createItem(QString::number(player.caps), tr("Captured zones")));
        table->setItem(row, 7, createItem(QString::number(player.aiKillsTotal), tr("AI kills")));
        table->setItem(row, 8, createItem(QString::number(player.damage), tr("Awarded damage")));
        table->setItem(row, 9, createItem(QString::number(player.bombing), tr("Bombing damage")));
        table->setItem(row, 10, createItem(QString::number(player.deaths), tr("Deaths")));
    }

    static bool alliesTableIsConnected = false;
    static bool axisTableIsConnected = false;

    if (!alliesTableIsConnected && allies) {
        connect(ui->alliesTable, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
            const int row = item->row();
            if (row >= 0 && row < this->alliesList.size()) {
                PlayerProfileDialog dialog(this);
                dialog.setPlayerData(this->alliesList.at(row));
                dialog.exec();
            }
        });
        alliesTableIsConnected = true;
    }

    if (!axisTableIsConnected && !allies) {
        connect(ui->axisTable, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
            const int row = item->row();
            if (row >= 0 && row < this->axisList.size()) {
                PlayerProfileDialog dialog(this);
                dialog.setPlayerData(this->axisList.at(row));
                dialog.exec();
            }
        });
        axisTableIsConnected = true;
    }

    table->resizeColumnsToContents();
}

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

void MainWindow::setupReplayControls() {
    m_replayTimer = new QTimer(this);
    m_replayTimer->setInterval(33);

    auto *backButton = new QToolButton(this);
    backButton->setText(tr("Back"));
    backButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoPrevious));
    ui->horizontalLayout_4->insertWidget(0, backButton);

    ui->horizontalSlider->setRange(0, 0);
    ui->horizontalSlider->setValue(0);
    ui->horizontalSlider->setSingleStep(1);
    ui->horizontalSlider->setPageStep(10);
    ui->label_2->setText("00:00/00:00");

    ui->toolButton->setText(tr("Play"));
    ui->toolButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
    ui->toolButton_2->setText(tr("Pause"));
    ui->toolButton_2->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause));
    ui->toolButton->setEnabled(false);
    ui->toolButton_2->setEnabled(false);

    connect(backButton, &QToolButton::clicked, this, [this] {
        m_replayTimer->stop();
        changeStackedWidget2(0);
    });

    connect(ui->toolButton, &QToolButton::clicked, this, [this] {
        if (m_selectedReplay.movementPackets.empty()) {
            return;
        }
        if (ui->horizontalSlider->value() >= ui->horizontalSlider->maximum()) {
            ui->horizontalSlider->setValue(0);
        }
        m_replayTimer->start();
    });

    connect(ui->toolButton_2, &QToolButton::clicked, this, [this] {
        m_replayTimer->stop();
    });

    connect(m_replayTimer, &QTimer::timeout, this, [this] {
        const int value = ui->horizontalSlider->value();
        const int maxValue = ui->horizontalSlider->maximum();
        if (value >= maxValue) {
            m_replayTimer->stop();
            return;
        }
        ui->horizontalSlider->setValue(value + 1);
    });

    connect(ui->horizontalSlider, &QSlider::sliderPressed, this, [this] {
        m_replayTimer->stop();
    });

    connect(ui->horizontalSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_selectedReplay.movementPackets.empty()) {
            ui->mappa->renderReplayFrame(-1);
            ui->label_2->setText("00:00/00:00");
            syncChatWithReplayTime(std::numeric_limits<uint32_t>::max(), false);
            return;
        }

        const int clampedIndex = std::clamp(value, 0, static_cast<int>(m_selectedReplay.movementPackets.size()) - 1);
        const uint32_t currentTime = m_selectedReplay.movementPackets.at(clampedIndex).time;
        ui->mappa->renderReplayFrame(clampedIndex);
        updateReplayTimeLabel(value);
        syncChatWithReplayTime(currentTime, m_replayTimer->isActive());
    });
}

void MainWindow::setupReplayEventTabs() {
    if (m_chatList != nullptr) {
        return;
    }

    m_chatList = new QListWidget(ui->tab_3);
    m_chatList->setSelectionMode(QAbstractItemView::NoSelection);
    m_chatList->setWordWrap(true);
    m_chatList->setUniformItemSizes(false);
    ui->horizontalLayout_5->addWidget(m_chatList);
}

void MainWindow::populateChatList(const std::vector<ChatPacket> &chatPackets) const {
    if (m_chatList == nullptr) {
        return;
    }

    m_chatList->clear();
    if (chatPackets.empty()) {
        m_chatList->addItem(tr("No chat messages for this replay."));
        return;
    }

    std::vector<ChatPacket> sortedPackets = chatPackets;
    std::stable_sort(sortedPackets.begin(), sortedPackets.end(), [](const ChatPacket &a, const ChatPacket &b) {
        return a.time < b.time;
    });

    for (const ChatPacket &packet: sortedPackets) {
        const QString timeLabel = formatReplayTimeMs(packet.time);
        const QString sender = QString::fromStdString(packet.sender).trimmed().isEmpty()
                                   ? tr("Unknown")
                                   : QString::fromStdString(packet.sender);
        const QString teamLabel = packet.isEnemy ? tr("Enemy") : tr("Ally");
        const QString message = QString::fromStdString(packet.message);
        const QString text = QString("[%1] (%2) %3: %4")
                .arg(timeLabel, teamLabel, sender, message);
        auto *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, static_cast<uint>(packet.time));
        item->setToolTip(tr("Channel: %1").arg(packet.channel));
        m_chatList->addItem(item);
    }

    m_chatList->scrollToTop();
}

void MainWindow::syncChatWithReplayTime(const uint32_t currentTimeMs, const bool autoScroll) const {
    if (m_chatList == nullptr || m_chatList->count() == 0) {
        return;
    }

    QListWidgetItem *lastVisibleItem = nullptr;
    for (int i = 0; i < m_chatList->count(); ++i) {
        QListWidgetItem *item = m_chatList->item(i);
        bool ok = false;
        const uint32_t messageTime = item->data(Qt::UserRole).toUInt(&ok);
        if (!ok) {
            continue;
        }

        const bool isVisible = messageTime <= currentTimeMs;
        item->setHidden(!isVisible);
        if (isVisible) {
            lastVisibleItem = item;
        }
    }

    if (autoScroll && lastVisibleItem != nullptr) {
        m_chatList->scrollToItem(lastVisibleItem, QAbstractItemView::PositionAtBottom);
    }
}

void MainWindow::configureReplayPlayback() {
    m_replayTimer->stop();

    if (m_selectedReplay.movementPackets.empty()) {
        ui->horizontalSlider->setRange(0, 0);
        ui->horizontalSlider->setValue(0);
        ui->label_2->setText("00:00/00:00");
        ui->toolButton->setEnabled(false);
        ui->toolButton_2->setEnabled(false);
        ui->mappa->clearReplay();
        syncChatWithReplayTime(std::numeric_limits<uint32_t>::max(), false);
        return;
    }

    std::stable_sort(m_selectedReplay.movementPackets.begin(), m_selectedReplay.movementPackets.end(),
                     [](const MovementPacket &a, const MovementPacket &b) {
                         return a.time < b.time;
                     });

    const MapLevelConfig levelConfig = resolveMapLevelConfig(m_selectedReplay.header.rawLevel);
    ui->mappa->setReplayPackets(
        m_selectedReplay.movementPackets,
        levelConfig.isValid ? levelConfig.size : 0.0,
        levelConfig.bottomLeft
    );

    const QSignalBlocker blocker(ui->horizontalSlider);
    ui->horizontalSlider->setRange(0, static_cast<int>(m_selectedReplay.movementPackets.size()) - 1);
    ui->horizontalSlider->setValue(0);

    ui->mappa->renderReplayFrame(0);
    updateReplayTimeLabel(0);
    syncChatWithReplayTime(m_selectedReplay.movementPackets.front().time, false);
    ui->toolButton->setEnabled(true);
    ui->toolButton_2->setEnabled(true);
}

void MainWindow::updateReplayTimeLabel(const int packetIndex) const {
    if (m_selectedReplay.movementPackets.empty()) {
        ui->label_2->setText("00:00/00:00");
        return;
    }

    const int clampedIndex = std::clamp(packetIndex, 0, static_cast<int>(m_selectedReplay.movementPackets.size()) - 1);
    const uint32_t currentTime = m_selectedReplay.movementPackets.at(clampedIndex).time;
    const uint32_t totalTime = m_selectedReplay.movementPackets.back().time;
    ui->label_2->setText(formatReplayTimeMs(currentTime) + "/" + formatReplayTimeMs(totalTime));
}

QString MainWindow::formatReplayTimeMs(const uint32_t timeMs) const {
    const int totalSeconds = static_cast<int>(timeMs / 1000U);
    const int hours = totalSeconds / 3600;
    const int minutes = (totalSeconds % 3600) / 60;
    const int seconds = totalSeconds % 60;

    if (hours > 0) {
        return QString("%1:%2:%3")
                .arg(hours)
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));
    }

    return QString("%1:%2")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'));
}

MainWindow::MapLevelConfig MainWindow::resolveMapLevelConfig(const std::string &rawLevel) const {
    const QString resourcePath = ":/translations/map_levels.json";
    const QString levelName = QString::fromStdString(rawLevel);

    auto readConfig = [&resourcePath](const QString &identifier) -> MapLevelConfig {
        MapLevelConfig config;
        const QJsonObject obj = getJsonFromResources(resourcePath, identifier);
        if (obj.isEmpty()) {
            return config;
        }

        const QJsonValue sizeValue = obj.value("size");
        const QJsonValue xValue = obj.value("bottomLeftX");
        const QJsonValue yValue = obj.value("bottomLeftY");
        if (!sizeValue.isDouble() || !xValue.isDouble() || !yValue.isDouble()) {
            return config;
        }

        const double size = sizeValue.toDouble();
        if (size <= 0.0) {
            return config;
        }

        config.isValid = true;
        config.size = size;
        config.bottomLeft = QPointF(xValue.toDouble(), yValue.toDouble());
        return config;
    };

    MapLevelConfig config = readConfig(levelName);
    if (config.isValid) {
        return config;
    }

    if (levelName.endsWith("_snow")) {
        config = readConfig(levelName.left(levelName.size() - 5));
        if (config.isValid) {
            return config;
        }
    }

    LOG_WARN_GLOBAL(QString("Map level config not found or invalid for '%1'. Falling back to packet extents.").arg(levelName));
    return {};
}

void MainWindow::importServerReplay(const QString &sessionId) {
    if (sessionId.trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Server Replay"), tr("Invalid replay session ID."));
        return;
    }

    if (m_serverReplayImportThread != nullptr) {
        QMessageBox::information(this, tr("Server Replay"), tr("A server replay import is already in progress."));
        return;
    }

    const QString dbPath = QString(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/wtplotter/replays.sqlite3");
    const QPointer<MainWindow> self(this);
    auto *thread = QThread::create([self, sessionId, dbPath] {
        //Convert session_id from hex to integer
        const ServerImportResult result = downloadAndImportServerReplay(sessionId, dbPath);
        if (self.isNull()) {
            return;
        }
        QMetaObject::invokeMethod(self, [self, result] {
            if (self.isNull()) {
                return;
            }
            self->ui->openServerReplayButton->setEnabled(true);
            self->ui->openServerReplayButton->setText(self->ui->openServerReplayButton->property("originalText").toString());

            if (result.success) {
                self->populateReplayTreeView(self->ui->replayTreeView);
                self->executeCommand(result.sessionId);
                QMessageBox::information(self, QObject::tr("Server Replay"), result.message);
            } else {
                QMessageBox::warning(self, QObject::tr("Server Replay"), result.message);
            }
        }, Qt::QueuedConnection);
    });
    m_serverReplayImportThread = thread;

    const QString originalButtonText = ui->openServerReplayButton->text();
    ui->openServerReplayButton->setProperty("originalText", originalButtonText);
    ui->openServerReplayButton->setEnabled(false);
    ui->openServerReplayButton->setText(tr("Downloading..."));

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this] {
        m_serverReplayImportThread = nullptr;
    });

    thread->start();
}
