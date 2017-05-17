#ifndef CARDASSISTANT_H
#define CARDASSISTANT_H

#include <QProcess>
#include <settings/applicationsettings.h>

class CardAssistant
{
public:
	CardAssistant();
	QStringList insertMediaInit();
	QStringList SDCardTypesInit();
	QStringList versionTypesInit();
protected:
	int processRun(const QString &cmd);
	void logFile(const QString &logdata);
private:
	ApplicationSettings *appset;
	QProcess *p;
};

#endif // CARDASSISTANT_H
