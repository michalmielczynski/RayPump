#-------------------------------------------------
#
# Project created by QtCreator 2013-03-13T12:40:30
#
#-------------------------------------------------

QT       += core gui network

# to remove:
QT      += sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RayPump
TEMPLATE = app

#CONFIG -=app_bundle
macx:ICON = $${PWD}/RayPump.icns

SOURCES += main.cpp\
        raypumpwindow.cpp \
    localserver.cpp \
    application.cpp \
    json/json.cpp \
    remoteclient.cpp \
    simplecript.cpp \
    commoncode.cpp \
    jobmanager.cpp

HEADERS  += raypumpwindow.h \
    localserver.h \
    commoncode.h \
    application.h \
    json/json.h \
    remoteclient.h \
    simplecript.h \
    jobmanager.h

FORMS    += raypumpwindow.ui

RESOURCES += \
    icons.qrc

RC_FILE = RayPumpIcon.rc

OTHER_FILES += \
    RayPumpIcon.rc
