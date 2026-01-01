#ifndef LOGGER_H
#define LOGGER_H

#include <QFile>
#include <QMutex>
#include <QObject>
#include <QTextStream>

class Logger : public QObject {
	Q_OBJECT
public:
	static Logger& instance();
	bool init(const QString& filePath);

	// full write API: pass level, message, function-name, sender-object, source-file, and line
	void write(const QString& level, const QString& msg, const char* funcName, QObject* sender, const char* filePath, int line);

	static void info(const QString& msg, const char* funcName, QObject* sender, const char* filePath, int line);
	static void warning(const QString& msg, const char* funcName, QObject* sender, const char* filePath, int line);
	static void error(const QString& msg, const char* funcName, QObject* sender, const char* filePath, int line);

private:
	explicit Logger(QObject* parent = nullptr);
	~Logger();

	QFile m_file;
	QTextStream m_out;
	QMutex m_mutex;
};

// ——— Macros ———

// inside any QObject-derived class
#define LOG_INFO(msg) Logger::info(msg, __FUNCTION__, this, __FILE__, __LINE__)
#define LOG_WARN(msg) Logger::warning(msg, __FUNCTION__, this, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::error(msg, __FUNCTION__, this, __FILE__, __LINE__)

// from global (non-QObject) code, or lambdas where you have no 'this'
#define LOG_INFO_GLOBAL(msg) Logger::info(msg, __FUNCTION__, nullptr, __FILE__, __LINE__)
#define LOG_WARN_GLOBAL(msg) Logger::warning(msg, __FUNCTION__, nullptr, __FILE__, __LINE__)
#define LOG_ERROR_GLOBAL(msg) Logger::error(msg, __FUNCTION__, nullptr, __FILE__, __LINE__)

#endif  // LOGGER_H