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
#include "position.h"
#include "../ui/sceneimageviewer.h"
#include "replay.h"
#include "constants.h"
#include <QMap>
#include <QIcon>
#include <QImage>
#include <QFontDatabase>

void checkAppVersion();

void uploadReplay(Replay &replayData, const QString &uploader, QList<Position> positionCache, QList<Position> poi);

QFile getLatestReplay(const QDir &replayDirectory);

void saveImage(QPixmap drawedMapImage);

QString replayLengthToString(int length);

QString difficultyToString(Constants::Difficulty difficulty);

QString difficultyToStringLocaleAware(Constants::Difficulty difficulty);

QString epochSToFormattedTime(int time);

QIcon invertIconColors(const QIcon &icon);

QJsonObject getJsonFromResources(const QString &resourceName, const QString &identifier);

QJsonArray exportPositionsToJson(Replay &replayData, QList<Position> positionCache, QList<Position> poi);


#endif // UTILS_H
