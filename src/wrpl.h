#pragma once

#include <QVariantMap>
#include <QByteArray>
#include <QString>

namespace wrpl {

	/**
	 * Parse a BLK buffer.
	 * @param buf input buffer (raw BLK)
	 * @param outErr on error, set to an error message
	 * @return QVariantMap containing the parsed structure. If an error occurred, returns an empty map and outErr is set.
	 */
	QVariantMap ParseBlk(const QByteArray& buf, QString& outErr);

} // namespace wrpl
