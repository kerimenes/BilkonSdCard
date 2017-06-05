#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QMessageBox>

#define red "background-color: red"
#define blue "background-color: blue"
#define gray "background-color: gray"
#define green "background-color: green"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	ui->progressBar->setVisible(false);
	ui->statusMedia->setStyleSheet(gray);
	ui->statusTypes->setStyleSheet(gray);
	ui->statusVersion->setStyleSheet(gray);
	card = new CardAssistant();
	ui->mediatypes->addItems(card->insertMediaInit());
	ui->cardtypes->addItems(card->SDCardTypesInit());
	ui->versionList->addItems(card->versionTypesInit());
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_mediatypes_activated(const QString &arg1)
{
	mediatypes = arg1;
	if (mediatypes.contains("sd") | mediatypes.contains("mmc"))	{
		ui->statusMedia->setStyleSheet(green);
	}
}

void MainWindow::on_cardtypes_activated(const QString &arg1)
{
	int err = card->createConfigScript(arg1);
	if (err) {
		QMessageBox::warning(this, "u-boot-scipts", "error");
		ui->statusTypes->setStyleSheet(red);
	}
	ui->statusTypes->setStyleSheet(green);
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

	int err = card->runFormat(ui->statusMedia->styleSheet(),mediatypes, ui->progressBar);
	if (err != 0){
		ui->statusMedia->setStyleSheet(red);
	} else {
		QMessageBox::about(this, trUtf8("SD KART FORMAT"), trUtf8("Format tamamlandı."));
		ui->statusMedia->setStyleSheet(green);
	}
}

