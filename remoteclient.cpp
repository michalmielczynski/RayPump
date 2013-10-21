#include "remoteclient.h"

RemoteClient::RemoteClient(QObject *parent) :
    QObject(parent)
{
    m_tcpSocket = new QTcpSocket(this);
    m_blockSize = 0;
    connect(m_tcpSocket, SIGNAL(readyRead()), SLOT(handleReadyRead()));
    connect(m_tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(handleError(QAbstractSocket::SocketError)));

    //setupDatabase();
}

bool RemoteClient::connectRayPump()
{
    if(Globals::SERVER_VIRTUALIZED){
        QHostInfo info = QHostInfo::fromName(Globals::SERVER_HOST_NAME);
        if (info.addresses().isEmpty()){
            uERROR << "cannot get IP address for" << Globals::SERVER_HOST_NAME;
            return false;
        }
        uINFO << "connecting" << info.addresses().first().toString();
        m_tcpSocket->abort();
        m_tcpSocket->connectToHost(info.addresses().first(), PORT);
    }
    else{
        m_tcpSocket->abort();
        m_tcpSocket->connectToHost(QHostAddress(Globals::SERVER_IP), PORT);
    }
    return true;
}

void RemoteClient::sendRayPumpMessage(CommandCode command, QVariantMap &args)
{
    args.insert("command", command);

    QByteArray data = QtJson::serialize(args);
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    out.setVersion(QDataStream::Qt_4_6);
    out << (quint16)0;
    out << data;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));

    m_tcpSocket->write(block);
//    m_tcpSocket->flush();
    if (!m_tcpSocket->waitForBytesWritten()){
        uERROR << "wait for bytes written failed";
    }

}

void RemoteClient::handleReadyRead()
{
    QDataStream in(m_tcpSocket);
    in.setVersion(QDataStream::Qt_4_6);

    if (m_blockSize == 0){
        if (m_tcpSocket->bytesAvailable() < (int)sizeof(quint16)){
            return;
        }

        in >> m_blockSize;
    }

    if (m_tcpSocket->bytesAvailable() < m_blockSize){
        uWARNING << "awaiting complete data";
        return;
    }

    QByteArray data;
    in >> data;

    m_blockSize = 0;

    bool ok;
    QVariantMap result = QtJson::parse(data, ok).toMap();
    if (!ok){
        uERROR << "error parsing json" << data;
        return;
    }

    if (!result.contains("command")){
        uERROR << "no command value found" << data;
        return;
    }

    CommandCode command = (CommandCode)(result.value("command").toInt());
    /// @badcode switch here is pretty inconsistent.
    /// Another switch in main window is required anyway, so it might be better idea just to do the basic check and redirect the input with single emit
    switch (command){
    case CC_CONFIRM_AUTH:
        emit receivedRayPumpMessage(command, result.value("reason"));
        break;
    case CC_CONFIRM_VALIDATED:
    {
        bool validated = result.value("valid").toBool();
        if (validated){
            m_accessHash = result.value("hash").toByteArray();
            if (m_accessHash.isEmpty()){
                uERROR << "empty access hash";
            }
            else{
                emit connected(true);
            }
        }
        else{
            uWARNING << "validated is false";
        }
        emit receivedRayPumpMessage(command, result);
    }
        break;
    case CC_CONFIRM_SCENE_SCHEDULED: ////
        emit receivedRayPumpMessage(command, result.value("seconds", 0));
        break;
    case CC_CONFIRM_SCENE_PREPARED:
        emit receivedRayPumpMessage(command, result);
        break;
    case CC_CONFIRM_SCENE_READY:
    {
        QString sceneName = result.value("scene_name").toString();
        if (sceneName.isEmpty()){
            uERROR << "empty scene name";
        }
        else{
            emit sceneReady(sceneName);
        }
    }
        break;
    case CC_CONFIRM_SCENE_TESTING: case CC_CONFIRM_SCENE_RUNNING:
    {
        QString sceneName = result.value("scene_name").toString();
        emit receivedRayPumpMessage(command, sceneName);
    }
        break;
    case CC_CONFIRM_DOWNLOAD_READY:
    {
        bool valid = result.value("ready").toBool();
        emit receivedRayPumpMessage(command, valid);
    }
        break;
    case CC_CONFIRM_JOBLIMIT_EXCEEDED: case CC_CONFIRM_DAILYJOBLIMIT_EXCEEDED:
        emit receivedRayPumpMessage(command, result);
        break;
    case CC_ERROR_SCENE_NOT_FOUND:
        emit receivedRayPumpMessage(command, result.value("scene_name"));
        break;
    case CC_ERROR_SCENE_TEST_FAILED:
        if (!result.contains("reason")){
            uERROR << "cannot find 'reason' value";
        }
        emit receivedRayPumpMessage(command, result);
        break;
    case CC_CONFIRM_GENERAL_INFO: case CC_CONFIRM_IMPORTANT_INFO:
        emit receivedRayPumpMessage(command, result.value("message"));
        break;
    case CC_CONFIRM_QUEUE_STATUS:
        emit receivedRayPumpMessage(command, result);
        break;
    default:
        uERROR << "command not recognized" << result.value("command");
        break;
    }

    handleReadyRead();

}

void RemoteClient::handleError(QAbstractSocket::SocketError error)
{
    uERROR << "socket error" << error << m_tcpSocket->errorString();
    emit connected(false, m_tcpSocket->errorString());
}

void RemoteClient::setupDatabase()
{
    uINFO << "connecting...";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setHostName("mysql5.hti.pl");
    db.setDatabaseName("raypump");
    db.setUserName("raypump");
    db.setPassword("W5GKHY97rAcVMYJG");
    bool ok = db.open();

    if (ok){
        uINFO << "DB CONNECTED";
    }
    else{
        uERROR << "CANNOT CONNECT DB";
    }
}
