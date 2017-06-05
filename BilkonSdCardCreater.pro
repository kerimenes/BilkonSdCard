#-------------------------------------------------
#
# Project created by QtCreator 2017-05-11T15:13:03
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BilkonSdCardCreater
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    cardassistant.cpp \
    json/jsonhelper.cpp \
    json/qjsonmodel.cpp

HEADERS  += mainwindow.h \
    cardassistant.h \
    json/jsonhelper.h \
    json/qjsonmodel.h

FORMS    += mainwindow.ui

QMAKE_CXXFLAGS += -std=c++11

