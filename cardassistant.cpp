#include "cardassistant.h"

#include <QDir>
#include <QUuid>
#include <QFile>
#include <QTime>
#include <QDebug>
#include <QJsonValue>
#include <QMessageBox>

CardAssistant::CardAssistant()
{
	filename = "/home/kerim/myfs/source-codes/bilkon/BilkonSdCard/creater.json";
	json = new JsonHelper(filename);

	p = new QProcess();
}

int CardAssistant::createConfigScript(const QString &script)
{
	QDir dir;
	QString path = json->value("folder.sdcard_prog");
	dir.setCurrent(path);

	QString config = "config.sh";
	QFile f(config);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
		logFile(QString("error writing test script '%1'").arg(config));
		return -2;
	}
	f.write("#!/bin/bash\n\n");
	f.write("\n");

	/* Versiyon check */
	QString ramdisk = json->value("release_ramdisk");
	QString rootfs = json->value("release_rootfs");
	QString uimage = json->value("release_uimage");
	if (uimage.isEmpty() | rootfs.isEmpty() | ramdisk.isEmpty()) {
		logFile("Release versiyonlari yazilmamış seçili versiyon yok.");
		return -3;
	}
	f.write(QString("ramdisk=%1\n").arg(ramdisk).toUtf8());
	f.write(QString("rootfs=%1\n").arg(rootfs).toUtf8());
	f.write(QString("uimage=%1\n").arg(uimage).toUtf8());

	/* uboot_scripts check */
	int err = ubootScriptsCreate(script);
	if(err) {
		logFile("Not create uboot-scripts");
		return -4;
	}
	QString ubootscr = json->value(QString("list.%1").arg(script)).replace("txt", "scr");
	f.write("\n");
	f.write(QString("ubootscr=%1\n").arg(ubootscr).toUtf8());

	f.close();
	return 0;
}

int CardAssistant::ubootScriptsCreate(const QString &script)
{
	QDir dir;
	QString path = json->value("folder.uboot_scripts");
	dir.setCurrent(path);
	QString bootTxt = json->value(QString("list.%1").arg(script));
	QString bootScr = bootTxt.replace("txt", "scr");
	QString compileBootCmd = QString("mkimage -A arm -O linux -T script -d %1 %2").arg(bootTxt).arg(bootScr);

	int err = processRun(compileBootCmd);
	if(err)
		logFile("PRocess error");

	QString data = p->readAllStandardOutput().data();
	if (data.contains("ARM Linux Script")) {
		qDebug() << "Başarili";
		return 0;
	}
	else {
		qDebug() << "Failed";
		return -1;
	}
}

int CardAssistant::releaseParse(QString release)
{
	int filestate = checkReleaseFile(release);
	int tarstate;
	/* dosya yok ise hatalıysa */
	if (filestate)
		tarstate = untarRelease(release);
	else
		return getConfigPath(release);
	/* tar açma hatalı değilse */
	if (!tarstate)
		return getConfigPath(release);
	return -1;

}

int CardAssistant::checkReleaseFile(const QString release)
{
	QDir dir;
	QString path = json->value("folder.binaries");
	dir.setCurrent(path);

	QString releasename = release.split(".").at(0);
	int err = processRun(QString("ls %1/ramdisk_zero.gz %1/rootfs.tar.gz %1/uImage | wc -l").arg(releasename));
	if (err)
		logFile("Process error");
	QString data = p->readAllStandardOutput().data();
	if (data.split("\n").at(0) == "3")
		return 0;
	else return -1;
}

int CardAssistant::untarRelease(QString release)
{
	QDir dir;
	QString path = json->value("folder.binaries");
	dir.setCurrent(path);

	int err = p->execute("tar", QStringList() << "xf" << release);
	return err;
}

int CardAssistant::getConfigPath(QString release)
{
	QDir dir;
	QString path = json->value("folder.binaries");
	dir.setCurrent(path);

	QString ramdisk;
	QString rootfs;
	QString uimage;
	QString releasename = release.split(".").at(0);

	/* ramdisk_zero path */
	int err = processRun(QString("ls %1/ramdisk_zero.gz").arg(releasename));
	if (err)
		logFile("Process Error");
	QString data = p->readAllStandardOutput().data();
	if (!data.isEmpty())
		ramdisk = QString("%1/%2").arg(path).arg(data);
	/* rootfs path*/
	err = processRun(QString("ls %1/rootfs.tar.gz").arg(releasename));
	if (err)
		logFile("Process Error");
	data = p->readAllStandardOutput().data();
	if(!data.isEmpty())
		rootfs = QString("%1/%2").arg(path).arg(data);

	/* uImage path */
	err = processRun(QString("ls %1/uImage").arg(releasename));
	if (err)
		logFile("Process Error");
	data = p->readAllStandardOutput().data();
	if(!data.isEmpty())
		uimage = QString("%1/%2").arg(path).arg(data);

	err = json->insert("release_uimage", uimage.split("\n").at(0));
	if (err)
		return -3;
	err = json->insert("release_rootfs", rootfs.split("\n").at(0));
	if (err)
		return -3;
	err = json->insert("release_ramdisk", ramdisk.split("\n").at(0));
	if (err)
		return -3;
	return json->save();
}

int CardAssistant::runFormat(const QString &status, const QString &cardtype, QProgressBar *bar)
{
	if (status.contains("gray"))
		return -1;
	QTime t;
	t.start();
	showProgressBar(bar);
	QDir dir;
	QString path = json->value("folder.sdcard_prog");
	dir.setCurrent(path);

	QStringList flds = cardtype.split(" ");
	flds.removeAll("");
	QString card = flds.at(0);
	QString size = flds.at(1);

	if (size.split(",").at(0).toInt() > 8)
		return -1;

	QString cmdSudo = json->value("sudo_cmd");
	QString cmdFormat = QString("./format.sh /dev/%1").arg(card);
	int err =  processRun(cmdSudo.arg(cmdFormat));
	if (err)
		logFile("Process Error ");
	while(!p->readAllStandardOutput().isEmpty()) {
		progress(bar, t.elapsed());
	}

	int i = 0;
	foreach (QString tmp, insertMediaInit()) {
		if (tmp.contains(card))
			i++;
	}
	if (i == 4) {
		progress(bar, 99);
		return 0;
	} else return -1;
}

QStringList CardAssistant::versionTypesInit()
{
	QStringList versiontypes;

	int err = processRun(QString("ls -A %1").arg(json->value("folder.binaries")));
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
	QStringList cardtypes = json->valueObject("list").keys();
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

	if (mediaList.isEmpty()) {
		mediaList << "Bir donanım takın örn:SD kart";
		return mediaList;
	}
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
	QString logpath = json->value("log_path");
	QFile log(logpath);
	if (!log.open(QIODevice::ReadWrite | QIODevice::Append))
		return;

	QTextStream out(&log);
	out << logdata << "\n";
	log.close();
}

void CardAssistant::showProgressBar(QProgressBar *bar, int maxRange)
{
	bar->setVisible(true);
	bar->setValue(0);
	bar->setRange(0, maxRange);
}

void CardAssistant::progress(QProgressBar *bar, int value)
{
	bar->setValue(value);
}
