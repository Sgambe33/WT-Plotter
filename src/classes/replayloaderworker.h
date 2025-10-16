#include <QObject>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QSettings>
#include <QStackedWidget>
#include "dbmanager.h"

class ReplayLoaderWorker : public QObject {
	Q_OBJECT
public:
	explicit ReplayLoaderWorker(const QString& folderPath, const QString& dbFilePath, QObject* parent = nullptr);

public slots:
	void loadReplays();

signals:
	void progressUpdated(int progress);
	void finished();

private:
	QString m_folderPath;
	QString m_dbFilePath;
};