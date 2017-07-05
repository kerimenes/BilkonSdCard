#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>

#define red "background-color: red"
#define blue "background-color: blue"
#define gray "background-color: gray"
#define green "background-color: green"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	createActions();
	createMenus();
	ui->progressBar->setVisible(false);
	ui->statusMedia->setStyleSheet(gray);
	ui->statusTypes->setStyleSheet(gray);
	ui->statusVersion->setStyleSheet(gray);
	card = new CardAssistant();
	if (waitForPassword())
		return;
	card->getProgressBar(ui->progressBar);
	ui->mediatypes->addItems(card->insertMediaInit());
	ui->cardtypes->addItems(card->SDCardTypesInit());
	ui->versionList->addItems(card->versionTypesInit());

	timer = new QTimer();
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
	timer->start(2000);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::timeout()
{
	timer->setInterval(2000);
	ui->mediatypes->clear();
	ui->mediatypes->addItems(card->insertMediaInit());
}

int MainWindow::waitForPassword()
{
	if (card->getUserPass()) {
		QString pass = QInputDialog::getText(this, "Giriş Ekranı", "Kullanıcı Şifrenizi Giriniz", QLineEdit::Normal);
		if (pass.isEmpty())
			return -1;
		else
			card->setPassword(pass);
		return 0;
	}
	else
		return 0;
}

void MainWindow::createMenus()
{
	menuFile = ui->menuBar->addMenu(tr("&File"));
	menuFile->addAction(actMacUpdate);
	menuFile->addSeparator();
	menuFile->addAction(actSdkUpdate);
	menuFile->addSeparator();
	menuFile->addAction(actReleaseDownload);
}

void MainWindow::createActions()
{
	actMacUpdate = new QAction("Mac", this);
	//	actMac->setShortcuts(QKeySequence::New);
	actMacUpdate->setStatusTip("Download Mac new files");
	connect(actMacUpdate, SIGNAL(triggered(bool)), this, SLOT(menuMacUpdate()));

	actReleaseDownload = new QAction("Download Release", this);
	actReleaseDownload->setStatusTip("Download Release");
	connect(actReleaseDownload, SIGNAL(triggered(bool)), this, SLOT(menuReleaseDownload()));
}

void MainWindow::menuReleaseDownload()
{
	card->downloadReleaseList();
}

void MainWindow::menuMacUpdate()
{
	card->downloadMac();
}

void MainWindow::on_mediatypes_activated(const QString &arg1)
{
	mediatypes = arg1;
	if (mediatypes.contains("sd") | mediatypes.contains("mmc"))	{
		ui->statusMedia->setStyleSheet(green);
		card->getMediaTypes(mediatypes.split(" ").first());
	}
}

void MainWindow::on_cardtypes_activated(const QString &arg1)
{
	int err = card->createConfigScript(arg1);
	if (err) {
		QMessageBox::warning(this, "u-boot-scipts", "error");
		ui->statusTypes->setStyleSheet(red);
	} else
		ui->statusTypes->setStyleSheet(green);
	if(ui->statusTypes->styleSheet() == green) {
		if (arg1.contains("poe")) {
			QString numOfMacFiles = QInputDialog::getText(this, tr("Yapmak istediğiniz Sd kart sayısını giriniz"),
														  tr(" SD Kart Sayısı"));
			err = card->createMacfile(numOfMacFiles);
			if (err == -6){
				QMessageBox::warning(this, "Error", "Yetersiz MAC addresi");
				return;
			}
			if (err == -2){
				QMessageBox::warning(this, "Error", "Komut Çalıştırma hatası");
				return;
			}
		}
	}
}
void MainWindow::on_versionList_activated(const QString &arg1)
{
	if (card->releaseParse(arg1)) {
		QMessageBox::warning(this, "Check Relese", "error");
		ui->statusVersion->setStyleSheet(red);
	}
	else
		ui->statusVersion->setStyleSheet(green);
}

void MainWindow::on_buttonFormat_clicked()
{
	if (mediatypes.isEmpty())
		mediatypes = ui->mediatypes->currentText();
	if (mediatypes.contains("sd") | mediatypes.contains("mmc"))	{
		ui->statusMedia->setStyleSheet(green);
	}
	else ui->statusMedia->setStyleSheet(gray);

	if (mediatypes.contains("1") | mediatypes.contains("2") | mediatypes.contains("3")) {
		QMessageBox::warning(this, trUtf8("SD KART FORMAT"), trUtf8("Format tamamlanamadı: Lütfen düzgün bir kart tipi seçiniz."));
		ui->statusMedia->setStyleSheet(red);
		return;
	}

	int err = card->runFormat(ui->statusMedia->styleSheet(), mediatypes);
	if (err != 0){
		ui->statusMedia->setStyleSheet(red);
	} else {
		QMessageBox::about(this, trUtf8("SD KART FORMAT"), trUtf8("Format tamamlandı."));
		ui->statusMedia->setStyleSheet(green);
	}
}


void MainWindow::on_pushButton_3_clicked()
{
	QMessageBox::StandardButton reply = QMessageBox::question(this, "Açıklama", card->getInformation());
	if (reply == QMessageBox::Yes)
		qDebug() << card->runProgramLoader(card->getScriptTypes());
}
