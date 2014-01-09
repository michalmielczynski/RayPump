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

#ifndef RSYNCWRAPPER_H
#define RSYNCWRAPPER_H

#include <QObject>
#include <QProcess>
#include <QVariantMap>
#include <QList>
#include <QFileInfo>
#include <QMessageBox>
#include <QElapsedTimer>

#include "commoncode.h"

class RsyncWrapper : public QObject
{
    Q_OBJECT
public:
    explicit RsyncWrapper(QObject *parent = 0);
    inline bool synchroInProgress() { return m_synchroInProgress; }
    inline bool isRunning() { return (m_rsyncProcess->state() != QProcess::NotRunning) ;}
    void buffer(const QString &sourceDirectory, const QString &destinationDirectory);
    bool run();
    inline bool isHashValid() { return !m_accessHash.isEmpty(); }
    inline void setAccessHash(const QByteArray &hash) {
        m_accessHash = hash;
        if (m_autorun){
            run();
        }
    }
    inline void kill() { m_rsyncProcess->kill(); }
    inline quint64 elapsed() { return m_rsyncTimer.elapsed(); }
    inline void setTransferLimit(int kib) { m_transferLimit = kib; }

signals:
    void rsyncOutput(const QByteArray &output);
    void finished(bool success);

public slots:
    void handleRsyncFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleReadyReadRsyncOutput();

private:
    struct Command{
        QStringList arguments;
        QString workingDirectory;
    };

    QProcess *m_rsyncProcess;
    QList<Command> m_commandBuffer;
    QFileInfo m_rsyncFilePath;
    bool m_synchroInProgress;
    bool m_autorun;
    QByteArray m_accessHash;
    QElapsedTimer m_rsyncTimer;
    int m_transferLimit; // KiB (1024 bytes)

};

#endif // RSYNCWRAPPER_H
