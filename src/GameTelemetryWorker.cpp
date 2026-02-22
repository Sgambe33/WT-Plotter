#include "GameTelemetryWorker.h"
#include "classes/utils.h"
#include <QDateTime>
#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImageWriter>
#include <QThread>
#include <QFile>
#include <QFileInfoList>
#include <QDir>
#include <QSettings>
#include "classes/logger.h"
#include <QMessageBox>
#include <qbuffer.h>


bool GameTelemetryWorker::havePOIBeenDrawn = false;
QString GameTelemetryWorker::DATA_URL = "http://localhost:8111/map_obj.json";
QString GameTelemetryWorker::MAP_URL = "http://localhost:8111/map.img";
QString GameTelemetryWorker::MAP_INFO = "http://localhost:8111/map_info.json";
QString GameTelemetryWorker::INDICATORS = "http://localhost:8111/indicators";
QString GameTelemetryWorker::STATE = "http://localhost:8111/state";

GameTelemetryWorker::GameTelemetryWorker(SceneImageViewer *imageViewer, QObject *parent)
    : QObject(parent),
      m_timer(new QTimer(this)),
      m_matchStartTime(0),
      networkManager(new QNetworkAccessManager(this)) {
    connect(m_timer, &QTimer::timeout, this, &GameTelemetryWorker::onTimeout);
}

GameTelemetryWorker::~GameTelemetryWorker() {
    stopTimer();
}

void GameTelemetryWorker::startTimer() {
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "startTimer", Qt::QueuedConnection);
        return;
    }

    m_timer->start(1000);
    activityTimer.start();
}

void GameTelemetryWorker::stopTimer() {
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "stopTimer", Qt::QueuedConnection);
        return;
    }
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

void GameTelemetryWorker::performTask() {
    startTimer();
}

void GameTelemetryWorker::onTimeout() {
    try {
        if (false) {//shouldLoadMap()
            fetchAndDisplayMap();
            emit changeStackedWidget2(2);
            emit updateStatusLabel(QString("Map loaded..."));
        } // else if (shouldUpdateMarkers()) {
        //    if (m_matchStartTime == 0) {
        //        m_matchStartTime = QDateTime::currentMSecsSinceEpoch();
        //        matchStartEpoch = time(nullptr);
        //        updatePOI();
        //        emit updateStatusLabel(QString("Match started..."));
        //        emit sendActivityToDiscord("Match started", "Entering lobby", "logowt_stripe_flat");
        //    }
        //    updateMarkers()
        //
        //    if (activityTimer.elapsed() >= 10000) {
        //        showAltActivity = !showAltActivity;
        //        if (showAltActivity) {
        //            QString lookedupMapName = Constants::lookupMapName(currentMap);
        //
        //            QString mapName = getJsonFromResources(":/translations/locations.json", Constants::lookupMapName(currentMap)).value("en").toString();
        //            if (mapName.isEmpty()) {
        //                mapName = "Unknown map";
        //                lookedupMapName = "unknownmap";
        //            }
        //            emit sendActivityToDiscord("In mission", mapName, lookedupMapName + "_tankmap", matchStartEpoch);
        //        } else {
        //            QJsonObject vehicleIndicators = fetchJsonElement(GameTelemetryWorker::INDICATORS);
        //            QJsonObject vehicleState = fetchJsonElement(GameTelemetryWorker::STATE);
        //            if (vehicleIndicators.value("type").toString().contains("tankModels/")) {
        //                //The player is in a tank
        //                QString vehicleName = vehicleIndicators.value("type").toString().replace("tankModels/", "");
        //                QString translatedVehicleName = getJsonFromResources(":/translations/vehicles.json", vehicleName).value("en").toString();
        //                int totalCrew = vehicleIndicators.value("crew_total").toInt();
        //                int aliveCrew = vehicleIndicators.value("crew_current").toInt();
        //                int currentSpeed = vehicleIndicators.value("speed").toInt();
        //                QString formattedText = QString("Crew: %1/%2 | Speed: %3").arg(aliveCrew).arg(totalCrew).arg(currentSpeed);
        //                emit sendActivityToDiscord("In mission", translatedVehicleName, QString("https://static.encyclopedia.warthunder.com/images/%1.png").arg(vehicleName),
        //                                           matchStartEpoch, formattedText);
        //            } else {
        //                //The player is in a plane
        //                QString vehicleName = vehicleIndicators.value("type").toString();
        //                QString translatedVehicleName = getJsonFromResources(":/translations/vehicles.json", vehicleName).value("en").toString();
        //                int speed = vehicleState.value("TAS, km/h").toInt();
        //                int altitude = vehicleState.value("H, m").toInt();
        //                QString formattedText = QString("Speed: %1 | Altitude: %2").arg(speed).arg(altitude);
        //                emit sendActivityToDiscord("In mission", translatedVehicleName, QString("https://static.encyclopedia.warthunder.com/images/%1.png").arg(vehicleName),
        //                                           matchStartEpoch, formattedText);
        //            }
        //        }
        //        activityTimer.restart();
        //    }
        //} else if (shouldEndMatch()) {
        //    m_matchStartTime = 0;
        //    matchStartEpoch = 0;
        //
        //    emit updateStatusLabel(QString("Match ended..."));
        //
        //    QPixmap originalMap = getOriginalMapImage();
        //    drawSpecialMarkers(originalMap);
        //    drawMarkers(originalMap);
        //    this->m_drawedMapImage = originalMap;
        //
        //    emit changeStackedWidget2(1);
        //    emit updatePixmap(originalMap);
        //
        //    endMatch();
        //    restartScheduler();
    else
    {
        emit updateStatusLabel(tr("Awaiting match start..."));
        sendActivityToDiscord("", "In hangar", "logowt_stripe_flat");
    }
}

catch
(
const std::exception &e
)
 {
        qCritical() << "An error occurred during scheduled task execution:" << e.what();
    }
}

void GameTelemetryWorker::restartScheduler() {
    stopTimer();
    startTimer();
}

bool GameTelemetryWorker::isMatchRunning() {
    try {
        QJsonObject mapInfo = fetchJsonElement(MAP_INFO);
        if (!mapInfo.isEmpty()) {
            return mapInfo.contains("valid") && mapInfo["valid"].toBool();
        }
        return false;
    } catch (const std::exception &e) {
        LOG_ERROR(QString("Exception while fetching map info: %1").arg(e.what()));
        //qCritical() << "Exception while fetching map info:" << e.what();
        return false;
    }
}

bool GameTelemetryWorker::isPlayerOnTank() {
    try {
        QJsonObject response = fetchJsonElement(INDICATORS);
        if (!response.isEmpty()) {
            bool result = response.contains("valid") && response["valid"].toBool();
            result = result && (response.contains("army") && response["army"].toString().compare("tank", Qt::CaseInsensitive) == 0);
            return result;
        }
        return false;
    } catch (const std::exception &e) {
        LOG_ERROR(QString("Exception while fetching indicators: %1").arg(e.what()));
        //qCritical() << "Exception while fetching indicators:" << e.what();
        return false;
    }
}

void GameTelemetryWorker::fetchAndDisplayMap() {
    if (!isMatchRunning())
        return;

    QImage mapImage = fetchMapImage();
    if (!mapImage.isNull()) {
        setOriginalMapImage(QPixmap::fromImage(mapImage));
        QString md5 = QCryptographicHash::hash([mapImage] {
            QByteArray ba;
            QBuffer buf(&ba);
            buf.open(QIODevice::WriteOnly);
            mapImage.save(&buf, "PNG");
            return ba;
        }(), QCryptographicHash::Md5).toHex();
        LOG_INFO(QString("Map MD5: %1").arg(md5));
        //qDebug() << "Map MD5:" << md5;
        currentMap = md5;
    }
}

QPixmap GameTelemetryWorker::getOriginalMapImage() const {
    return m_originalMapImage;
}

void GameTelemetryWorker::setOriginalMapImage(const QPixmap &originalMapImage) {
    this->m_originalMapImage = originalMapImage;
}

QJsonObject GameTelemetryWorker::fetchJsonElement(QString url) {
    QNetworkReply *reply = networkManager->get(QNetworkRequest(QUrl(url)));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QJsonObject jsonObject;
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        jsonObject = doc.object();
    } else {
        LOG_WARN(QString("Network error: %1").arg(reply->errorString()));
        //qCritical() << "Network error:" << reply->errorString();
    }
    reply->deleteLater();
    return jsonObject;
}

QJsonArray GameTelemetryWorker::fetchJsonArray(QString url) {
    QNetworkReply *reply = networkManager->get(QNetworkRequest(QUrl(url)));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        LOG_WARN(QString("Network error: %1").arg(reply->errorString()));
        //qCritical() << "Network error:" << reply->errorString();
        throw std::runtime_error(reply->errorString().toStdString());
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isArray()) {
        LOG_WARN("Error: JSON response is not an array");
        //qCritical() << "Error: JSON response is not an array.";
        throw std::runtime_error("JSON response is not an array.");
    }

    return jsonDoc.array();
}

QImage GameTelemetryWorker::fetchMapImage() {
    QNetworkReply *reply = networkManager->get(QNetworkRequest(QUrl(MAP_URL)));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QImage pixmap;
    if (reply->error() == QNetworkReply::NoError) {
        pixmap.loadFromData(reply->readAll());
    } else {
        LOG_WARN(QString("Network error: %1").arg(reply->errorString()));
        //qCritical() << "Network error:" << reply->errorString();
    }
    reply->deleteLater();
    return pixmap;
}

void GameTelemetryWorker::setActivityFromWorker(const QString &state, const QString &details, const QString &logo, time_t epochStartTime, const QString &largeText) {
    emit sendActivityToDiscord(state, details, logo, epochStartTime, largeText);
}
