#-------------------------------------------------
#
# Project created by QtCreator 2017-08-08T13:04:32
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MapEditor
TEMPLATE = app

# for android, define this to 1
DEFINES += MINIMALISTIC_INTERFACE=1

SOURCES += main.cxx\
        mapeditor.cxx

HEADERS  += mapeditor.hxx

FORMS    += mapeditor.ui \
    tileanimationdialog.ui

RESOURCES += \
    resources.qrc

DISTFILES +=
