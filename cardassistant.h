#ifndef CARDASSISTANT_H
#define CARDASSISTANT_H

#include <QTimer>
#include <QThread>
#include <QProcess>
#include <QProgressBar>
#include <QNetworkReply>
#include <QNetworkAccessManager>

#include "json/jsonhelper.h"

namespace Ui {
class MainWindow;
}

class CardAssistant: public QObject
{
	Q_OBJECT
public:
	CardAssistant();
	QStringList insertMediaInit();
	QStringList SDCardTypesInit();
	QStringList versionTypesInit();
	int releaseParse(QString release);
	int runFormat(const QString &status, const QString &cardtype);
	int untarRelease(QString release);
	int getConfigPath(QString release);
	int ubootScriptsCreate(const QString &script);
	int createConfigScript(const QString &script);
	int runInstallSd();
	int runInstallNand();
	int runAddNandProg();
	int runAddNewNandProg();
	int runAddMacProg(const qint8 numberOfSd);
	int runProgramLoader(const QString &script);
	void getProgressBar(QProgressBar *pbar);
	QString getInformation();
	void getMediaTypes(const QString &media);
	void setPassword(const QString &pass);
	QString getScriptTypes();
	int createMacfile(const QString &numOfMacFile);
	int getUserPass();

	int downloadMac();
	int downloadRelease();
	QStringList downloadableReleaseList();
	QStringList getReleaseList();
	int downloadReleaseList();
signals:
	void finishedJob();
public slots:
	void timeout();

protected:
	QWidget * parentWidget();
	int processRun(const QString &cmd);
	void logFile(const QString &logdata);
	void showProgressBar(QProgressBar *bar, int maxRange = 99);
	void progress(QProgressBar *bar, int value);
	QJsonObject jsonRead();
	int checkReleaseFile(const QString release);
	QString replaceVariable(QString str);
	int saveDownloadFile(QIODevice *data, QString targetname);
	QNetworkReply *startDownload(QUrl url);
protected slots:
	void readyRead();
	void finished(int state);
	void downloadFinished(QNetworkReply *);
private:
	QTimer *timer;
	QJsonModel *model;
	QProcess *p;
	QString filename;
	JsonHelper *json;
	QStringList datalist;
	QStringList releaseList;
	QProgressBar *bar;
	QNetworkAccessManager manager;
};

#endif // CARDASSISTANT_H
