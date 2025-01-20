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

#include "classes/player.h"
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
	void changeStackedWidget(int index);
	void updateProgressBar(double progress);
	void updateStatusLabel(QString msg);

private:
	Ui::MainWindow* ui;
	QStandardItemModel* model;
	QThread* m_thread = nullptr;
	Worker* m_worker;

	void startPlotter();
	void stopPlotter();
	void populateReplayTreeView(QTreeView* replayTreeView, const QString& directoryPath);
	void onTreeItemClicked(const QModelIndex& index);
	void executeCommand(const QString& filePath);
	void populateTeamTable(QTableWidget* table, const QList<Player>& players);
	void checkForUpdates();
	void setCustomFont(const QString& fontPath, QWidget* widget);
};

#endif // MAINWINDOW_H
