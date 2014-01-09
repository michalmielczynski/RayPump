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

#include "rsyncwrapper.h"

RsyncWrapper::RsyncWrapper(QObject *parent) :
    QObject(parent),
    m_synchroInProgress(false),
    m_transferLimit(0),
    m_autorun(false)
{
#if defined(Q_OS_WIN)
    m_rsyncFilePath = QFileInfo("rsync/rsync.exe");
#elif defined(Q_OS_LINUX)
    m_rsyncFilePath = QFileInfo("/usr/bin/rsync");
#elif defined(Q_OS_MAC)
    m_rsyncFilePath = QFileInfo("/usr/bin/rsync");
#endif
    if (!m_rsyncFilePath.isFile()){
        uERROR << "cannot find rsync binary. Cannot continue" << m_rsyncFilePath.absoluteFilePath();
        QMessageBox::warning(0, tr("RayPump failed"), tr("Installation is broken, please re-install RayPump client"));
        exit(EXIT_FAILURE);
    }

    m_rsyncProcess = new QProcess(this);
    m_rsyncProcess->setReadChannel(QProcess::StandardOutput);
    m_rsyncProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_rsyncProcess, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(handleRsyncFinished(int,QProcess::ExitStatus)));
    connect(m_rsyncProcess, SIGNAL(readyReadStandardOutput()), SLOT(handleReadyReadRsyncOutput()));
}

void RsyncWrapper::buffer(const QString &sourceDirectory, const QString &destinationDirectory)
{
    Command command;
    QStringList arguments;

    if (QDir(sourceDirectory).isReadable()){
        command.workingDirectory = sourceDirectory;
        arguments << "--progress" << "-zr" << "--exclude=*.blend?" << "--compress-level=9" << QString("--bwlimit=%1").arg(m_transferLimit)\
                  << "./"\
                  << destinationDirectory;
    }
    else if (QDir(destinationDirectory).isReadable()){
        command.workingDirectory = destinationDirectory;
        arguments << "--progress" << "--perms" << "--exclude=*.blend*" << "--exclude=*.gz" << "-zr" << "--compress-level=9" <<  QString("--bwlimit=%1").arg(m_transferLimit)\
                  << sourceDirectory\
                  << "./";
    }
    else{
        uERROR << "both source and destination directory are not readable (not local?). Aborting...";
        return;
    }

    command.arguments = arguments;
    m_commandBuffer.append(command);

}

bool RsyncWrapper::run()
{
    m_synchroInProgress = false;

    if (m_commandBuffer.isEmpty()){
        uINFO << "Nothing to run - use buffer method first";
        return true;
    }

    if (isRunning()){
        uERROR << "rsync is already running. Aboring...";
        return false;
    }

    if (!isHashValid()){
        uERROR << "can't continue without valid hash";
        m_autorun = true;
        return false;
    }

    m_rsyncTimer.start();

    Command command = m_commandBuffer.takeFirst();
    //uINFO << command.workingDirectory << command.arguments;

    QStringList env = QProcess::systemEnvironment();
    env << "RSYNC_PASSWORD=" + m_accessHash;

    m_rsyncProcess->setEnvironment(env);
    m_rsyncProcess->setWorkingDirectory(command.workingDirectory);
    m_rsyncProcess->start(m_rsyncFilePath.absoluteFilePath(), command.arguments);
    if (!m_rsyncProcess->waitForStarted()){
        uERROR << "failed to start rsync process for scene";
        m_rsyncTimer.invalidate();
        return false;
    }

    m_synchroInProgress = true;
    m_autorun = false;
    return true;

}

void RsyncWrapper::handleRsyncFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_commandBuffer.isEmpty()){
        m_synchroInProgress = false;
        uINFO << "got exit code" << exitCode << exitStatus;
        uINFO << "process finished in" << m_rsyncTimer.elapsed()/1000 << "seconds";
        emit finished(exitCode == 0 && exitStatus == QProcess::NormalExit);
    }
    else{
        uINFO << "running next buffered job";
        run();
    }
}

void RsyncWrapper::handleReadyReadRsyncOutput()
{
    if (m_rsyncProcess->state() != QProcess::Running){
        uERROR << "sync process is not running";
        return;
    }

    QByteArray output = m_rsyncProcess->readAllStandardOutput().trimmed();
    if (!output.isEmpty()){
        //uINFO << output;
        emit rsyncOutput(output);
    }
}
