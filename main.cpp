#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTranslator>
#include <QLocale>
#include <QDebug>

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
        qCritical() << "SQLite driver not available!";
        return -1;
    }

    QTranslator appTranslator;
    QString languageCode = settings.value("language", "en").toString();
    QString translationFile = QCoreApplication::applicationDirPath() + QString("/wtplotter_%1.qm").arg(languageCode);
    if (appTranslator.load(translationFile)) {
        a.installTranslator(&appTranslator);
    }
    else {
        qWarning() << "Failed to load translation file:" << translationFile;
    }

    MainWindow w;
    w.show();

    return a.exec();
}