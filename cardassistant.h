#ifndef CARDASSISTANT_H
#define CARDASSISTANT_H

#include <QProcess>
#include <QProgressBar>

#include "qjsonmodel.h"

class CardAssistant: public QObject
{
	Q_OBJECT
public:
	CardAssistant();
	QStringList insertMediaInit();
	QStringList SDCardTypesInit();
	QStringList versionTypesInit();
	int releaseParse(QString release);
	int runFormat(const QString &status, const QString &cardtype, QProgressBar *bar);
	int untarRelease(QString release);
	int getConfigPath(QString release);
	int ubootScriptsCreate(const QString &script);
protected:
	int processRun(const QString &cmd);
	void logFile(const QString &logdata);
	void showProgressBar(QProgressBar *bar, int maxRange = 99);
	void progress(QProgressBar *bar, int value);
	QJsonObject jsonRead();
	int saveJson(QJsonObject root);
	int checkReleaseFile(const QString release);
private:
	QJsonModel *model;
	QProcess *p;
	QString filename;
	QJsonObject obj;
};

#endif // CARDASSISTANT_H
