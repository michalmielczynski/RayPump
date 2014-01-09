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
                   CC_CONFIRM_QUEUE_PROGRESS,
                   CC_REQUEST_CLEANUPBUFFER
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
    qint16 m_blockSize;
    
};

#endif // REMOTECLIENT_H
