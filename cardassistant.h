#ifndef CARDASSISTANT_H
#define CARDASSISTANT_H

#include <QProcess>
#include <QProgressBar>

#include "json/jsonhelper.h"

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
	int createConfigScript(const QString &script);
protected:
	int processRun(const QString &cmd);
	void logFile(const QString &logdata);
	void showProgressBar(QProgressBar *bar, int maxRange = 99);
	void progress(QProgressBar *bar, int value);
	QJsonObject jsonRead();
	int checkReleaseFile(const QString release);
	QString replaceVariable(QString str);
private:
	QJsonModel *model;
	QProcess *p;
	QString filename;
	JsonHelper *json;
};

#endif // CARDASSISTANT_H
