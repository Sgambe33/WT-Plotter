#ifndef POSITION_H
#define POSITION_H

#include <QString>

class Position {
public:
    Position(double x = -1, double y = -1, const QString &color = "#FFFFFF", const QString &type = "unknown", const QString &icon = "unknown", qint64 timestamp = 0);

    double x() const;
    double y() const;
    QString color() const;
    QString type() const;
    QString icon() const;
    qint64 timestamp() const;

    bool isCaptureZone() const;
    bool isRespawnBaseTank() const;
    bool isPlayer() const;
    bool isAirfield() const;
    bool isAircraft() const;
    bool isRespawnBaseFighter() const;

    bool isValid() const;

private:
    double m_x;
    double m_y;
    QString m_color;
    QString m_type;
    QString m_icon;
    qint64 m_timestamp;
};

#endif // POSITION_H
