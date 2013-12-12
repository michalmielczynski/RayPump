#ifndef COMMONCODE_H
#define COMMONCODE_H

#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QHostAddress>
#include <QDir>
#include <QApplication>

#define uINFO qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "info: (" << __FUNCTION__ << ")"
#define uWARNING qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "WARNING: (" << __FUNCTION__ << ")"
#define uERROR qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "ERROR: (" << __FUNCTION__ << ")"

const static qreal G_VERSION = 0.994f;
const static qreal G_ALLOWED_ADDON_VERSION = 0.993f;
const static QString G_VERSION_NAME =  "Winter White";

// global variables
struct Globals{
    static bool SERVER_VIRTUALIZED;
    static QString SERVER_HOST_NAME;
    static QString SERVER_IP;
    static QString BUFFER_DIRECTORY;
    static QString RENDERS_DIRECTORY;

};

enum ItemRole {
    IR_JOB_PATH = Qt::UserRole + 1
};

#endif // COMMONCODE_H

