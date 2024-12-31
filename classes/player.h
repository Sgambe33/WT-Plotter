#ifndef PLAYER_H
#define PLAYER_H

#include <QString>
#include <QJsonObject>

class Player {
public:
    Player();

    static Player fromJson(const QJsonObject &json);

    // Getters
    QString getName() const;
    QString getClanTag() const;
    int getKills() const;
    int getGroundKills() const;
    int getNavalKills() const;
    int getTeamKills() const;
    int getAiKills() const;
    int getAiGroundKills() const;
    int getAiNavalKills() const;
    int getAssists() const;
    int getDeaths() const;
    int getCaptureZone() const;
    int getDamageZone() const;
    int getScore() const;
    int getAwardDamage() const;
    int getMissileEvades() const;
    QString getUserId() const;
    int getSquadId() const;
    int getAutoSquad() const;
    int getTeam() const;

private:
    QString name;
    QString clanTag;
    int kills;
    int groundKills;
    int navalKills;
    int teamKills;
    int aiKills;
    int aiGroundKills;
    int aiNavalKills;
    int assists;
    int deaths;
    int captureZone;
    int damageZone;
    int score;
    int awardDamage;
    int missileEvades;
    QString userId;
    int squadId;
    int autoSquad;
    int team;
};

#endif // PLAYER_H
