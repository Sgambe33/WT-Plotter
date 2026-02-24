#include "utils.h"
#include "src/version.h"
#include "logger.h"
#include <QHash>

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
    static QHash<QString, QHash<QString, QJsonObject> > resourceCache;

    if (!resourceCache.contains(resourceName)) {
        QFile file(resourceName);
        if (!file.open(QIODevice::ReadOnly)) {
            LOG_WARN_GLOBAL(QString("Failed to open file: %1").arg(resourceName));
            return QJsonObject();
        }

        const QByteArray data = file.readAll();
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull() || !jsonDoc.isArray()) {
            LOG_WARN_GLOBAL(QString("Failed to parse JSON array from file: %1").arg(resourceName));
            return QJsonObject();
        }

        QHash<QString, QJsonObject> byIdentifier;
        const QJsonArray jsonArray = jsonDoc.array();
        for (const QJsonValue &value: jsonArray) {
            if (!value.isObject()) {
                continue;
            }
            const QJsonObject obj = value.toObject();
            const QString id = obj.value("identifier").toString();
            if (!id.isEmpty()) {
                byIdentifier.insert(id, obj);
            }
        }
        resourceCache.insert(resourceName, std::move(byIdentifier));
    }

    const auto resourceIt = resourceCache.constFind(resourceName);
    if (resourceIt == resourceCache.cend()) {
        return QJsonObject();
    }

    const auto &byIdentifier = resourceIt.value();
    const auto objectIt = byIdentifier.constFind(identifier);
    if (objectIt != byIdentifier.cend()) {
        return objectIt.value();
    }

    LOG_WARN_GLOBAL(QString("No object found with identifier: %1").arg(identifier));
    return QJsonObject();
}
