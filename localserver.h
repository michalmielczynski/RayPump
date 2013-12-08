#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QDir>

#include "commoncode.h"
#include "json/json.h"

class LocalServer : public QTcpServer
{
    Q_OBJECT
public:
    static const int PORT = 5005;
    explicit LocalServer(QObject *parent = 0);
    bool start();
    void confirmSceneScheduled(bool successful);
    void sendRetry();
    
signals:
    void messageReceived(const QByteArray &message);
    void requestShowWindow();
    
public slots:
    void acceptConnection();

private slots:
    void handleMessage();
    void handleConnectionError(QAbstractSocket::SocketError error);
private:
    QTcpSocket *m_tcpServerConnection;
    
};

#endif // LOCALSERVER_H
