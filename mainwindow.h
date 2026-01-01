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

#include "classes/player.h"
#include "classes/dbmanager.h"
#include "classes/discordworker.h"
#include "worker.h"
#include "sceneimageviewer.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	void openPreferencesDialog();
	void openAboutDialog();
	~MainWindow();

public slots:
	void updatePixmap(const QPixmap& pixmap);
	void refreshReplays();
	void loadReplaysFromFolder();
	void onReplayLoaderFinished();
	void updateProgressBar(double progress);
	void updateStatusLabel(QString msg);
	void changeStackedWidget1(int index);
	void changeStackedWidget2(int index);
	void onLanguageChanged(const QString& languageCode);

protected:
	void closeEvent(QCloseEvent* event) override;
	void changeEvent(QEvent* event) override;

private:
	Ui::MainWindow* ui;
	QStandardItemModel* model;

	QThread* m_worker_thread = nullptr;
	Worker* m_worker;

	QThread* m_discord_thread = nullptr;
	DiscordWorker* m_discord_worker;

	DbManager m_dbmanager;
	QTranslator* appTranslator;
	QFont wtSymbols;
	QSettings settings;
	QList<QPair<Player, PlayerReplayData>>* allies = new QList<QPair<Player, PlayerReplayData>>();
	QList<QPair<Player, PlayerReplayData>>* axis = new QList<QPair<Player, PlayerReplayData>>();

	void startPlotter();
	void stopPlotter();

	void startDiscord();
	void stopDiscord();

	void populateReplayTreeView(QTreeView* replayTreeView, const QString& directoryPath);
	void onTreeItemClicked(const QModelIndex& index);
	void executeCommand(const QString& filePath);
	void populateTeamTable(QTableWidget* table, const QList<QPair<Player, PlayerReplayData>>* players, bool allies);
	void changeLanguage(const QString& languageCode);
	void setCustomFont(const QString& fontPath, QWidget* widget);
};

#endif // MAINWINDOW_H
