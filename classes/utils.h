#ifndef UTILS_H
#define UTILS_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QUrl>
#include <QFile>
#include <QFileInfoList>
#include <QString>
#include <QDebug>
#include <QObject>
#include <QPixmap>
#include <QTimer>
#include <QDir>
#include <QJsonArray>
#include <QList>
#include <QSettings>
#include <QImageWriter>
#include "classes/position.h"
#include "sceneimageviewer.h"
#include "classes/replay.h"
#include "classes/constants.h"
#include <QMap>
#include <QIcon>
#include <QImage>
#include <QFontDatabase>

class Utils {
public:
    static void checkAppVersion();
    static void uploadReplay(Replay& replayData, const QString& uploader, QList<Position> positionCache, QList<Position> poi);
    static QFile getLatestReplay(const QDir& replayDirectory);
    static void saveImage(QPixmap drawedMapImage);
    static QString replayLengthToString(int length);
    static QString difficultyToString(Constants::Difficulty difficulty);
    static QString difficultyToStringLocaleAware(Constants::Difficulty difficulty);
    static Constants::Difficulty stringToDifficulty(const QString& difficultyStr);
    static QString epochSToFormattedTime(int time);
    static QIcon invertIconColors(const QIcon& icon);
    static QJsonObject getJsonFromResources(const QString& resourceName, const QString& identifier);
    static void setCustomFont(const QString& fontPath, QWidget* widget) {


private:    
    static QJsonArray exportPositionsToJson(Replay& replayData, QList<Position> positionCache, QList<Position> poi);
};

#endif // UTILS_H
