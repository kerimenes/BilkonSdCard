#include "cardassistant.h"

#include <QDir>
#include <QUuid>
#include <QFile>
#include <QTime>
#include <QDebug>
#include <QJsonValue>
#include <QMessageBox>
#include <QInputDialog>
#include <QNetworkRequest>
#include <QNetworkInterface>

CardAssistant::CardAssistant()
{
	filename = "creater.json";
	json = new JsonHelper(filename);

	p = new QProcess();
}

QStringList CardAssistant::getReleaseList()
{
	return releaseList;
}

int CardAssistant::downloadReleaseList()
{
	if(!QDir::setCurrent(json->value("folder.binaries"))) {
		logFile("CreateConfig: not change directory");
		return -3;
	}

	QUrl url("http://fourier.bilkon.arge/releases/file_list.txt");
	startDownload(url);
}

int CardAssistant::downloadMac()
{
	if(!QDir::setCurrent(json->value("folder.tools"))) {
		logFile("CreateConfig: not change directory");
		return -3;
	}

	QUrl url("http://fourier.bilkon.arge/MAC/random_mac_list.txt");
	startDownload(url);
}

QNetworkReply* CardAssistant::startDownload(QUrl url)
{
	connect(&manager, SIGNAL(finished(QNetworkReply*)), SLOT(downloadFinished(QNetworkReply*)));
	QNetworkReply *reply = manager.get(QNetworkRequest(url));
	qDebug() << "starting download";
	return reply;
}

int CardAssistant::getUserPass()
{
	QString pass = json->value("PASS");
	if (pass.isEmpty())
		return -1;
	return 0;
}

int CardAssistant::saveDownloadFile(QIODevice* data, QString targetname)
{
	QFile file(targetname);
	if (!file.open(QIODevice::WriteOnly)) {
		logFile( "Could not open writing");
		return -2;
	}
	file.write(data->readAll());
	file.close();
	return 0;
}

void CardAssistant::downloadFinished(QNetworkReply* reply)
{
	if(reply->error() == QNetworkReply::NoError){
		if (reply->url().toString().contains("mac_list.txt"))
			logFile("Mac address Download finished");
			if (saveDownloadFile(reply, "macs.txt"))
				return;
		if (reply->url().toString().contains("file_list.txt")) {
			releaseList.clear();
			logFile("Release Download finished");
			qDebug() << "download finished";
			QString data = reply->readAll();
			qDebug() << data;
			foreach (QString tmp, data.split("\n")) {
				qDebug() << "release list " << tmp;
				if(!tmp.contains("release"))
					continue;
				releaseList << tmp;
			}
			/*qDebug() <<*/ QInputDialog::getItem(parentWidget(), "Select Release File", "Releases", releaseList);
		}
	} else {
		qDebug() << reply->errorString();
	}
}

int CardAssistant::runProgramLoader(const QString &script)
{
	QString type = json->value(QString("list.%1").arg(script));
	showProgressBar(bar);

	int err;
	if (type == "rescue_erase.txt") {
		err = runInstallSd();
		progress(bar, 99);
		return err;
	}
	if (type == "boot_zero_SD_mac.txt") {

	}
	if (type == "boot_zero_prog.txt") {
		err = runInstallNand();					//
		progress(bar, 99);
		return err;
	}
	if (type == "boot_zero_sd.txt") {
		err = runInstallSd();						//
		progress(bar, 99);
		return err;
	}
	if (type == "boot_zero_sd_prog.txt") {
		err = runInstallNand();
		progress(bar, 78);
		if(!err) {
			err = runAddNewNandProg();
			progress(bar, 99);
			return err;
		}
		else return err;
	}
	if (type == "rescue_erase_cammgr.txt") {
		err = runInstallNand();
		progress(bar, 99);
	}
	if (type == "boot_zero_sd_new_mtd.txt") {
		err = runInstallNand();
		progress(bar, 81);
		if(!err) {
			err = runAddNewNandProg();
			progress(bar, 99);
			return err;
		}
		else return err;
	}
	return 0;
}

int CardAssistant::runAddMacProg(const qint8 numberOfSd)
{
	if(!QDir::setCurrent(json->value("folder.sdcard_prog"))) {
		logFile("CreateConfig: not change directory");
		return -3;
	}

	if (numberOfSd == 0) {
		logFile("Error number of sd");
		return -6;
	}

	QString pass = json->value("PASS");
	QString device = QString("%1-2").arg(json->value("current.media")).remove("-");
	QString cmd = QString("echo %1 | sudo -S ./add_macprog_sd.sh /dev/%2 ../mgen %3 2>&1").arg(pass).arg(device).arg(QString::number(numberOfSd));

	int err = processRun(cmd);
	if (err) {
		logFile("Process Error");
		return -2;
	} else
		logFile(QString("Process %1").arg(cmd));
	QString data = p->readAllStandardOutput().data();
	if(data.contains("cp --help")) {
		logFile("Copy Error");
		logFile(data);
		return -5;
	} else if(data.contains("wrong fs type")) {
		logFile("Mount Error");
		logFile(data);
		return -5;
	}
	return 0;
}

int CardAssistant::runAddNewNandProg()
{
	if(!QDir::setCurrent(json->value("folder.sdcard_prog"))) {
		logFile("CreateConfig: not change directory");
		return -3;
	}

	QString pass = json->value("PASS");
	QString device = QString("%1-2").arg(json->value("current.media")).remove("-");
	QString cmd = QString("echo %1 | sudo -S ./add_newnandprog_sd.sh /dev/%2 2>&1").arg(pass).arg(device);

	int err = processRun(cmd);
	if (err) {
		logFile("Process Error");
		return -2;
	} else
		logFile(QString("Process %1").arg(cmd));
	QString data = p->readAllStandardOutput().data();
	if(data.contains("cp --help")) {
		logFile("Copy Error");
		logFile(data);
		return -5;
	} else if(data.contains("wrong fs type")) {
		logFile("Mount Error");
		logFile(data);
		return -5;
	}
	return 0;
}

int CardAssistant::runAddNandProg()
{
	if(!QDir::setCurrent(json->value("folder.sdcard_prog"))) {
		logFile("CreateConfig: not change directory");
		return -3;
	}

	QString pass = json->value("PASS");
	QString device = QString("%1-2").arg(json->value("current.media")).remove("-");
	QString cmd = QString("echo %1 | sudo -S ./add_nandprog_sd.sh /dev/%2 2>&1").arg(pass).arg(device);

	int err = processRun(cmd);
	if (err) {
		logFile("Process Error");
		return -2;
	} else
		logFile(QString("Process %1").arg(cmd));
	QString data = p->readAllStandardOutput().data();
	if(data.contains("cp --help")) {
		logFile("Copy Error");
		logFile(data);
		return -5;
	} else if(data.contains("wrong fs type")) {
		logFile("Mount Error");
		logFile(data);
		return -5;
	}
	return 0;
}

int CardAssistant::runInstallNand()
{
	if(!QDir::setCurrent(json->value("folder.sdcard_prog"))) {
		logFile("CreateConfig: not change directory");
		return -3;
	}

	QString pass = json->value("PASS");
	QString device = json->value("current.media");
	QString cmd = QString("echo %1 | sudo -S ./install_nand.sh /dev/%2 2>&1").arg(pass).arg(device);

	int err = processRun(cmd);
	if (err) {
		logFile("Process Error");
		return -2;
	} else
		logFile(QString("Process %1").arg(cmd));
	QString data = p->readAllStandardOutput().data();
	if (data.contains("missing") | data.contains("error") | data.contains("target")) {
		logFile("Install Sd Scripts Error");
		logFile(data);
		return -4;
	}
	return 0;
}

int CardAssistant::runInstallSd()
{
	if(!QDir::setCurrent(json->value("folder.sdcard_prog"))) {
		logFile("CreateConfig: not change directory");
		return -3;
	}
	QString pass = json->value("PASS");
	QString device = json->value("current.media");
	QString cmd = QString("echo %1 | sudo -S ./install_sd.sh /dev/%2 2>&1").arg(pass).arg(device);

	int err = processRun(cmd);
	if (err) {
		logFile("Process Error");
		return -2;
	} else
		logFile(QString("Process %1").arg(cmd));

	QString data = p->readAllStandardOutput().data();
	if (data.contains("missing") | data.contains("error") | data.contains("target")) {
		logFile("Install Sd Scripts Error");
		logFile(data);
		return -4;
	}
	return 0;
}

int CardAssistant::createMacfile(const QString &numOfMacFile)
{
	json->insert("current.num_of_mac_files", numOfMacFile);

	if(!QDir::setCurrent(json->value("folder.tools"))) {
		logFile("Format: not change directory");
		return -3;
	}

	if(processRun("rm -rf mgen"))
		return -2;
	if (!processRun("cat macs.txt | wc -l")) {
		QString number = p->readAllStandardOutput().data();
		if (numOfMacFile.toInt() * 250 > number.toInt()) {
			logFile("Create Mac File: Error ~  insufficient number of mac");
			return -6;
		}
	}

	QString cmd = QString("./create_macaddr.sh macs.txt mgen/ 250 %1").arg(numOfMacFile);
	int err = processRun(cmd);
	if(err) {
		logFile("Process Error");
		return -2;
	} else
		logFile(QString("Process %1").arg(cmd));

	QString data = p->readAllStandardOutput().data();
	if(data.contains("Destination already exists, doing nothing")) {
		logFile("Create Mac File: Error ~ Destination already exists, doing nothing");
		return -4;
	}
	if (data.contains("Please select a source file")) {
		logFile("Create Mac File: Error ~ Please select a source file");
		return -5;
	}
	if (!processRun("ls mgen/ | wc -l")) {
		QString number = p->readAllStandardOutput().data();
		if (number.toInt() != numOfMacFile.toInt()) {
			logFile("Create Mac File: Error ~  insufficient number of mac");
			return -6;
		}
	}

	qint16 numberOfMacs = 250 * numOfMacFile.toInt();
	if(!processRun(QString("head -%1 macs.txt >> oldmacs.txt 2>&1").arg(QString::number(numberOfMacs)))) {
		qDebug() << p->readAllStandardOutput().data();
		if (!processRun(QString("sed -i '1,%1d' macs.txt 2>&1").arg(numberOfMacs))) {
			qDebug() << QString("sed -i '1,%1d' macs.txt").arg(numberOfMacs);
			logFile("Deleting Macs and fixed...");
		} else
			return -2;
	} else
		return -2;
	return 0;
}

void CardAssistant::readyRead()
{
	datalist << p->readAllStandardOutput();
}

void CardAssistant::timeout()
{

}

QWidget *CardAssistant::parentWidget()
{
	return (QWidget *)parent();
}

void CardAssistant::finished(int state)
{
	if (state == 0)
		emit finishedJob();
}

int CardAssistant::createConfigScript(const QString &script)
{
	if(!QDir::setCurrent(json->value("folder.sdcard_prog"))) {
		logFile("CreateConfig: not change directory");
		return -3;
	}
	QString config = "config.sh";
	QFile f(config);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
		logFile(QString("error writing test script '%1'").arg(config));
		return -2;
	}
	f.write("#!/bin/bash\n\n");
	f.write("\n");

	/* Versiyon check */
	QString ramdisk = json->value("release.ramdisk");
	QString rootfs = json->value("release.rootfs");
	QString uimage = json->value("release.uimage");

	if (uimage.isEmpty() | rootfs.isEmpty() | ramdisk.isEmpty()) {
		logFile("Release versiyonlari yazilmamış seçili versiyon yok.");
		return -3;
	}
	f.write(QString("ramdisk=%1\n").arg(ramdisk).toUtf8());
	f.write(QString("rootfs=%1\n").arg(rootfs).toUtf8());
	f.write(QString("kernel=%1\n").arg(uimage).toUtf8());

	/* uboot_scripts check */
	int err = ubootScriptsCreate(script);
	if(err) {
		logFile("Not create uboot-scripts");
		return -4;
	}
	QString ubootscr = json->value(QString("list.%1").arg(script)).replace("txt", "scr");
	f.write("\n");
	f.write(QString("ubootscr=u-boot-scripts/%1\n").arg(ubootscr).toUtf8());

	f.close();
	return 0;
}

int CardAssistant::ubootScriptsCreate(const QString &script)
{
	if(!QDir::setCurrent(json->value("folder.uboot_scripts"))) {
		logFile("U-boot-Create: not change directory");
		return -3;
	}
	QString bootTxt = json->value(QString("list.%1").arg(script));
	QString tmp = bootTxt;
	QString bootScr = tmp.replace("txt", "scr");
	processRun(QString("rm %1").arg(bootScr));

	QString compileBootCmd = QString("mkimage -A arm -O linux -T script -d %1 %2").arg(bootTxt).arg(bootScr);

	int err = processRun(compileBootCmd);
	if(err)
		logFile("Process error");

	QString data = p->readAllStandardOutput().data();
	if (data.contains("ARM Linux Script")) {
		json->insert("current.sd_types", script);
		return 0;
	}
	else {
		logFile("Error = uboot Scripts Create");
		return -1;
	}
}

int CardAssistant::releaseParse(QString release)
{
	int tarstate;
	int filestate = checkReleaseFile(release);
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
	if(!QDir::setCurrent(json->value("folder.binaries"))) {
		logFile("CheckReleaseFile: not change directory");
		return -3;
	}

	QString releasename = release.split(".").first();
	int err = processRun(QString("ls %1/ramdisk_zero.gz %1/rootfs.tar.gz %1/uImage | wc -l").arg(releasename));
	if (err)
		logFile("Process error");
	QString data = p->readAllStandardOutput().data();
	if (data.split("\n").first() == "3") {
		logFile("CheckReleaseFile: File is open");
		return 0;
	}
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
	QString path = json->value("folder.binaries");
	if(!QDir::setCurrent(json->value("folder.binaries"))) {
		logFile("ConfigPath: not change directory");
		return -3;
	}
	QString ramdisk;
	QString rootfs;
	QString uimage;
	QString releasename = release.split(".").first(); /* split tar */
	/* Get release_date */
	json->insert("current.date",releasename.split("_").at(1));

	/* Get Firmware_version */
	processRun(QString("cat %1/encsoft/device_info.json | grep -i \"firmware_version\"").arg(releasename));
	QString firmware = p->readAllStandardOutput().data();
	if (firmware.isEmpty())
		json->insert("current.firmware_version", releasename);
	else
		json->insert("current.firmware_version", firmware.split(":").at(1));
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

	err = json->insert("release.uimage", uimage.split("\n").at(0));
	if (err)
		return -3;
	err = json->insert("release.rootfs", rootfs.split("\n").at(0));
	if (err)
		return -3;
	err = json->insert("release.ramdisk", ramdisk.split("\n").at(0));
	if (err)
		return -3;
	return 0;
}

int CardAssistant::runFormat(const QString &status, const QString &cardtype)
{
	if (status.contains("gray"))
		return -1;
	QTime t;
	t.start();
	showProgressBar(bar);
	if(!QDir::setCurrent(json->value("folder.sdcard_prog"))) {
		logFile("Format: not change directory");
		return -3;
	}

	QStringList flds = cardtype.split(" ");
	flds.removeAll("");
	QString card = flds.at(0);
	QString size = flds.at(1);

	if (size.split(",").at(0).toInt() > 8)
		return -1;
	QString pass = json->value("PASS");
	QString cmdFormat = QString("./format.sh /dev/%1").arg(card);
	int err =  processRun(QString("echo %1 | sudo -S %2 2>&1").arg(pass).arg(cmdFormat));
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
	int err = processRun("lsblk -o NAME,SIZE");
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
	p->waitForFinished();

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

void CardAssistant::getProgressBar(QProgressBar *pbar)
{
	bar = pbar;
}

void CardAssistant::setPassword(const QString &pass)
{
	json->insert("PASS", pass);
}

void CardAssistant::getMediaTypes(const QString &media)
{
	json->insert("current.media", media);
}

QString CardAssistant::getScriptTypes()
{
	return json->value("current.sd_types");
}

QString CardAssistant::getInformation()
{
	QByteArray ba;
	ba.append( "Uygulanacak ayarlar listelenmiştir. Onaylıyorsanız 'Yes' tuşuna basınız.");
	ba.append("Onaylamıyorsanız 'No' ile lütfen seçimlerinizi tekrar yapınız.");
	ba.append("\n");
	ba.append(QString("Release Tarihi = %1").arg(json->value("current.date")));
	ba.append("\n");
	ba.append(QString("Release Versiyon Numarası = %1").arg(json->value("current.firmware_version")));
	ba.append(QString("Uygulanacak olan Sd Kart çeşiti = \"%1\"").arg(json->value("current.sd_types")));
	ba.append("\n");
	ba.append(QString("Seçilen SD Kart = \"%1\"").arg(json->value("current.media")));
	return ba.data();
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
