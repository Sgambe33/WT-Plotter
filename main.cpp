#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	a.setStyle("Fusion");

	QDir docDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/wtplotter");
	if (!docDir.exists())
		docDir.mkpath(".");
	QDir plotDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/wtplotter/plots");
	if (!plotDir.exists())
		plotDir.mkpath(".");

	QSettings settings("sgambe33", "wtplotter");
	if (settings.value("plotSavePath").toString().isEmpty())
	{
		settings.setValue("plotSavePath", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/wtplotter/plots");
	}

	if (!QSqlDatabase::isDriverAvailable("QSQLITE"))
	{
		qDebug() << "SQLite driver not available!";
		return -1;
	}

	MainWindow w;
	w.show();
	return a.exec();
}
