#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class PreferencesDialog; }
QT_END_NAMESPACE

class PreferencesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PreferencesDialog(QWidget* parent = nullptr);
	~PreferencesDialog();

private slots:
	void on_chooseReplayFolder_clicked();
	void on_choosePlotSavePath_clicked();
	void on_autosaveCheck_stateChanged(int state);

private:
	Ui::PreferencesDialog* ui;
	QSettings settings;
};

#endif // PREFERENCESDIALOG_H

