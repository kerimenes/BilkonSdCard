#include "cardassistant.h"

#include <QUuid>
#include <QDebug>
#include <QFile>
#include <QJsonValue>

CardAssistant::CardAssistant()
{
	appset = ApplicationSettings::instance();
	appset->load("../../BilkonSdCardCreater/creater.json", QIODevice::ReadWrite);
	p = new QProcess();
}

QStringList CardAssistant::versionTypesInit()
{
	QStringList versiontypes;

	int err = processRun(QString("ls -A %1").arg(appset->get("version_path").toString()));
	if (err)
		logFile("Process Error ");
	QString data = p->readAllStandardOutput().data();
	QStringList flds = data.split("\n");
	foreach (QString tmp, flds) {
		if (tmp.isEmpty())
			continue;
		if (tmp.contains("rootfs") | !tmp.contains("release"))
			continue;
		if (tmp.contains(".tar.gz"))
			versiontypes << tmp;
	}
	return versiontypes;
}

QStringList CardAssistant::SDCardTypesInit()
{
	QStringList cardtypes = appset->getValueList("list").toObject().keys();
	return cardtypes;
}

QStringList CardAssistant::insertMediaInit()
{
	int err = processRun("lsblk -o \"NAME,SIZE\"");
	if (err)
		logFile("Process Error ");
	QString data = p->readAllStandardOutput().data();
	QStringList flds = data.split("\n");
	QStringList mediaList;
	foreach (QString tmp, flds) {
		tmp.remove("└─");
		tmp.remove("├─");
		if (tmp.contains("sda"))
			continue;
		if(tmp.contains("sdb"))
			mediaList << tmp;
		if (tmp.contains("mmc"))
			mediaList << tmp;
	}

	if (mediaList.isEmpty())
		return QStringList();

	return mediaList;
}

int CardAssistant::processRun(const QString &cmd)
{
	if (!cmd.contains("|")) {
		p->start(cmd);
	} else {
		QString tmpscr = QString("/tmp/btt_process_%1.sh").arg(QUuid::createUuid().toString().split("-").first().remove("{"));
		QFile f(tmpscr);
		if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
			logFile(QString("error writing test script '%1'").arg(tmpscr));
			return -2;
		}
		f.write("#!/bin/bash\n\n");
		f.write(cmd.toUtf8());
		f.write("\n");
		f.close();
		QProcess::execute(QString("chmod +x %1").arg(tmpscr));
		p->start(tmpscr);
	}
	if (!p->waitForStarted())
		return -1;
	p->waitForFinished(2000);
	return 0;
}

void CardAssistant::logFile(const QString &logdata)
{
	QString logpath = appset->get("log_path").toString();
	QFile log(logpath);
	if (!log.open(QIODevice::ReadWrite | QIODevice::Append))
		return;

	QTextStream out(&log);
	out << logdata << "\n";
	log.close();
}
