#include "mainwindow.h"
#include "ui_mainwindow.h"

#define red "background-color: red"
#define green "background-color: green"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	ui->statusMedia->setStyleSheet(red);
	ui->statusTypes->setStyleSheet(green);
	ui->statusVersion->setStyleSheet(red);
	CardAssistant *card = new CardAssistant();
	ui->mediatypes->addItems(card->insertMediaInit());
	ui->cardtypes->addItems(card->SDCardTypesInit());
	ui->versionList->addItems(card->versionTypesInit());


}

MainWindow::~MainWindow()
{
	delete ui;
}

