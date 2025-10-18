#include "utils.h"
#include "src/version.h"
#include "logger.h"

void checkAppVersion() {
    QUrl url("https://raw.githubusercontent.com/Sgambe33/WT-Plotter/refs/heads/main/version.json");

#ifdef DEBUG_BUILD
    url = "http://localhost:5000/version";
#endif

    QNetworkRequest request(url);
    QNetworkAccessManager networkManager;
    QNetworkReply *reply = networkManager.get(request);

    QString appVersion = QString("%1.%2.%3")
            .arg(APP_VERSION_MAJOR)
            .arg(APP_VERSION_MINOR)
            .arg(APP_VERSION_PATCH);

    LOG_INFO_GLOBAL(QString("Running wtplotter version %1").arg(appVersion));

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        LOG_INFO_GLOBAL(QString("Failed to fetch version information: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    reply->deleteLater();

    if (!jsonDoc.isObject()) {
        LOG_WARN_GLOBAL("Invalid JSON received");
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QString latestVersion = jsonObj.value("version").toString();
    QString changelog = jsonObj.value("changelog").toString();
    bool isCritical = jsonObj.value("critical").toBool();

    if (latestVersion.isEmpty()) {
        LOG_WARN_GLOBAL("Version key not found in JSON");
        return;
    }

    if (appVersion != latestVersion && isCritical) {
        QMessageBox::warning(nullptr, "Update Required", R"(
        <p>A new version of this app has been found. Please update by
        downloading the latest version <a href='https://github.com/Sgambe33/WT-Plotter/releases/latest'>
        here</a>.</p>)" + changelog + R"(<p>Thank you< / p>)");
        std::exit(0);
    }
    if (appVersion != latestVersion && !isCritical) {
        QMessageBox::information(nullptr, "Update Available",
                                 changelog +
                                 R"(Update by downloading the latest version <a href='https://github.com/Sgambe33/WT-Plotter/releases/latest'> here</a>)");
    }
}

QFile getLatestReplay(const QDir &replayDirectory) {
    const QFileInfoList files = replayDirectory.entryInfoList(QDir::Files, QDir::Time);
    if (files.isEmpty()) {
        return QFile();
    }

    qint64 sixtySecondsAgo = QDateTime::currentMSecsSinceEpoch() - 120000;
    for (const QFileInfo &fileInfo: files) {
        if (fileInfo.suffix() == "wrpl" && fileInfo.lastModified().toMSecsSinceEpoch() >= sixtySecondsAgo) {
            return QFile(fileInfo.filePath());
        }
    }

    return QFile();
}

//TODO:Optimize
void uploadReplay(Replay &replayData, const QString &uploader, QList<Position> positionCache, QList<Position> poi) {
    QNetworkAccessManager networkManager;
    QNetworkRequest request(QUrl("https://warthunder-heatmaps.crabdance.com/uploadPositions"));
#ifdef DEBUG_BUILD
    request.setUrl(QUrl("http://localhost:5000/uploadPositions"));
#endif

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject headerMap;
    headerMap["sessionId"] = replayData.getSessionId();
    headerMap["uploader"] = uploader;
    headerMap["startTime"] = replayData.getStartTime();
    headerMap["map"] = replayData.getLevel();
    headerMap["gameMode"] = replayData.getBattleType();
    headerMap["difficulty"] = difficultyToString(replayData.getDifficulty());
    headerMap["wtplotterVersion"] = QString("%1.%2.%3")
            .arg(APP_VERSION_MAJOR)
            .arg(APP_VERSION_MINOR)
            .arg(APP_VERSION_PATCH);

    QJsonObject data;
    data["replayHeader"] = headerMap;
    data["positions"] = exportPositionsToJson(replayData, positionCache, poi);

    QJsonDocument doc(data);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    QNetworkReply *reply = networkManager.post(request, jsonData);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        LOG_WARN_GLOBAL(QString("Failed to upload replay: %1").arg(reply->errorString()));
    } else {
        LOG_INFO_GLOBAL(
            QString("Replay uploaded successfully. Uploaded %1 positions").arg(data["positions"].toArray().size()));
    }
    reply->deleteLater();
}

QJsonArray exportPositionsToJson(Replay &replayData, QList<Position> positionCache, QList<Position> poi) {
    QJsonArray positions;
    for (const Position &position: positionCache) {
        QJsonObject obj;
        obj["x"] = position.x();
        obj["y"] = position.y();
        obj["type"] = position.type();
        obj["icon"] = position.icon();
        obj["timestamp"] = position.timestamp();
        obj["sessionId"] = replayData.getSessionId();
        positions.append(obj);
    }
    for (const Position &position: poi) {
        QJsonObject obj;
        obj["x"] = position.x();
        obj["y"] = position.y();
        obj["type"] = position.type();
        obj["icon"] = position.icon();
        obj["timestamp"] = position.timestamp();
        obj["sessionId"] = replayData.getSessionId();
        positions.append(obj);
    }

    return positions;
}

void saveImage(QPixmap drawedMapImage) {
    QString savePath = QSettings("sgambe33", "wtplotter").value("plotSavePath", "").toString();

    if (drawedMapImage.isNull()) {
        LOG_ERROR_GLOBAL("drawedMapImage is null, cannot draw positions");
        return;
    }

    if (savePath.trimmed().isEmpty()) {
        LOG_ERROR_GLOBAL("savePath is not set");
        QMessageBox msgBox;
        msgBox.critical(nullptr, "Error", "You have not set the save folder in the preferences!");
        return;
    }

    QDir savePathDir(savePath);
    if (!savePathDir.exists()) {
        LOG_ERROR_GLOBAL(QString("savePath directory does not exist: %1").arg(savePath));
        return;
    }

    QString fileName = savePathDir.absoluteFilePath(QString::number(QDateTime::currentSecsSinceEpoch()) + ".jpg");

    QImageWriter writer;
    writer.setFormat("jpg");
    writer.setFileName(fileName);

    if (!writer.write(drawedMapImage.toImage())) {
        LOG_ERROR_GLOBAL(QString("Error while saving match plot to disk: %1").arg(writer.errorString()));
    } else {
        LOG_INFO_GLOBAL("Match plot saved successfully");
    }
}

QString replayLengthToString(int length) {
    int hours = length / 3600;
    int minutes = (length % 3600) / 60;
    int seconds = length % 60;
    return QString("%1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(
        seconds, 2, 10, QChar('0'));
}

QString difficultyToString(Constants::Difficulty difficulty) {
    switch (difficulty) {
        case Constants::Difficulty::ARCADE: return "ARCADE";
        case Constants::Difficulty::REALISTIC: return "REALISTIC";
        case Constants::Difficulty::SIMULATOR: return "SIMULATOR";
        default: return "UNKNOWN";
    }
}

QString difficultyToStringLocaleAware(Constants::Difficulty difficulty) {
    switch (difficulty) {
        case Constants::Difficulty::ARCADE: return QObject::tr("Arcade");
        case Constants::Difficulty::REALISTIC: return QObject::tr("Realistic");
        case Constants::Difficulty::SIMULATOR: return QObject::tr("Simulator");
        default: return QObject::tr("UNKNOWN");
    }
}

QString epochSToFormattedTime(int time) {
    QDateTime startTime = QDateTime::fromSecsSinceEpoch(time);
    return startTime.toString("hh:mm:ss");
}

QIcon invertIconColors(const QIcon &icon) {
    QPixmap pixmap = icon.pixmap(32, 32);
    QImage image = pixmap.toImage();
    image.invertPixels();
    return QIcon(QPixmap::fromImage(image));
}

QJsonObject getJsonFromResources(const QString &resourceName, const QString &identifier) {
    QFile file(resourceName);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARN_GLOBAL(QString("Failed to oen file: %1").arg(resourceName));
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

    if (jsonDoc.isNull() || !jsonDoc.isArray()) {
        LOG_WARN_GLOBAL(QString("Failed to parse JSON array from file: %1").arg(resourceName));
        return QJsonObject();
    }

    const QJsonArray jsonArray = jsonDoc.array();

    for (const QJsonValue &value: jsonArray) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            if (obj.contains("identifier") && obj["identifier"].toString() == identifier) {
                return obj;
            }
        }
    }
    LOG_WARN_GLOBAL(QString("No ibject found with identifier: %1").arg(identifier));
    return QJsonObject();
}
