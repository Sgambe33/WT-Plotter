#include "preferencesdialog.h"
#include "./ui_preferencesdialog.h"
#include <QFileDialog>

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

void PreferencesDialog::loadLanguages()
{
    ui->languageComboBox->clear();
    ui->languageComboBox->addItem("American English", "en");

    QStringList translationFiles = QDir(QCoreApplication::applicationDirPath()).entryList(QStringList("*.qm"));

    for (const QString& file : translationFiles) {
        qDebug() << "Found translation file:" << file;

        QString languageCode = file.mid(10, 2);

        if (languageCode == "en") {
            qDebug() << "Skipping English translation file:" << file;
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

    if (languageCode != "en") {
        QString translationFile = QCoreApplication::applicationDirPath() + QString("/wtplotter_%1.qm").arg(languageCode);
        if (appTranslator.load(translationFile)) {
            qApp->installTranslator(&appTranslator);
        }
        else {
            qWarning() << "Failed to load translation file:" << translationFile;
        }
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

