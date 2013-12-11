#ifndef REMOTECLIENT_H
#define REMOTECLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QHostInfo>

#include "commoncode.h"
#include "json/json.h"

/// @note has to be synchronized on server side
/// @todo it is extremely simplified API, which should be extended and documented
enum CommandCode { CC_CONFIRM_AUTH,
                   CC_CONFIRM_VALIDATED,
                   CC_REQUEST_CREDITENTIALS,
                   CC_REQUEST_SCENE_PREPARE,
                   CC_REQUEST_SCENE_RUN,
                   CC_REQUEST_SCENE_TRANSFER_COMPLETE,
                   CC_CONFIRM_SCENE_PREPARED,
                   CC_CONFIRM_SCENE_SCHEDULED,
                   CC_CONFIRM_SCENE_TESTING,
                   CC_CONFIRM_SCENE_RUNNING,
                   CC_CONFIRM_SCENE_READY,
                   CC_CONFIRM_DOWNLOAD_READY,
                   CC_REQUEST_RENDERS_CLEANUP,
                   CC_CONFIRM_JOBLIMIT_EXCEEDED,
                   CC_CONFIRM_DAILYJOBLIMIT_EXCEEDED,
                   CC_ERROR_SCENE_NOT_FOUND, /// @deprecated
                   CC_ERROR_SCENE_TEST_FAILED,
                   CC_COMMAND_SET_DAILYJOBLIMIT,
                   CC_REQUEST_CANCELALLJOBS,
                   CC_REQUEST_CHANGE_IMAGE_FORMAT,
                   CC_CONFIRM_IMPORTANT_INFO,
                   CC_REQUEST_CHANGE_CURRENT_FRAME,
                   CC_CONFIRM_QUEUE_STATUS,
                   CC_REQUEST_CLEANUPDONEJOBS,
                   CC_REQUEST_CANCELJOB,
                   CC_REQUEST_READQUEUE,
                   CC_CONFIRM_GENERAL_INFO,
                   CC_CONFIRM_QUEUE_PROGRESS
                 };
class RemoteClient : public QObject
{
    Q_OBJECT
public:

#ifdef QT_NO_DEBUG
    static const int PORT = 5015;
#else
    static const int PORT = 5020;
#endif

    explicit RemoteClient(QObject *parent = 0);
    bool connectRayPump();
    void sendRayPumpMessage(CommandCode command, QVariantMap &args);
    inline QByteArray accessHash() { return m_accessHash; }
    inline bool isConnected() { return m_tcpSocket->isOpen(); }
    
signals:
    void connected(bool on, const QString &msg = "");
    void receivedRayPumpMessage(CommandCode command, const QVariantMap &args);

public slots:
    void handleReadyRead();
    void handleError(QAbstractSocket::SocketError error);

private:
    void setupDatabase(); /// @test
    QTcpSocket *m_tcpSocket;
    QByteArray m_accessHash;
    qint16 m_blockSize;
    
};

#endif // REMOTECLIENT_H
