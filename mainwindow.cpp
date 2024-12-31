#include "mainwindow.h"
#include "./ui_mainwindow.h"
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
#include "classes\replay.h"
#include "./sceneimageviewer.h"
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QPushButton>
#include <qfontdatabase.h>
#include "worker.h"
#include "preferencesdialog.h"
#include <QSettings>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), 
    model(new QStandardItemModel(this)),
    m_thread(nullptr),
    m_worker(nullptr)
{
    ui->setupUi(this);

    QSettings settings("sgambe33", "wtplotter");
    
	if (!settings.value("replayFolderPath").isNull()) {
        populateReplayTreeView(ui->replayTreeView, settings.value("replayFolderPath").toString());
	}

    ui->splitter->setSizes(QList<int>() << 50 << 200);

    QPixmap img(":/icons/map_images/unknownmap.png");
    img = img.scaled(125, 125, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->mapImage->setPixmap(img);

    QStatusBar *statusBar = ui->statusbar;
    QWidget *buttonBox = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(buttonBox);
    layout->setContentsMargins(0, 0, 0, 0);

    QPushButton *replayButton = new QPushButton("Replays", buttonBox);

    

    QPushButton *plotterButton = new QPushButton("Plotter", buttonBox);

    connect(replayButton, &QPushButton::clicked, [=] {
        ui->stackedWidget->setCurrentIndex(0);
        ui->replayTreeView->setDisabled(false);
        plotterButton->setDisabled(false);
        replayButton->setDisabled(true);
        stopPlotter();
    });

    connect(plotterButton, &QPushButton::clicked, [=] {
        ui->stackedWidget->setCurrentIndex(2);
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

    startPlotter();
    connect(ui->replayTreeView, &QTreeView::clicked, this, &MainWindow::onTreeItemClicked);
    connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::openPreferencesDialog);
}

void MainWindow::openPreferencesDialog()
{
    PreferencesDialog dialog(this);
    dialog.exec();
}

void MainWindow::setCustomFont(const QString &fontPath, QWidget *widget) {
    int id = QFontDatabase::addApplicationFont(fontPath);
    if (id != -1) {
        QString family = QFontDatabase::applicationFontFamilies(id).at(0);
        QFont customFont(family);
        widget->setFont(customFont);
        for (auto child : widget->findChildren<QWidget*>()) {
            child->setFont(customFont);
        }
    } else {
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
    connect(m_worker, &Worker::changeStackedWidget, this, &MainWindow::changeStackedWidget);
    
    m_worker->moveToThread(m_thread);
    m_thread->start();
}

void MainWindow::updatePixmap(const QPixmap &pixmap)
{
    ui->mappa->setPixmap(pixmap);
}

void MainWindow::changeStackedWidget(int index)
{   
    ui->stackedWidget->setCurrentIndex(index);
}

void MainWindow::updateProgressBar(double progress){
	ui->progressBar->setValue(progress);
}

void MainWindow::updateStatusLabel(QString msg){
    ui->statusLabel->setText(msg);
}

void MainWindow::stopPlotter() {
    if (m_thread && m_worker) {
        QMetaObject::invokeMethod(m_worker, "stopTimer", Qt::QueuedConnection);

        m_thread->quit();
        m_thread->wait();

        m_thread = nullptr;
        m_worker = nullptr;

        qDebug()<< "Killing plotter and thread...";
    }
}

void MainWindow::populateReplayTreeView(QTreeView *replayTreeView, const QString &directoryPath)
{
    model->setHorizontalHeaderLabels({"File Name"});

    QMap<QDate, QList<QFileInfo>> filesByDate;

    QDir dir(directoryPath);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    dir.setNameFilters({"*.wrpl"});
    QFileInfoList fileInfoList = dir.entryInfoList();

    for (const QFileInfo &fileInfo : fileInfoList)
    {
        QDate creationDate = fileInfo.birthTime().date();
        filesByDate[creationDate].append(fileInfo);
    }

    for (auto it = filesByDate.cbegin(); it != filesByDate.cend(); ++it)
    {
        QStandardItem *dateItem = new QStandardItem(it.key().toString("yyyy-MM-dd"));
        dateItem->setFlags(dateItem->flags() & ~Qt::ItemIsEditable);
        for (const QFileInfo &fileInfo : it.value())
        {
            QStandardItem *fileNameItem = new QStandardItem(fileInfo.fileName());
            fileNameItem->setData(fileInfo.absoluteFilePath(), Qt::UserRole);
            fileNameItem->setFlags(fileNameItem->flags() & ~Qt::ItemIsEditable);
            dateItem->appendRow({fileNameItem});
        }

        model->appendRow(dateItem);
    }

    replayTreeView->setModel(model);
    replayTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void MainWindow::onTreeItemClicked(const QModelIndex &index)\
{
    QString filePath = index.data(Qt::UserRole).toString();

    if (!filePath.isEmpty())
    {
        qDebug() << filePath;
        executeCommand(filePath);
    }
}

void MainWindow::executeCommand(const QString &filePath)
{
    Replay rep = Replay::fromFile(filePath);


	QPixmap mapPixmap(":/icons/map_images/" + rep.getLevel() + "_tankmap_thumb.png");
	if (mapPixmap.isNull()) {
		mapPixmap = QPixmap(":/icons/map_images/unknownmap.png");
    }
    ui->mapImage->setPixmap(mapPixmap);

    ui->mapNameLabel->setText(QString("Map: ") + rep.getLevel());
    ui->startTimeLabel->setText(QString("Start time: ") + QString::number(rep.getTimePlayed())); //TODO:FIX
    ui->timePlayedLabel->setText(QString("Time played: ") + QString::number(rep.getTimePlayed()));
    ui->resultLabel->setText(QString("Result: ") + rep.getStatus());

    QList<Player> players = rep.getPlayers();

    QList<Player> allies;
    QList<Player> axis;
    for (const Player &player : players)
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

    std::sort(allies.begin(), allies.end(), [](const Player &p1, const Player &p2)
              {
                  return p1.getScore() > p2.getScore();
              });
    std::sort(axis.begin(), axis.end(), [](const Player &p1, const Player &p2)
              {
                  return p1.getScore() > p2.getScore();
              });

    // Populate both tables (allies and axis)
    populateTeamTable(ui->alliesTable, allies);
    populateTeamTable(ui->axisTable, axis);
}

void MainWindow::populateTeamTable(QTableWidget *table, const QList<Player> &players)
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
        const Player &player = players[i];
        table->setItem(i, 0, new QTableWidgetItem(player.getClanTag()+ " " + player.getName()));
        
        QTableWidgetItem *scoreItem = new QTableWidgetItem(QString::number(player.getScore()));
        scoreItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 1, scoreItem);

        QTableWidgetItem *killsItem = new QTableWidgetItem(QString::number(player.getKills()));
        killsItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 2, killsItem);

        QTableWidgetItem *groundKillsItem = new QTableWidgetItem(QString::number(player.getGroundKills()));
        groundKillsItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 3, groundKillsItem);

        QTableWidgetItem *navalKillsItem = new QTableWidgetItem(QString::number(player.getNavalKills()));
        navalKillsItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 4, navalKillsItem);

        QTableWidgetItem *assistsItem = new QTableWidgetItem(QString::number(player.getAssists()));
        assistsItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 5, assistsItem);

        QTableWidgetItem *captureZoneItem = new QTableWidgetItem(QString::number(player.getCaptureZone()));
        captureZoneItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 6, captureZoneItem);

        QTableWidgetItem *aiKillsItem = new QTableWidgetItem(QString::number(player.getAiKills() + player.getAiGroundKills() + player.getAiNavalKills()));
        aiKillsItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 7, aiKillsItem);

        QTableWidgetItem *awardDamageItem = new QTableWidgetItem(QString::number(player.getAwardDamage()));
        awardDamageItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 8, awardDamageItem);

        QTableWidgetItem *damageZoneItem = new QTableWidgetItem(QString::number(player.getDamageZone()));
        damageZoneItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 9, damageZoneItem);

        QTableWidgetItem *deathsItem = new QTableWidgetItem(QString::number(player.getDeaths()));
        deathsItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 10, deathsItem);
    }

    table->resizeColumnsToContents();
}

MainWindow::~MainWindow()
{
    delete ui;
}
