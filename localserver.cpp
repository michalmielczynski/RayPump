/* Copyright 2013 michal.mielczynski@gmail.com. All rights reserved.
 *
 *
 * RayPump Client software might be distributed under GNU GENERAL PUBLIC LICENSE
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

#include "localserver.h"

LocalServer::LocalServer(QObject *parent) :
    QTcpServer(parent)
{
    connect(this, SIGNAL(newConnection()), SLOT(acceptConnection()));
}

bool LocalServer::start()
{
    while (!isListening() && !listen(QHostAddress::LocalHost, PORT)) {
        uERROR << "cannot start listening";
        return false;
    }

    uINFO << "listening on port" << serverPort();
    return true;
}

void LocalServer::confirmSceneScheduled(bool successful)
{
    if (successful){
        m_tcpServerConnection->write("SUCCESS\n");
    }
    else{
        m_tcpServerConnection->write("FAILED\n");
    }
}

void LocalServer::sendRetry()
{
    m_tcpServerConnection->write("RETRY\n");
}

void LocalServer::acceptConnection()
{
    uINFO << "client connected";
    emit requestShowWindow();
    m_tcpServerConnection = nextPendingConnection();
    connect(m_tcpServerConnection, SIGNAL(readyRead()), SLOT(handleMessage()));
    connect(m_tcpServerConnection, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(handleConnectionError(QAbstractSocket::SocketError)));

    m_tcpServerConnection->write((Globals::BUFFER_DIRECTORY + "\n").toLatin1());

    QVariantMap map;
    map.insert("CONNECTED", true);
    emit messageReceived(QtJson::serialize(map));
}

void LocalServer::handleMessage()
{

    QByteArray message = m_tcpServerConnection->readAll();
    emit messageReceived(message);

}

void LocalServer::handleConnectionError(QAbstractSocket::SocketError error)
{
    QVariantMap map;
    map.insert("DISCONNECTED", true);
    emit messageReceived(QtJson::serialize(map));

    if (error == QTcpSocket::RemoteHostClosedError){
        uINFO << "client disconnected";
        return;
    }

    uERROR << "connection error" << errorString();
}
