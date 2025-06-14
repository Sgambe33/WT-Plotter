#include "preferencesdialog.h"
#include "./ui_preferencesdialog.h"
#include <QFileDialog>
#include "classes/logger.h"

PreferencesDialog::PreferencesDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::PreferencesDialog),
	settings("sgambe33", "wtplotter")
{
	ui->setupUi(this);
	setWindowTitle("Preferences");

	ui->replayFolderTextEdit->setPlainText(settings.value("replayFolderPath").toString());
	ui->plotSavePathTextEdit->setPlainText(settings.value("plotSavePath").toString());
	ui->autosaveCheck->setChecked(settings.value("autosave").toBool());
	ui->startMinimizedCheck->setChecked(settings.value("startMinimized").toBool());

	loadLanguages();

	connect(ui->languageComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &PreferencesDialog::onLanguageChanged);
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

void PreferencesDialog::on_startMinimizedCheck_stateChanged(int state)
{
	settings.setValue("startMinimized", state == Qt::Checked);
}

void PreferencesDialog::loadLanguages()
{
	ui->languageComboBox->clear();
	ui->languageComboBox->addItem("American English", "en");

	QStringList translationFiles;

	QDir translationsDir(QCoreApplication::applicationDirPath() + "/translations");
	if (translationsDir.exists()) {
		translationFiles = translationsDir.entryList(QStringList("wtplotter_*.qm"));
	}
	else {
		translationFiles = QDir(QCoreApplication::applicationDirPath()).entryList(QStringList("wtplotter_*.qm"));
	}
	for (const QString& file : translationFiles) {
		QString languageCode = file.mid(10, 2);

		if (languageCode == "en") {
			continue;
		}

		QString languageName = QLocale(languageCode).nativeLanguageName();

		if (!languageName.isEmpty()) {
			ui->languageComboBox->addItem(languageName, languageCode);
		}
	}

	QString currentLanguage = settings.value("language", "en").toString();
	int index = ui->languageComboBox->findData(currentLanguage);
	if (index >= 0) {
		ui->languageComboBox->setCurrentIndex(index);
	}
}

void PreferencesDialog::changeLanguage(const QString& languageCode)
{
	qApp->removeTranslator(&appTranslator);
	QString translationPath;

	QDir translationsDir(QCoreApplication::applicationDirPath() + "/translations");
	if (translationsDir.exists()) {
		translationPath = translationsDir.filePath(QString("wtplotter_%1.qm").arg(languageCode));
	}
	else {
		translationPath = QCoreApplication::applicationDirPath() + QString("/wtplotter_%1.qm").arg(languageCode);
	}
	if (appTranslator.load(translationPath)) {
		qApp->installTranslator(&appTranslator);
	}
	else {
		LOG_WARN(QString("Failed to load translation file: %1").arg(translationPath));
	}

	settings.setValue("language", languageCode);
	ui->retranslateUi(this);
}

void PreferencesDialog::onLanguageChanged(int index)
{
	QString languageCode = ui->languageComboBox->itemData(index).toString();
	changeLanguage(languageCode);
	emit languageChanged(languageCode);
}