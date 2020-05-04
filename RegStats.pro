#-------------------------------------------------
#
# Project created by QtCreator 2016-09-30T11:42:25
#
#-------------------------------------------------

QT += core gui sql
QT += charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RegStats
TEMPLATE = app


SOURCES += main.cpp\
    chartwindow.cpp \
        mainwindow.cpp \
    settingsform.cpp \
    psnotesform.cpp \
    customprocessdialog.cpp \
    pt4loader.cpp

HEADERS  += mainwindow.h \
    chartwindow.h \
    settingsform.h \
    psnotes.h \
    psnotesform.h \
    customprocessdialog.h \
    pt4loader.h

FORMS    += mainwindow.ui \
    settingsform.ui \
    psnotesform.ui \
    customprocessdialog.ui \
    pt4loader.ui

RESOURCES += \
    resources.qrc
