#include <QDateTime>
#include <QFileInfo>
#include "logger.h"

Logger& Logger::instance() {
	static Logger _instance;
	return _instance;
}

Logger::Logger(QObject* parent) : QObject(parent), m_out(&m_file) {}

Logger::~Logger() {
	if (m_file.isOpen())
		m_file.close();
}

bool Logger::init(const QString& filePath) {
	m_file.setFileName(filePath);
	if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		return false;
	m_file.close();
	if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
		return false;
	return true;
}

void Logger::write(const QString& level, const QString& msg, const char* funcName, QObject* sender, const char* filePath, int line) {
	QMutexLocker locker(&m_mutex);

	QString ts = QDateTime::currentDateTime().toString(Qt::ISODate);

	// strip path to just filename.cpp
	QFileInfo fi(QString::fromUtf8(filePath));
	QString fileName = fi.fileName();

	QString cls = sender ? sender->metaObject()->className() : QStringLiteral("Global");

	// format:
	// 2025-05-14T21:26:55 [INFO] (ClassName::function @ filename.cpp:187) My message
	m_out << ts << " [" << level << "] " << "(" << cls << "::" << funcName << " @ " << fileName << ":" << line << ") " << msg << "\n";
	m_out.flush();
}

void Logger::info(const QString& msg, const char* funcName, QObject* sender, const char* filePath, int line) {
	instance().write("INFO", msg, funcName, sender, filePath, line);
}

void Logger::warning(const QString& msg, const char* funcName, QObject* sender, const char* filePath, int line) {
	instance().write("WARN", msg, funcName, sender, filePath, line);
}

void Logger::error(const QString& msg, const char* funcName, QObject* sender, const char* filePath, int line) {
	instance().write("ERROR", msg, funcName, sender, filePath, line);
}