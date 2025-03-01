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

#include "classes/player.h"
#include "classes/dbmanager.h"
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

private:
	Ui::MainWindow* ui;
	QStandardItemModel* model;
	QThread* m_thread = nullptr;
	Worker* m_worker;
	DbManager m_dbmanager;
	QTranslator* appTranslator;

	void startPlotter();
	void stopPlotter();
	void populateReplayTreeView(QTreeView* replayTreeView, const QString& directoryPath);
	void onTreeItemClicked(const QModelIndex& index);
	void executeCommand(const QString& filePath);
	void populateTeamTable(QTableWidget* table, const QList<QPair<Player, PlayerReplayData>>& players);
	void changeLanguage(const QString& languageCode);
	void setCustomFont(const QString& fontPath, QWidget* widget);
};

#endif // MAINWINDOW_H
