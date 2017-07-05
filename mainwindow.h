#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <cardassistant.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

protected:
	int waitForPassword();
	void createActions();
	void createMenus();
protected slots:
	void timeout();
	void menuMacUpdate();
	void menuReleaseDownload();
private slots:
	void on_mediatypes_activated(const QString &arg1);

	void on_cardtypes_activated(const QString &arg1);

	void on_buttonFormat_clicked();

	void on_versionList_activated(const QString &arg1);

	void on_pushButton_3_clicked();

private:
	Ui::MainWindow *ui;
	CardAssistant *card;
	QString mediatypes;
	QTimer *timer;
	QMenu *menuFile;
	QMenu *menuEdit;
	QAction *actMacUpdate;
	QAction *actSdkUpdate;
	QAction *actReleaseDownload;
};

#endif // MAINWINDOW_H
