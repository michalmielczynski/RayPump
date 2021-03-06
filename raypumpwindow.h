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

#ifndef RAYPUMPWINDOW_H
#define RAYPUMPWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QDir>
#include <QFile>
#include <QDesktopServices>
#include <QUrl>
#include <QCloseEvent>
#include <QTimer>
#include <QLayout>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QSettings>
#include <QHostInfo>
#include <QFileDialog>

#include "localserver.h"
#include "remoteclient.h"
#include "commoncode.h"
#include "json/json.h"
#include "simplecript.h"
#include "jobmanager.h"
#include "rsyncwrapper.h"
#include "fileops.h"

/** @todo GENERAL IDEAS/BUGS

 *1)
 *Integrated rsync (difficulty: hard)
 * - current separate rsync binary (for Windows) causes some problems;
 * - being Linux/Mac binary dependent also isn't the best solution;
 * Making custom rsync based class inside RayPump code could help to solve stability and security issues
 *
 *2)
 * Non-ASCII paths/names cause problems (work in progress, by Tiago Shibata) (difficulty: medium)
 * - scene names should be converted to ASCII during the copying to BUFFER;
 * - RayPump path passed to Blender should handle UTF-8;
 *
 *3)
 * Auto-lauch (difficulty: easy)
 * - running the RayPump client should be initialized from Blender panel to avoid application switching
 * - path of the RayPump binary can be set (and stored) in Blender settings.
 *
 *4)
 * Support usage by multiple users (difficulty: unknown)
 * - Two instances of RayPump cannot be executed inside different users (both can't use port 5005)
 *   Possible solution: add a port configuration?
 *
 *5)
 * Automatic software upgrade (difficulty: hard)
 * - despite the outdated warnings, RayPump client should download and upgrade automatically
 *
 *6)
 * "Back to the Blender" (difficulty: hard)
 * - just an idea, but it would be neat thing to have: once the rendered image is ready and downloaded it gets imported
 *   back to Blender. In perfect world, this online rendered image would behave just like local render result (having all the
 *   features inside Blender)
 *
 *7)
 * Fixing the copyright notice (difficulty: trivial)
 * - header should contain GPL notice instead of illegal distribution stuff
 */

namespace Ui {
class RayPumpWindow;
}

class RayPumpWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit RayPumpWindow(QWidget *parent = 0);
    ~RayPumpWindow();
    void run();

    /// @note has to be synchronized with the same enum on server side
    enum Package {
        PT_FREE                 =   1<<0,
        PT_COMMERCIAL_STATIC    =   1<<1,
        PT_COMMERCIAL_ANIMATION =   1<<2,
        PT_STRESSTEST           =   1<<3
    };
    Q_DECLARE_FLAGS(Packages, Package)

    struct Job{
        void clear() {
            frameCurrent = frameStart = frameEnd = 1;
            jobType = PT_FREE;
        }

        QString name; // full name of the job (includes date and frames encoded). Provided by the server
        Package jobType;
        int frameCurrent;
        int frameStart;
        int frameEnd;
        QStringList externalPaths;
        QString versionCycle;
    };

public slots:
    void handleInstanceWakeup(const QString &message);

protected:
    void closeEvent(QCloseEvent *event);
    void changeEvent(QEvent *event);

private slots:
    void handleLocalMessage(const QByteArray &message);
    void handleRsyncSceneFinished(bool success);
    void handleRsyncRendersFinished(bool success);
    void handleConnectionStatus(bool connected, const QString &msg);
    void handleRayPumpCommand(CommandCode command, const QVariantMap &arg);
    void handleSceneReady(const QString &sceneName);
    void handleTrayIconClicked(QSystemTrayIcon::ActivationReason reason);
    void handleTrayIconMessageClicked();
    void showHideWindow();
    void transferRenders();
    void handleJobManagerStatusBarMessage(const QString &message);
    void handleRenderProgressChanged(int progress, int total);
    void handleReadyReadRsyncSceneOutput(const QByteArray output);
    void handleReadyReadRsyncRendersOutput(const QByteArray output);
    void handleRenderPointsChanged(int renderPoints);
    void handleOtherUserJobProgress(double progress);

    void on_lineEditUserPass_returnPressed();
    void on_actionConnect_toggled(bool checked);
    void on_actionQuit_triggered();
    void on_actionAbout_triggered();
    void on_actionWebsite_triggered();
    void on_actionDownload_triggered();
    void on_actionFolder_triggered();
    void on_actionShow_triggered();
    void on_checkBoxRememberUser_toggled(bool checked);
    void on_actionAlways_on_Top_triggered(bool checked);
    void on_lineEditUserName_returnPressed();
    void on_actionAdd_Render_Points_triggered();
    void on_actionCancel_uploading_triggered();
    void on_tableWidget_cellDoubleClicked(int row, int column);
    void on_pushButtonCleanUp_clicked();
    void on_pushButtonRefresh_clicked();
    void on_pushButtonConnect_clicked();
    void on_groupBoxAdvancedMode_toggled(bool toggled);
    void on_pushButtonCancelJob_clicked();
    void on_pushButtonRenderPath_clicked();
    void on_spinBoxUploadLimit_valueChanged(int arg1);
    void on_actionCleanRemoteBuffer_triggered();

private:
    void setupTrayIcon();
    void setupRsyncWrappers();
    void setupLocalServer();
    void setupRemoteClient();
    void setupAutoconnection();
    void setupJobManager();
    bool transferScene(const QFileInfo &sceneFileInfo);
    void assertSynchroDirectories();
    void cleanUpBufferDirectory();
    void setRenderFilesPermission();
    void calculateJobTime(const QString &sceneName);
    bool checkMalformedUsername(const QString &userName);
    void openRenderFolder(const QString &subfolderName = "");
    void setJobType(const QString &jobTypeString);
    QString formattedStringFromSeconds(int duration);
    void setupLicenseAgreement();

    Ui::RayPumpWindow *ui;
    LocalServer *m_localServer;
    RemoteClient *m_remoteClient;
    JobManager *m_jobManager;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayIconMenu;
    RsyncWrapper *m_sceneTransferManager, *m_rendersTransferManager;
    bool m_wantToQuit;
    quint64 m_simpleCryptKey;
    int m_downloadTryCounter;
    QString m_status;
    int m_renderPoints;
    Job m_currentJob;
    float m_blenderAddonVersion;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RayPumpWindow::Packages)

#endif // RAYPUMPWINDOW_H
