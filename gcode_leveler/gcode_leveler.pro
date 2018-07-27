#-------------------------------------------------
#
# Project created by QtCreator 2017-08-29T17:16:10
#
#-------------------------------------------------

QT       += core gui serialport widgets

greaterThan(QT_MAJOR_VERSION, 5): QT += widgets

#CONFIG += static
#export  LD_LIBRARY_PATH=/home/shane/Qt5.9.3/5.9.3/gcc_64/lib:$LD_LIBRARY_PATH

TARGET = gcode_leveler
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    gcodelib.cpp

HEADERS  += mainwindow.h \
    global.h \
    gcodelib.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    Notes

