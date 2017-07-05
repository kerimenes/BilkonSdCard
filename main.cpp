#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindow w;
	QPalette pal;
	pal.setColor(QPalette::Background, Qt::lightGray);
	w.setAutoFillBackground(true);
	w.setPalette(pal);
	w.show();

	return a.exec();
}
