#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	a.setStyle("Fusion");
	QTranslator myappTranslator;
	if (myappTranslator.load(QLocale::system(), "app", "_", ":/i18n"))
        a.installTranslator(&myappTranslator);

	MainWindow w;
	w.show();
	return a.exec();
}
