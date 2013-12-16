#-------------------------------------------------
#
# Project created by QtCreator 2013-03-13T12:40:30
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = raypump
TEMPLATE = app

#CONFIG -=app_bundle
macx:ICON = $${PWD}/RayPump.icns

SOURCES += main.cpp\
        raypumpwindow.cpp \
    localserver.cpp \
    json/json.cpp \
    remoteclient.cpp \
    simplecript.cpp \
    commoncode.cpp \
    jobmanager.cpp \
    qtsingleapplication/qtsinglecoreapplication.cpp \
    qtsingleapplication/qtsingleapplication.cpp \
    qtsingleapplication/qtlockedfile.cpp \
    qtsingleapplication/qtlockedfile_win.cpp \
    qtsingleapplication/qtlockedfile_unix.cpp \
    qtsingleapplication/qtlocalpeer.cpp

HEADERS  += raypumpwindow.h \
    localserver.h \
    commoncode.h \
    json/json.h \
    remoteclient.h \
    simplecript.h \
    jobmanager.h \
    qtsingleapplication/qtsinglecoreapplication.h \
    qtsingleapplication/qtsingleapplication.h \
    qtsingleapplication/QtSingleApplication \
    qtsingleapplication/qtlockedfile.h \
    qtsingleapplication/QtLockedFile \
    qtsingleapplication/qtlocalpeer.h

FORMS    += raypumpwindow.ui

RESOURCES += \
    icons.qrc

RC_FILE = RayPumpIcon.rc

OTHER_FILES += \
    RayPumpIcon.rc \
    qtsingleapplication/qtsinglecoreapplication.pri \
    qtsingleapplication/qtsingleapplication.pri
