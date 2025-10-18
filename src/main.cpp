#include "ui/mainwindow.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTranslator>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QObject>
#include <QDebug>

void setupApplicationDirectories() {
	QDir docDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/wtplotter");
	if (!docDir.exists()) {
		docDir.mkpath(".");
	}

	QDir plotDir(docDir.filePath("plots"));
	if (!plotDir.exists()) {
		plotDir.mkpath(".");
	}

	QSettings settings("sgambe33", "wtplotter");
	if (settings.value("plotSavePath").toString().isEmpty()) {
		settings.setValue("plotSavePath", plotDir.path());
	}
}

bool isSqliteDriverAvailable() {
	if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
		qCritical() << "SQLite driver not available!";
		return false;
	}
	return true;
}

void setupTranslations(QSettings& settings) {
	static QTranslator translator;
	QString languageCode = settings.value("language", "en").toString();
	QString translationPath;

	QDir translationsDir(QCoreApplication::applicationDirPath() + "/translations");
	if (translationsDir.exists()) {
		translationPath = translationsDir.filePath(QString("wtplotter_%1.qm").arg(languageCode));
	}
	else {
		translationPath = QCoreApplication::applicationDirPath() + QString("/wtplotter_%1.qm").arg(languageCode);
	}

	if (translator.load(translationPath)) {
		QCoreApplication::installTranslator(&translator);
	}
	else {
		qWarning() << "Failed to load translation file:" << translationPath;
	}
}

void setupSystemTray(QSystemTrayIcon& trayIcon, MainWindow& mainWindow, QApplication& app) {
	trayIcon.setIcon(QIcon(":/icons/logo.png"));

	QMenu* trayMenu = new QMenu();
	QAction* quitAction = new QAction(QObject::tr("Quit"), trayMenu);
	trayMenu->addAction(quitAction);
	QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

	trayIcon.setContextMenu(trayMenu);

	QObject::connect(&trayIcon, &QSystemTrayIcon::activated, [&mainWindow](QSystemTrayIcon::ActivationReason reason) {
		if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
			mainWindow.show();
			mainWindow.setWindowState(Qt::WindowNoState);
			mainWindow.raise();
			mainWindow.activateWindow();
		}
		});

	QSettings settings("sgambe33", "wtplotter");
	if (settings.value("startMinimized", false).toBool()) {
		mainWindow.hide();
		mainWindow.setWindowState(Qt::WindowMinimized);
	}
	else {
		mainWindow.show();
	}

	trayIcon.show();
}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	app.setStyle("Fusion");

	setupApplicationDirectories();

	if (!isSqliteDriverAvailable()) {
		return -1;
	}

	QSettings settings("sgambe33", "wtplotter");
	setupTranslations(settings);

	MainWindow mainWindow;

	QSystemTrayIcon trayIcon;
	setupSystemTray(trayIcon, mainWindow, app);

	return app.exec();
}
