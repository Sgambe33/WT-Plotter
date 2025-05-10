#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>
#include <QMap>

namespace Constants {

    enum class Difficulty {
        ARCADE = 0,
        REALISTIC = 5,
        SIMULATOR = 10
    };

    // Declare the function here (no definition)
    QMap<QString, QString> hashOfMap();
    //b2dc2d1b83752833f70ab6de6782188b -> avg_sweden
    //89fddaea913ced86d4286c99de847777 -> avg_snow_alps
    //e1afdf80fdf0576c1e392f2ff8ee4a74 -> avg_syria
    //3c4423c909446cfd36e03dabc63ebd7a -> avg_egypt_sinai
    //9d2e06025b8aa2dce50336b43e058271 -> avg_container_port
}

#endif // CONSTANTS_H
