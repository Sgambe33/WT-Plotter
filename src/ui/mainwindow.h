#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QStandardItemModel>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QStandardItem>
#include <QLabel>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QDebug>
#include <QTableWidget>
#include <QStackedWidget>
#include <QFont>
#include <QTimer>
#include <QPointF>
#include <QListWidget>
#include <cstdint>

#include "playerprofiledialog.h"
#include "../classes/dbmanager.h"
#include "../GameTelemetryWorker.h"
#include "sceneimageviewer.h"
#include "../classes/discordworker.h"
#include "Structs.h"


QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    void openPreferencesDialog();

    void openAboutDialog();

    ~MainWindow();

public slots:
    void updatePixmap(const QPixmap &pixmap);

    void refreshReplays();

    void loadReplaysFromFolder();

    void onReplayLoaderFinished();

    void updateProgressBar(double progress);

    void updateStatusLabel(QString msg);

    void changeStackedWidget1(int index);

    void changeStackedWidget2(int index);

    void onLanguageChanged(const QString &languageCode);

signals:
    void sendActivityToDiscord(const QString &state, const QString &details, const QString &logo, time_t epochStartTime = -1, const QString &largeText = QString());

protected:
    void closeEvent(QCloseEvent *event) override;

    void changeEvent(QEvent *event) override;

private:
    struct MapLevelConfig {
        bool isValid = false;
        double size = 0.0;
        QPointF bottomLeft;
    };

    Ui::MainWindow *ui;
    QStandardItemModel *model;
    QThread *m_discord_thread = nullptr;
    DiscordWorker *m_discord_worker;
    DbManager m_dbmanager;
    QTranslator *appTranslator;
    QFont wtSymbols;
    QSettings settings;

    QList<UiPlayerData> alliesList;
    QList<UiPlayerData> axisList;
    wrpl::Replay m_selectedReplay;
    QTimer *m_replayTimer = nullptr;
    QListWidget *m_chatList = nullptr;
    QThread *m_serverReplayImportThread = nullptr;

    void startDiscordPresence();

    void setActivityFromMainWindow(const QString &state, const QString &details, const QString &logo, time_t epochStartTime = -1, const QString &largeText = QString());

    void populateReplayTreeView(QTreeView *replayTreeView) const;

    void onTreeItemClicked(const QModelIndex &index);

    void executeCommand(const QString &sessionId);
    void importServerReplay(const QString &sessionId);

    void populateTeamTable(QTableWidget *table, const QList<UiPlayerData> *players, bool allies);

    void setupReplayControls();
    void setupReplayEventTabs();
    void populateChatList(const std::vector<ChatPacket> &chatPackets) const;
    void syncChatWithReplayTime(uint32_t currentTimeMs, bool autoScroll) const;
    void configureReplayPlayback();
    void updateReplayTimeLabel(int packetIndex) const;
    QString formatReplayTimeMs(uint32_t timeMs) const;
    MapLevelConfig resolveMapLevelConfig(const std::string &rawLevel) const;

    void changeLanguage(const QString &languageCode);

    void setCustomFont(const QString &fontPath, QWidget *widget);
};


#endif // MAINWINDOW_H
