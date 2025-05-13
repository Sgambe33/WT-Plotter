#include "position.h"
#include <stdexcept>

Position::Position(double x, double y, const QString& color, const QString& type, const QString& icon, qint64 timestamp)
	: m_x(x), m_y(y), m_color(color), m_type(type), m_icon(icon), m_timestamp(timestamp) {
	if (x < 0 || x > 1) {
		throw std::invalid_argument("x must be between 0 and 1");
	}
	if (y < 0 || y > 1) {
		throw std::invalid_argument("y must be between 0 and 1");
	}
}

double Position::x() const {
	return m_x;
}

double Position::y() const {
	return m_y;
}

QString Position::color() const {
	return m_color;
}

QString Position::type() const {
	return m_type;
}

QString Position::icon() const {
	return m_icon;
}

qint64 Position::timestamp() const {
	return m_timestamp;
}

bool Position::isCaptureZone() const {
	return m_type.compare("capture_zone", Qt::CaseInsensitive) == 0;
}

bool Position::isRespawnBaseTank() const {
	return m_type.compare("respawn_base_tank", Qt::CaseInsensitive) == 0;
}

bool Position::isValid() const {
	return m_x >= 0 && m_x <= 1 && m_y >= 0 && m_y <= 1;
}
