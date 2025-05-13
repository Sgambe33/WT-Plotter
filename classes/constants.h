#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>
#include <QMap>
#include <QPair>

namespace Constants
{
    enum class Difficulty : int {
        ARCADE = 0,
        REALISTIC = 5,
        SIMULATOR = 10
    };

    inline const QMap<QString, QString> mapHashes = {
        QPair<QString, QString>(u"b2dc2d1b83752833f70ab6de6782188b", u"avg_sweden"),
        QPair<QString, QString>(u"89fddaea913ced86d4286c99de847777", u"avg_snow_alps"),
        QPair<QString, QString>(u"e1afdf80fdf0576c1e392f2ff8ee4a74", u"avg_syria"),
        QPair<QString, QString>(u"3c4423c909446cfd36e03dabc63ebd7a", u"avg_egypt_sinai"),
        QPair<QString, QString>(u"9d2e06025b8aa2dce50336b43e058271", u"avg_container_port"),
        QPair<QString, QString>(u"76ec6c53ca3ebb0367b3757e6f66c3b8", u"avg_soviet_suburban_snow"),
        QPair<QString, QString>(u"a21c5b0b969845f51f1a2fa34fb23879", u"avg_korea_lake"),
        QPair<QString, QString>(u"ccb69826a962cba2be0eb8860e22ac22", u"avg_netherlands"),
        QPair<QString, QString>(u"de7bafb1e6f35341214c363c845f966b", u"avg_testdrive")

    };

    inline QString lookupMapName(const QString& hash) {
        return mapHashes.value(hash, {});
    }
}

#endif // CONSTANTS_H
