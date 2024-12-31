#include "preferencesdialog.h"
#include "./ui_preferencesdialog.h"
#include <QFileDialog>


PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog),
    settings("sgambe33", "wtplotter")
{
    ui->setupUi(this);
    setWindowTitle("Preferences");
	ui->replayFolderTextEdit->setPlainText(settings.value("replayFolderPath").toString());
	ui->plotSavePathTextEdit->setPlainText(settings.value("plotSavePath").toString());
	ui->autosaveCheck->setChecked(settings.value("autosave").toBool());
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::on_chooseReplayFolder_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Replay Folder"), QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!folderPath.isEmpty()) {
        ui->replayFolderTextEdit->setPlainText(folderPath);
        settings.setValue("replayFolderPath", folderPath);

    }
}

void PreferencesDialog::on_choosePlotSavePath_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Plot Save Path"), QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!folderPath.isEmpty()) {
        ui->plotSavePathTextEdit->setPlainText(folderPath);
		settings.setValue("plotSavePath", folderPath);
    }
}

void PreferencesDialog::on_autosaveCheck_stateChanged(int state)
{
	settings.setValue("autosave", state == Qt::Checked);
}

void PreferencesDialog::on_buttonBox_accepted()
{

}

