#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>
#include <QMap>

static const QMap<QString, QString> MAPS = {
    {"map1", "Location 1"},
    {"map2", "Location 2"},
    {"map3", "Location 3"}
};

enum class Difficulty {
    ARCADE = 0,
    REALISTIC = 5,
    SIMULATOR = 10
};

#endif // CONSTANTS_H