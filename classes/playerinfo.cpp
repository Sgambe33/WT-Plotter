#include "playerinfo.h"
#include <QJsonValue>

// Constructor
PlayerInfo::PlayerInfo()
    : team(0), slot(0), mrank(0), autoSquad(false), waitTime(0), tier(0), squad(0), rank(0) {}

// Static method to create a PlayerInfo object from JSON
PlayerInfo PlayerInfo::fromJson(const QJsonObject &playerInfoObject) {
    PlayerInfo player;

    if (playerInfoObject.contains("name") && playerInfoObject["name"].isString()) {
        player.name = playerInfoObject["name"].toString();
    }
    if (playerInfoObject.contains("team") && playerInfoObject["team"].isDouble()) {
        player.team = playerInfoObject["team"].toInt();
    }
    if (playerInfoObject.contains("clanTag") && playerInfoObject["clanTag"].isString()) {
        player.clanTag = playerInfoObject["clanTag"].toString();
    }
    if (playerInfoObject.contains("platform") && playerInfoObject["platform"].isString()) {
        player.platform = playerInfoObject["platform"].toString();
    }
    if (playerInfoObject.contains("id") && playerInfoObject["id"].isString()) {
        player.id = playerInfoObject["id"].toString();
    }
    if (playerInfoObject.contains("slot") && playerInfoObject["slot"].isDouble()) {
        player.slot = playerInfoObject["slot"].toInt();
    }
    if (playerInfoObject.contains("country") && playerInfoObject["country"].isString()) {
        player.country = playerInfoObject["country"].toString();
    }
    if (playerInfoObject.contains("mrank") && playerInfoObject["mrank"].isDouble()) {
        player.mrank = playerInfoObject["mrank"].toInt();
    }
    if (playerInfoObject.contains("auto_squad") && playerInfoObject["auto_squad"].isBool()) {
        player.autoSquad = playerInfoObject["auto_squad"].toBool();
    }
    if (playerInfoObject.contains("wait_time") && playerInfoObject["wait_time"].isDouble()) {
        player.waitTime = playerInfoObject["wait_time"].toInt();
    }
    if (playerInfoObject.contains("clanId") && playerInfoObject["clanId"].isString()) {
        player.clanId = playerInfoObject["clanId"].toString();
    }
    if (playerInfoObject.contains("tier") && playerInfoObject["tier"].isDouble()) {
        player.tier = playerInfoObject["tier"].toInt();
    }
    if (playerInfoObject.contains("squad") && playerInfoObject["squad"].isDouble()) {
        player.squad = playerInfoObject["squad"].toInt();
    }
    if (playerInfoObject.contains("rank") && playerInfoObject["rank"].isDouble()) {
        player.rank = playerInfoObject["rank"].toInt();
    }
    if (playerInfoObject.contains("crafts_info") && playerInfoObject["crafts_info"].isObject()) {
        player.craftsInfo = player.parseCraftsInfo(playerInfoObject["crafts_info"].toObject());
    }

    return player;
}

// Method to parse craftsInfo from JSON
QList<CraftInfo> PlayerInfo::parseCraftsInfo(const QJsonObject &craftInfoObject) {
    QList<CraftInfo> craftsList;
    for (const QString &key : craftInfoObject.keys()) {
        if (key.compare("__array", Qt::CaseInsensitive) == 0) {
            continue;
        }

        QJsonValue value = craftInfoObject[key];
        if (value.isObject()) {
            CraftInfo craft = CraftInfo::fromJson(value.toObject());
            craftsList.append(craft);
        }
    }
    return craftsList;
}

// Getters
QString PlayerInfo::getName() {
    return name;
}

int PlayerInfo::getRank() {
    return rank;
}

QString PlayerInfo::getClanTag() {
    return clanTag;
}

QString PlayerInfo::getPlatform() {
    return platform;
}

QList<CraftInfo> PlayerInfo::getCraftsInfo() {
    return craftsInfo;
}

QString PlayerInfo::getUserId() {
    return id;
}