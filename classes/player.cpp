#include "player.h"

// Constructor
Player::Player()
    : kills(0), groundKills(0), navalKills(0), teamKills(0),
    aiKills(0), aiGroundKills(0), aiNavalKills(0), assists(0),
    deaths(0), captureZone(0), damageZone(0), score(0),
    awardDamage(0), missileEvades(0), squadId(0), autoSquad(0), team(0) {}

Player Player::fromJson(const QJsonObject &json) {
    Player player;

    if (json.contains("name") && json["name"].isString()) {
        player.name = json["name"].toString();
    }
    if (json.contains("clanTag") && json["clanTag"].isString()) {
        player.clanTag = json["clanTag"].toString();
    }
    if (json.contains("kills") && json["kills"].isDouble()) {
        player.kills = json["kills"].toInt();
    }
    if (json.contains("groundKills") && json["groundKills"].isDouble()) {
        player.groundKills = json["groundKills"].toInt();
    }
    if (json.contains("navalKills") && json["navalKills"].isDouble()) {
        player.navalKills = json["navalKills"].toInt();
    }
    if (json.contains("teamKills") && json["teamKills"].isDouble()) {
        player.teamKills = json["teamKills"].toInt();
    }
    if (json.contains("aiKills") && json["aiKills"].isDouble()) {
        player.aiKills = json["aiKills"].toInt();
    }
    if (json.contains("aiGroundKills") && json["aiGroundKills"].isDouble()) {
        player.aiGroundKills = json["aiGroundKills"].toInt();
    }
    if (json.contains("aiNavalKills") && json["aiNavalKills"].isDouble()) {
        player.aiNavalKills = json["aiNavalKills"].toInt();
    }
    if (json.contains("assists") && json["assists"].isDouble()) {
        player.assists = json["assists"].toInt();
    }
    if (json.contains("deaths") && json["deaths"].isDouble()) {
        player.deaths = json["deaths"].toInt();
    }
    if (json.contains("captureZone") && json["captureZone"].isDouble()) {
        player.captureZone = json["captureZone"].toInt();
    }
    if (json.contains("damageZone") && json["damageZone"].isDouble()) {
        player.damageZone = json["damageZone"].toInt();
    }
    if (json.contains("score") && json["score"].isDouble()) {
        player.score = json["score"].toInt();
    }
    if (json.contains("awardDamage") && json["awardDamage"].isDouble()) {
        player.awardDamage = json["awardDamage"].toInt();
    }
    if (json.contains("missileEvades") && json["missileEvades"].isDouble()) {
        player.missileEvades = json["missileEvades"].toInt();
    }
    if (json.contains("userId") && json["userId"].isString()) {
        player.userId = json["userId"].toString();
    }
    if (json.contains("squadId") && json["squadId"].isDouble()) {
        player.squadId = json["squadId"].toInt();
    }
    if (json.contains("autoSquad") && json["autoSquad"].isDouble()) {
        player.autoSquad = json["autoSquad"].toInt();
    }
    if (json.contains("team") && json["team"].isDouble()) {
        player.team = json["team"].toInt();
    }

    return player;
}

QString Player::getName() const { return name; }
QString Player::getClanTag() const { return clanTag; }
int Player::getKills() const { return kills; }
int Player::getGroundKills() const { return groundKills; }
int Player::getNavalKills() const { return navalKills; }
int Player::getTeamKills() const { return teamKills; }
int Player::getAiKills() const { return aiKills; }
int Player::getAiGroundKills() const { return aiGroundKills; }
int Player::getAiNavalKills() const { return aiNavalKills; }
int Player::getAssists() const { return assists; }
int Player::getDeaths() const { return deaths; }
int Player::getCaptureZone() const { return captureZone; }
int Player::getDamageZone() const { return damageZone; }
int Player::getScore() const { return score; }
int Player::getAwardDamage() const { return awardDamage; }
int Player::getMissileEvades() const { return missileEvades; }
QString Player::getUserId() const { return userId; }
int Player::getSquadId() const { return squadId; }
int Player::getAutoSquad() const { return autoSquad; }
int Player::getTeam() const { return team; }
