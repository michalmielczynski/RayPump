/* Copyright 2013 michal.mielczynski@gmail.com. All rights reserved.
 *
 * DISTRIBUTION OF THIS SOFTWARE, IN ANY FORM, WITHOUT WRITTEN PERMISSION FROM
 * MICHAL MIELCZYNSKI, IS ILLEGAL AND PROHIBITED BY LAW.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAL MIELCZYNSKI ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL MICHAL MIELCZYNSKI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of Michal Mielczynski.
 */

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

    switch (command){

    case CC_CONFIRM_VALIDATED:
    {
        bool validated = result.value("valid").toBool();
        if (validated){
            QByteArray accessHash = result.value("hash").toByteArray();
            if (accessHash.isEmpty()){
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
    case CC_CONFIRM_AUTH: case CC_CONFIRM_SCENE_SCHEDULED:
    case CC_CONFIRM_SCENE_READY: case CC_CONFIRM_DOWNLOAD_READY:
    case CC_ERROR_SCENE_NOT_FOUND:
    case CC_CONFIRM_GENERAL_INFO: case CC_CONFIRM_IMPORTANT_INFO:
    case CC_CONFIRM_SCENE_TESTING: case CC_CONFIRM_SCENE_RUNNING:
    case CC_ERROR_SCENE_TEST_FAILED: case CC_CONFIRM_SCENE_PREPARED:
    case CC_CONFIRM_JOBLIMIT_EXCEEDED: case CC_CONFIRM_DAILYJOBLIMIT_EXCEEDED:
    case CC_CONFIRM_QUEUE_STATUS: case CC_CONFIRM_QUEUE_PROGRESS:
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
