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

#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QDir>
#include <QApplication>

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
