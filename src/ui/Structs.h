//
// Created by Cosimo on 23/02/2026.
//

#ifndef WTPLOTTER_STRUCTS_H
#define WTPLOTTER_STRUCTS_H

#include <QString>

struct UiPlayerData {
    QString name;
    QString clanTag;
    QString displayName; // name + clan tag
    QString country;
    QString platform;
    int team;
    int score;
    int airKills;
    int groundKills;
    int navalKills;
    int assists;
    int caps;
    int aiKillsTotal; // Sum of ai, aiGround, aiNaval
    int damage;
    int bombing;
    int deaths;
    QString userId;
    QString lineup;
};

#endif //WTPLOTTER_STRUCTS_H