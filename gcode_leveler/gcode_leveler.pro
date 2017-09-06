#-------------------------------------------------
#
# Project created by QtCreator 2017-08-29T17:16:10
#
#-------------------------------------------------

QT       += core gui serialport widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = gcode_leveler
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    global.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    Notes
