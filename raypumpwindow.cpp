/* Copyright 2013 Michal Mielczynski. All rights reserved.
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

#include "raypumpwindow.h"
#include "ui_raypumpwindow.h"

RayPumpWindow::RayPumpWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RayPumpWindow)
{
    uINFO << "RayPump Client" << G_VERSION << "(c) michal.mielczynski@gmail.com. All rights reserved.";
    m_simpleCryptKey = 1365559412; //:)

    ui->setupUi(this);
//#ifndef QT_NO_DEBUG
//    setWindowTitle("TEST VERSION");
//#endif
#if defined(Q_OS_MAC)
    ui->menuBar->setNativeMenuBar(false);
#endif
    ui->progressBar->hide();
    ui->progressBarRender->hide();
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->actionConnect->setProperty("force", false);
    ui->pushButtonRenderPath->setText("Change renders directory (" + Globals::RENDERS_DIRECTORY + ")");
    setupLicenseAgreement();

    setupTrayIcon();
    setupRsyncProcesses();
    setupLocalServer();
    setupRemoteClient();
    setupAutoconnection();
    setupJobManager();

    ui->statusBar->showMessage(tr("Awaiting user action"));
    m_wantToQuit = false;

    /// @todo fixed size still does not work
    setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

#if defined(Q_OS_MAC)
    Qt::WindowFlags flags = windowFlags();
    flags ^= Qt::WindowMinMaxButtonsHint;
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    flags ^= Qt::WindowMaximizeButtonHint;
    setWindowFlags(flags);
#else
    Qt::WindowFlags flags = windowFlags();
    flags ^= Qt::WindowMinMaxButtonsHint;
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    setWindowFlags(flags);
#endif
    assertSynchroDirectories();

    m_status = tr("Not connected");
    m_currentJob.clear();
    m_blenderAddonVersion = 0.0;



}

RayPumpWindow::~RayPumpWindow()
{
    delete ui;
}

void RayPumpWindow::closeEvent(QCloseEvent *event)
{
    if (!m_wantToQuit){
        if (m_trayIcon->isVisible()){
            showHideWindow();
            event->ignore();
        }
        else{
            QMainWindow::closeEvent(event);
            qApp->quit();
        }
    }
    else{
        if (m_synchroInProgress){
            QMessageBox msgBox;
            msgBox.setText(tr("Uploading in progress"));
            msgBox.setInformativeText(tr("Do you want to abort current scene transfer?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::No);
            int ret = msgBox.exec();
            switch (ret){
            case QMessageBox::Yes:
                uINFO << "we will quit";
//                QMainWindow::closeEvent(event);
//                qApp->quit();
//                return;
                break;
            default:
                event->ignore();
                return;
                break;
            }
        }

        QMainWindow::closeEvent(event);
        qApp->quit();

    }
}

void RayPumpWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
}

void RayPumpWindow::on_lineEditUserPass_returnPressed()
{
    if(ui->actionConnect->isEnabled()){
        ui->actionConnect->toggle();
    }
}

void RayPumpWindow::handleLocalMessage(const QByteArray &message)
{    
    bool ok;
    QVariantMap map = QtJson::parse(message, ok).toMap();
    if (!ok){
        uERROR << "cannot parse message to JSON data" << message;
        return;
    }

    foreach (QString key, map.keys()){
        if (key == "SCHEDULE"){
            m_localServer->confirmSceneScheduled(transferScene(map.value(key).toString()));
        }
        else if (key == "CONNECTED"){
            ui->statusBar->showMessage(tr("Blender connected"));
            m_trayIcon->showMessage("RayPump", tr("Blender connected"));
            m_status = tr("Blender connected");
            on_actionShow_triggered();
        }
        else if (key == "DISCONNECTED"){
            ui->statusBar->showMessage(tr("Blender disconnected"));
            m_trayIcon->showMessage("RayPump", tr("Blender disconnected"));
            m_status = tr("Blender disconnected");
        }
        else if (key == "FORMAT"){
            /// @deprecated everything above RayPump Free uses Scene's output format
        }
        else if (key == "FRAME_CURRENT"){
            m_currentJob.frameCurrent = map.value(key).toInt();
        }
        else if (key == "FRAME_START"){
            m_currentJob.frameStart = map.value(key).toInt();
        }
        else if (key == "FRAME_END"){
            m_currentJob.frameEnd = map.value(key).toInt();
        }
        else if (key == "VERSION"){
            m_blenderAddonVersion = map.value(key).toFloat();
            uINFO << "blender addon claims the version" << m_blenderAddonVersion;
        }
        else if (key == "JOB_TYPE"){
            setJobType(map.value(key).toString());
        }
        /// @todo more to come?
        else{
            uERROR << "unhandled command" << key;
        }

    }
}

void RayPumpWindow::handleRsyncSceneFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    uINFO << "got exit code" << exitCode << exitStatus;
    m_synchroInProgress = false;
    if (exitCode == 0 && exitStatus == QProcess::NormalExit){
        uINFO << "scene uploaded in" << m_rsyncTimer.elapsed()/1000 << "seconds";
        m_trayIcon->showMessage("RayPump", tr("Uploaded in %1 s").arg(m_rsyncTimer.elapsed()/1000), QSystemTrayIcon::Information, 3000);
        ui->statusBar->showMessage(tr("Transfer complete"));
        QVariantMap map;
        m_remoteClient->sendRayPumpMessage(CC_REQUEST_SCENE_TRANSFER_COMPLETE, map);
    }
    else{
        ui->statusBar->showMessage(tr("Failed to send scene"));
        uERROR << "failed to send scene";
    }
    ui->progressBar->reset();
    ui->progressBar->setMaximum(0);
    ui->progressBar->hide();
}

void RayPumpWindow::handleRsyncRendersFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit){
        m_downloadTryCounter = 0;
        ui->progressBarRender->hide();
        this->activateWindow();
        m_trayIcon->showMessage("RayPump", tr("Renders downloaded"), QSystemTrayIcon::Information, 10000);
        ui->statusBar->showMessage(tr("Download complete"));
        m_status = tr("Download complete");
        m_trayIcon->setIcon(QIcon(":/icons/icons/logo_small_glow.ico"));
        uINFO << "Transfer completed";
        QVariantMap map;
        m_remoteClient->sendRayPumpMessage(CC_REQUEST_RENDERS_CLEANUP, map);
        setRenderFilesPermission();

        if (m_totalJobTimer.isValid()){
            int secs = m_totalJobTimer.elapsed()/1000;
            uINFO << QString("total render time: %1:%2").arg(secs/60).arg(secs%60);
            m_totalJobTimer.invalidate();
        }
        else{
            uERROR << "total job timer is invalid";
        }
    }
    else if (m_downloadTryCounter > 3){
        switch (exitCode){
        case 5:
            ui->statusBar->showMessage(tr("Download auth failed"));
            uERROR << "rsync auth failed";
            break;
        case 23:
            ui->statusBar->showMessage(tr("No renders available"));
            m_status = tr("No renders available");
            uINFO << "no files to rsync";
            break;
        case 14:
            uERROR << "error in rsync protocol data stream (code 12) - connection unexpectedly closed";
            break;
        default:
            ui->statusBar->showMessage(tr("Download failed"));
            m_status = tr("Download failed");
            uERROR << "failed to download renders" << exitCode << exitStatus;
            break;
        }
    }
    else{
        m_downloadTryCounter++;
        uINFO << "download has failed. Try counter:" << m_downloadTryCounter;
        QTimer::singleShot(2000, this, SLOT(transferRenders()));
    }
}

void RayPumpWindow::handleConnectionStatus(bool connected, const QString &msg)
{
    ui->actionConnect->setProperty("force", false);

    if (connected){
        ui->statusBar->showMessage(tr("Connected"));
        m_status = tr("Connected");
    }
    else{
        ui->actionConnect->setProperty("force", true);
        ui->actionConnect->setChecked(false);
        if (!msg.isEmpty()){
            ui->statusBar->showMessage(msg);
            m_trayIcon->showMessage("RayPump", msg, QSystemTrayIcon::Warning, 3000);
        }
    }
}

void RayPumpWindow::handleRayPumpCommand(CommandCode command, const QVariant &arg)
{
    if (!arg.isValid()){
        uINFO << "invalid argument";
        return;
    }
    switch(command){
    case CC_CONFIRM_AUTH:
    {
        QVariantMap args;
        args["name"] = ui->lineEditUserName->text();
        args["password"] = ui->lineEditUserPass->text();
#if defined(Q_OS_MAC)
        args["platform"] = "mac";
#elif defined(Q_OS_WIN)
        args["platform"] = "win";
#elif defined(Q_OS_LINUX)
        args["platform"] = "lin";
#endif
        args["software_version"] = (qreal)G_VERSION;
        m_remoteClient->sendRayPumpMessage(CC_REQUEST_CREDITENTIALS, args);
    }
        break;
    case CC_CONFIRM_VALIDATED:
    {
        QVariantMap map  = arg.toMap();
        if (map.value("valid").toBool()){
            ui->actionConnect->setChecked(true);
            m_trayIcon->setIcon(QIcon(":/icons/icons/logo_small.ico"));
        }
        else{
            ui->actionConnect->setProperty("force", true);
            ui->actionConnect->setChecked(false);
            ui->statusBar->showMessage(tr("Authentication failed"));
        }

        QVariantMap empty;
        m_remoteClient->sendRayPumpMessage(CC_REQUEST_READQUEUE, empty); // reads also Render Points

        break;
    }
    case CC_CONFIRM_SCENE_PREPARED:
    {
        QString jobName = arg.toMap().value("job_name").toString();
        if (jobName.isEmpty()){
            uERROR << "no job name given";
            break;
        }
        m_currentJob.name = jobName;
    }
        break;
    case CC_CONFIRM_SCENE_SCHEDULED:
    {
        uINFO << "scene scheduled";
        int seconds = (arg.toInt());
        ui->statusBar->showMessage(tr("Job scheduled"));
        m_status = tr("Job scheduled. Awaiting free resources...");
        if (seconds){
            m_trayIcon->showMessage("RayPump", tr("Roughly estimated time to start: %1").arg(formattedStringFromSeconds(seconds)));
        }
        else{
            m_trayIcon->showMessage("RayPump", tr("Job scheduled"), QSystemTrayIcon::Information, 3000);
        }
    }
        break;
    case CC_CONFIRM_SCENE_TESTING:
    {
        QString sceneName = arg.toString();
        ui->progressBarRender->setProperty("testing", true);
        m_trayIcon->showMessage("RayPump", tr("Testing: %1").arg(sceneName), QSystemTrayIcon::Information, 2000);
        m_status = tr("Testing scene %1").arg(sceneName);
        uINFO << "testing scene" << sceneName;
    }
        break;
    case CC_CONFIRM_SCENE_RUNNING:
    {
        QString sceneName = arg.toString();
        uINFO << "scene confirmed (running)" << sceneName;
        if (!sceneName.isEmpty()){
            m_jobStartTimes.insert(sceneName, QDateTime::currentDateTime());
            m_trayIcon->showMessage("RayPump", tr("Starting %1").arg(sceneName), QSystemTrayIcon::Information, 3000);
            m_status = tr("Rendering %1").arg(sceneName);
            uINFO << "rendering scene" << sceneName;
        }
        ui->statusBar->showMessage(tr("Pumping rays..."));
    }
        break;
    case CC_CONFIRM_DOWNLOAD_READY:
    {
        //QString sceneName = arg.toString();
        uINFO << "download ready";
        transferRenders();
        //calculateJobTime(sceneName);
    }
        break;
    case CC_CONFIRM_JOBLIMIT_EXCEEDED:
        uERROR << "job limit exceeded" << arg.toInt();
        show();
        raise();
        activateWindow();
        m_trayIcon->showMessage("RayPump", tr("Queue limit reached"), QSystemTrayIcon::Warning);
        m_status = tr("Queue limit reached");
        QMessageBox::warning(this, tr("RayPump limitation"), tr("User's jobs queue is limited<br><br>Please wait for previously scheduled job(s) to complete").arg(arg.toInt()));
        break;
    case CC_CONFIRM_DAILYJOBLIMIT_EXCEEDED:
    {
        QVariantMap map = arg.toMap();
        show();
        raise();
        activateWindow();
        m_trayIcon->showMessage("RayPump", tr("Job limit reached"), QSystemTrayIcon::Warning);
        m_status = tr("Job limit reached");
        QMessageBox::warning(this,
                             tr("RayPump limitation"),
                             tr("%1: daily job count exceeded limit (%2 of %3)")
                             .arg(map.value("package").toString().toUpper())
                             .arg(map.value("counter").toInt())
                             .arg(map.value("limit").toInt()));
    }
        break;
    case CC_ERROR_SCENE_NOT_FOUND:
    {
        QString sceneName = arg.toString();
        QMessageBox::warning(this, tr("RayPump error"), tr("Failed to start scheduled scene: %1<br><br>Try to re-send your job. If problem presists, please report a bug").arg(sceneName));
    }
        break;
    case CC_ERROR_SCENE_TEST_FAILED:
    {
        QVariantMap map = arg.toMap();
        QString sceneName = map.value("scene_name").toString();
        QString reason = map.value("reason").toString();
        show();
        raise();
        activateWindow();
        if (reason == "crashed"){
            QMessageBox::warning(this, tr("RayPump error"), tr("This scene will not blend: %1<br><br>Your scene fails to run on GPUs").arg(sceneName));
            m_status = tr("Stress-test failed for %1").arg(sceneName);
        }
        else if (reason == "timed-out"){
            QMessageBox::warning(this, tr("RayPump error"), tr("This scene will not blend: %1<br><br>Your scene is to heavy for this RayPump Package").arg(sceneName));
            m_status = tr("Stress-test timed-out for %1").arg(sceneName);
        }
        else{
            uERROR << "unknown type of scene-failed error";
        }
        ui->statusBar->clearMessage();
    }
        break;    
    case CC_CONFIRM_GENERAL_INFO:
        m_trayIcon->showMessage("RayPump Farm message:", arg.toString(), QSystemTrayIcon::Warning);
        break;
    case CC_CONFIRM_IMPORTANT_INFO:
        QMessageBox::warning(this, tr("RayPump Farm message"), arg.toString());
        break;
    case CC_CONFIRM_QUEUE_STATUS:
    {
        QVariantMap jobs = arg.toMap();
        jobs.remove("command");
        m_jobManager->setJobs(jobs);
    }
        break;
    default:
        uERROR << "unhandled command" << command << arg;
        break;
    }
}

void RayPumpWindow::handleSceneReady(const QString &sceneName)
{
    uINFO << "scene ready" << sceneName;
    ui->statusBar->showMessage(tr("Downloading..."));
    transferRenders();
}

void RayPumpWindow::handleTrayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    switch ((int)reason){
    case QSystemTrayIcon::MiddleClick:
        on_actionFolder_triggered();
        break;
    case QSystemTrayIcon::Trigger:
        m_trayIcon->showMessage(tr("RayPump status"), m_status, QSystemTrayIcon::Information, 3000);
        break;
    }
}

void RayPumpWindow::handleTrayIconMessageClicked()
{
    on_actionShow_triggered();
}

void RayPumpWindow::showHideWindow()
{
    if(this->isVisible())
    {
        this->hide();
    }
    else
    {
        on_actionShow_triggered();
    }
}


void RayPumpWindow::on_actionConnect_toggled(bool checked)
{
    if (checked){
        if(ui->lineEditUserName->text().isEmpty()){
            ui->lineEditUserName->setFocus();
            ui->actionConnect->setChecked(false);
            return;
        }
        else if (ui->lineEditUserPass->text().isEmpty()){
            ui->lineEditUserPass->setFocus();
            ui->actionConnect->setChecked(false);
            return;
        }
        if (!checkMalformedUsername(ui->lineEditUserName->text())){
            m_trayIcon->showMessage(tr("RayPump Warning"), tr("This username might not work with RayPump (contains spaces, illegal chars, etc.)"), QSystemTrayIcon::Warning);
        }
        ui->statusBar->showMessage(tr("Connecting..."));
        m_remoteClient->connectRayPump();
        if (ui->checkBoxRememberUser->isChecked()){
            SimpleCrypt cryptor(m_simpleCryptKey);
            QSettings settings;
            settings.setValue("username", ui->lineEditUserName->text());
            settings.setValue("password", cryptor.encryptToString(ui->lineEditUserPass->text()));
        }

    }
    else{
        if (ui->lineEditUserName->text().isEmpty() || ui->lineEditUserPass->text().isEmpty()){
            return;
        }
        if (!ui->actionConnect->property("force").toBool())
        {
            QMessageBox msgBox;
            msgBox.setText(tr("Really disconnect?"));
            msgBox.setInformativeText(tr("Disconnecting might cause problems with uploads and running job."));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::No);
            int ret = msgBox.exec();
            switch (ret){
            case QMessageBox::Yes:
                // we continue;
                break;
            default:
                ui->actionConnect->blockSignals(true);
                ui->actionConnect->setChecked(true);
                ui->actionConnect->blockSignals(false);
                return;
                break;
            }
        }
        ui->statusBar->showMessage(tr("Disconnected"));
        m_status = tr("Disconnected");
        m_trayIcon->setIcon(QIcon(":/icons/icons/logo_small_off.ico"));
    }

    ui->lineEditUserName->setVisible(!ui->actionConnect->isChecked());
    ui->lineEditUserPass->setVisible(!ui->actionConnect->isChecked());
    ui->checkBoxRememberUser->setVisible(!ui->actionConnect->isChecked());
    ui->pushButtonConnect->setVisible(!ui->actionConnect->isChecked());
}

void RayPumpWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIconMenu = new QMenu(tr("RayPump tray"), this);
#ifdef Q_OS_MAC
    m_trayIcon->setToolTip(tr("RayPump - click to display menu"));
#else
    m_trayIcon->setToolTip(tr("RayPump - right click displays menu"));
#endif
    m_trayIconMenu->addAction(ui->actionShow);
    m_trayIconMenu->addAction(ui->actionFolder);
    m_trayIconMenu->addAction(ui->actionDownload);
    m_trayIconMenu->addAction(ui->actionQuit);
    m_trayIcon->setContextMenu(m_trayIconMenu);
    m_trayIconMenu->setDefaultAction(ui->actionShow);

    m_trayIcon->setIcon(QIcon(":/icons/icons/logo_small_off.ico"));
    m_trayIcon->show();

    connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(handleTrayIconClicked(QSystemTrayIcon::ActivationReason)));
    connect(m_trayIcon, SIGNAL(messageClicked()), SLOT(handleTrayIconMessageClicked()));

}

/// @note perhaps having rsync as library here would be more platform-independent and better in general. This might be implemented later.
void RayPumpWindow::setupRsyncProcesses()
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
        QMessageBox::warning(this, tr("RayPump failed"), tr("Installation is broken, please re-install RayPump client"));
        exit(EXIT_FAILURE);
    }
    m_rsyncSceneProcess = new QProcess(this);
    m_rsyncSceneProcess->setReadChannel(QProcess::StandardOutput);
    m_rsyncSceneProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_rsyncSceneProcess, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(handleRsyncSceneFinished(int,QProcess::ExitStatus)));
    connect(m_rsyncSceneProcess, SIGNAL(readyReadStandardOutput()), SLOT(handleReadyReadRsyncSceneOutput()));


    m_rsyncRendersProcess = new QProcess(this);
    m_rsyncRendersProcess->setReadChannel(QProcess::StandardOutput);
    m_rsyncRendersProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_rsyncRendersProcess, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(handleRsyncRendersFinished(int,QProcess::ExitStatus)));
    connect(m_rsyncRendersProcess, SIGNAL(readyReadStandardOutput()), SLOT(handleReadyReadRsyncRendersOutput()));
}

void RayPumpWindow::setupLocalServer()
{
    m_localServer = new LocalServer(this);
    m_localServer->start();
    connect(m_localServer, SIGNAL(messageReceived(QByteArray)), SLOT(handleLocalMessage(QByteArray)));
    connect(m_localServer, SIGNAL(requestShowWindow()), SLOT(show()));
}

void RayPumpWindow::setupRemoteClient()
{
    m_remoteClient = new RemoteClient(this);
    connect(m_remoteClient, SIGNAL(connected(bool,QString)), SLOT(handleConnectionStatus(bool,QString)));
    connect(m_remoteClient, SIGNAL(receivedRayPumpMessage(CommandCode,QVariant)), SLOT(handleRayPumpCommand(CommandCode,QVariant)));
    connect(m_remoteClient, SIGNAL(sceneReady(QString)), SLOT(handleSceneReady(QString)));
    m_synchroInProgress = false;
    m_downloadTryCounter = 0;
}

void RayPumpWindow::setupAutoconnection()
{
    QSettings settings;
    bool autoconnect = settings.value("autoconnect", false).toBool();
    if (autoconnect){
        ui->statusBar->showMessage(tr("Auto-connecting..."));
        SimpleCrypt cryptor(m_simpleCryptKey);
        QString pass = cryptor.decryptToString(settings.value("password").toString());
        ui->lineEditUserName->setText(settings.value("username").toString());
        ui->lineEditUserPass->setText(pass);
        ui->checkBoxRememberUser->setChecked(autoconnect);
        ui->actionConnect->toggle();
    }
}

void RayPumpWindow::setupJobManager()
{
    QSettings settings;
    bool advancedMode = settings.value("advanced", false).toBool();
    m_jobManager = new JobManager(ui->tableWidget, ui->lineEditCounters, ui->pushButtonCancelJob, this);
    connect(m_jobManager, SIGNAL(requestProgressDisplay(int,int)), SLOT(handleRenderProgressChanged(int,int)));
    connect(m_jobManager, SIGNAL(renderPointsChanged(int)), SLOT(handleRenderPointsChanged(int)));
    connect(m_jobManager, SIGNAL(requestStatusBarMessage(QString)), SLOT(handleJobManagerStatusBarMessage(QString)));

    ui->groupBoxAdvancedMode->setChecked(advancedMode);
}

bool RayPumpWindow::transferScene(const QFileInfo &sceneFileInfo)
{
    if (!m_remoteClient->isConnected()){
        uERROR << "not connected to server";
        return false;
    }

    if (m_synchroInProgress){
        m_trayIcon->showMessage(tr("RayPump is uploading"), tr("Please wait until previous job get uploaded"), QSystemTrayIcon::Warning);
        uERROR << "synchronization already in progress. Aborting...";
        return false;
    }

    if(m_blenderAddonVersion < (qreal)G_VERSION){
        uERROR << m_blenderAddonVersion << "instead of" << G_VERSION << "outdated Blender add-on version. Aborting...";
        QMessageBox::warning(this, tr("RayPump error"), tr("Your Blender's add-on version is outdated.<br><br>Please install the newest version from current RayPump directory"));
        return false;
    }

    if (m_rsyncSceneProcess->state() != QProcess::NotRunning){
        uERROR << "wrong process status (running?)";
        ui->statusBar->showMessage("Wrong process status");
        return false;
    }

    if (!sceneFileInfo.isFile()){
        uERROR << "scene file does not exist" << sceneFileInfo.absoluteFilePath();
        return false;
    }

    if (!ui->actionConnect->isChecked()){
        uERROR << "not connected to the RayPump. Aborting...";
        ui->statusBar->showMessage("Not connected");
        m_trayIcon->showMessage("RayPump", tr("Client is not connected"), QSystemTrayIcon::Warning);
        return false;
    }

    assertSynchroDirectories();
    QVariantMap arg;
    arg["scene_name"]=sceneFileInfo.fileName();
    arg["frame_number_current"] = m_currentJob.frameCurrent;
    arg["frame_number_start"] = m_currentJob.frameStart;
    arg["frame_number_end"] = m_currentJob.frameEnd;
    arg["job_type"] = m_currentJob.jobType;
    m_remoteClient->sendRayPumpMessage(CC_REQUEST_SCENE_PREPARE, arg);

    QStringList env = QProcess::systemEnvironment();
    env << "RSYNC_PASSWORD=" + m_remoteClient->accessHash();

    QStringList arguments;

#ifdef Q_OS_WIN
    if (!m_jobManager->uploadLimit()){
        arguments << "--progress" << "-z" << "--compress-level=9" << Globals::BUFFER_DIRECTORY + "/" + sceneFileInfo.fileName() << "rsync://" + ui->lineEditUserName->text() + "@" + Globals::SERVER_IP + "/share/" + ui->lineEditUserName->text() + "/BUFFER/";
    }
    else{
        uINFO << "limiting upload to" << m_jobManager->uploadLimit() << "KB/s";
        arguments << "--progress" << "-z" << "--compress-level=9" << QString("--bwlimit=%1").arg(m_jobManager->uploadLimit())  << Globals::BUFFER_DIRECTORY + "/" + sceneFileInfo.fileName() << "rsync://" + ui->lineEditUserName->text() + "@" + Globals::SERVER_IP + "/share/" + ui->lineEditUserName->text() + "/BUFFER/";
    }
#else
    if (Globals::SERVER_VIRTUALIZED){
        QHostInfo info = QHostInfo::fromName(Globals::SERVER_HOST_NAME);
        if (info.addresses().isEmpty()){
            uERROR << "cannot get IP address for" << Globals::SERVER_HOST_NAME;
            return false;
        }
        arguments << "--progress" << "-z" << "--compress-level=9" << Globals::BUFFER_DIRECTORY + "/" + sceneFileInfo.fileName() << "rsync://" + ui->lineEditUserName->text() + "@" + Globals::SERVER_HOST_NAME + "/share/" + ui->lineEditUserName->text() + "/BUFFER/";
    }
    else{
        arguments << "--progress" << "-z" << "--compress-level=9" << Globals::BUFFER_DIRECTORY + "/" + sceneFileInfo.fileName() << "rsync://" + ui->lineEditUserName->text() + "@" + Globals::SERVER_IP + "/share/" + ui->lineEditUserName->text() + "/BUFFER/";
    }
#endif

    m_rsyncTimer.start();
    m_totalJobTimer.start();
    m_rsyncSceneProcess->setEnvironment(env);
    m_rsyncSceneProcess->start(m_rsyncFilePath.absoluteFilePath(), arguments);
    if (!m_rsyncSceneProcess->waitForStarted()){
        uERROR << "failed to start rsync process for scene";
        m_rsyncTimer.invalidate();
        return false;
    }

    ui->statusBar->showMessage(tr("Transferring %1").arg(sceneFileInfo.baseName()));
    ui->progressBar->show();
    ui->progressBar->reset();
    m_status = tr("Transfering scene %1").arg(sceneFileInfo.baseName());

    m_synchroInProgress = true;
    return true;
}

void RayPumpWindow::transferRenders()
{
    if (!m_remoteClient->isConnected()){
        uERROR << "not connected to the server";
        return;
    }

    if (m_rsyncRendersProcess->state() == QProcess::Running){
        uWARNING << "process is still running. Aborting...";
        return;
    }

    QStringList env = QProcess::systemEnvironment();
    env << "RSYNC_PASSWORD=" + m_remoteClient->accessHash();
    m_rsyncRendersProcess->setEnvironment(env);

    QStringList arguments;
    //arguments << "--exclude=*.blend" << "-r" << "rsync://" + ui->lineEditUserName->text() + "@" + G_SERVER_IP + "/renders/" + ui->lineEditUserName->text() + "/" << Globals::RENDERS_DIRECTORY + "/";
    //arguments << "--exclude=*.blend" << "-r" << "rsync://" + ui->lineEditUserName->text() + "@" + G_SERVER_IP + "/renders/" + ui->lineEditUserName->text() + "/" << Globals::RENDERS_DIRECTORY + "/";

//    arguments  "--no-p" << "--no-g" << "--chmod=ugo=rwX" ;
//    m_rsyncRendersProcess->start(m_rsyncFilePath.absoluteFilePath(), arguments);
//    if (!m_rsyncRendersProcess->waitForStarted()){
//        uWARNING << "failed to setr sync";
//    }

//    arguments.clear();

    arguments << "--progress" << "--perms"  << "--exclude=*.blend" << "-zr" << "--compress-level=9" << "rsync://" + ui->lineEditUserName->text() + "@" + Globals::SERVER_IP + "/renders/" + ui->lineEditUserName->text() + "/" << Globals::RENDERS_DIRECTORY + "/";


    m_rsyncRendersProcess->start(m_rsyncFilePath.absoluteFilePath(), arguments);
    if (!m_rsyncRendersProcess->waitForStarted()){
        uERROR << "failed to start rsync process for renders";
    }
    else{
        ui->statusBar->showMessage(tr("Starting download..."));
    }
}

void RayPumpWindow::handleJobManagerStatusBarMessage(const QString &message)
{
    ui->statusBar->showMessage(message);
}

void RayPumpWindow::handleRenderProgressChanged(int progress, int total)
{
    ui->progressBarRender->setMaximum(total);
    ui->progressBarRender->setValue(progress);
    ui->progressBarRender->setVisible(total);
    if (!total){
        //ui->statusBar->showMessage(tr("Awaiting resources..."));
    }
    else{
           ui->statusBar->showMessage(tr("Pumping rays..."));
    }

}

void RayPumpWindow::handleReadyReadRsyncSceneOutput()
{

    if (m_rsyncSceneProcess->state() != QProcess::Running){
        uERROR << "scene sync process is not running";
        ui->progressBar->setMaximum(0);
        ui->progressBar->reset();
        return;
    }

    QByteArray output = m_rsyncSceneProcess->readAllStandardOutput().trimmed();
    int indexOfPercent = output.indexOf("%");
    if (indexOfPercent ==-1){
        ui->progressBar->setMaximum(0);
        ui->progressBar->reset();
        return;
    }

    int percent = output.mid(indexOfPercent-2, 2).toInt();
    ui->progressBar->setValue(percent);
    ui->progressBar->setMaximum(100);

}

void RayPumpWindow::handleReadyReadRsyncRendersOutput()
{
    if (m_rsyncRendersProcess->state() != QProcess::Running){
        uERROR << "renders sync process is not running";
        return;
    }

    QByteArray output = m_rsyncRendersProcess->readAllStandardOutput().trimmed();
    int indexOfPercent = output.indexOf("%");
    if (indexOfPercent ==-1){
        return;
    }
    int percent = output.mid(indexOfPercent-2, 2).toInt();
    if (percent){
        ui->statusBar->showMessage(tr("Downloading %1%").arg(percent));
    }
}

void RayPumpWindow::handleRenderPointsChanged(int renderPoints)
{
    if (renderPoints == m_renderPoints){
//        uINFO << "no real change. Aborting...";
        return;
    }

    m_renderPoints = renderPoints;
    if (m_renderPoints < 1){
        m_trayIcon->showMessage("RayPump", tr("No Render Points left"));
    }
    else if (m_renderPoints < 50){
        m_trayIcon->showMessage("RayPump", tr("Less than 50 Render Points left - no more commercial jobs can be scheduled"));
    }
    else if (m_renderPoints < 200){
        m_trayIcon->showMessage("RayPump", tr("Less than 200 Render Points left"));
    }
}

void RayPumpWindow::assertSynchroDirectories()
{
    QDir path(QDir::homePath());

    if (!path.mkpath(Globals::RENDERS_DIRECTORY)){
        QMessageBox::critical(0, "RayPump failed", "Failed to create renders directory " + Globals::RENDERS_DIRECTORY);
        Globals::RENDERS_DIRECTORY = "RayPump";
        if (!path.mkpath(Globals::RENDERS_DIRECTORY)){   //if the specified directory isn't avaliable, use <Home>/RayPump
            QMessageBox::critical(0, "RayPump failed", "Failed to create directory");
            exit(EXIT_FAILURE);
        }
    }
}

/// @todo doesn't work yet (should fix Windows' rsync bug that set persmissions to public)
void RayPumpWindow::setRenderFilesPermission()
{

    QDir renderPath(Globals::RENDERS_DIRECTORY);
    if (!renderPath.exists()){
        uERROR << "cannot find RENDERS directory. Aborting...";
        return;
    }

    foreach (QFileInfo dirInfo, renderPath.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs)){
        //uINFO << "dir" << dirInfo.absoluteFilePath();
//        QFile::setPermissions(dirInfo.absoluteFilePath(),
//                              QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner
//                              | QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup
//                              | QFile::ReadOther | QFile::WriteOther | QFile::ExeOther
//                              );
    }

}

void RayPumpWindow::calculateJobTime(const QString &sceneName)
{
    if (sceneName.isEmpty()){
        uWARNING << "empty scene name. Aborting...";
        return;
    }
    if (!m_jobStartTimes.contains(sceneName)){
        uWARNING << "cannot find this scene name in times cache" << sceneName << ". Aborting...";
        return;
    }


    int jobDuration = m_jobStartTimes.value(sceneName).secsTo(QDateTime::currentDateTime());
    int hours = jobDuration/60/60;
    int minutes = (jobDuration/60)%60;
    int seconds = jobDuration%60;

    uINFO << "scene" << sceneName << "took" << jobDuration << "seconds to render";
    if (!hours && !minutes){
        m_trayIcon->showMessage("RayPump", tr("Last job(s) rendered in %1 seconds").arg(seconds));
        ui->statusBar->showMessage(tr("Job(s) rendered in %1 seconds").arg(seconds));
    }
    else if (!hours){
        m_trayIcon->showMessage("RayPump", tr("Job(s) rendered in %1 min %2 sec").arg(minutes).arg(seconds));
        ui->statusBar->showMessage(tr("Job(s) rendered in %1 min %2 sec").arg(minutes).arg(seconds));
    }
    else{
        m_trayIcon->showMessage("RayPump", tr("Job(s) rendered in %1 h %2 min %3 sec").arg(hours).arg(minutes).arg(seconds));
        ui->statusBar->showMessage(tr("Job(s) rendered in %1 h %2 min %3 sec").arg(hours).arg(minutes).arg(seconds));
    }

}

bool RayPumpWindow::checkMalformedUsername(const QString &userName)
{
    return (!userName.contains(QRegExp("[\\s\\^@!]")));
}

void RayPumpWindow::openRenderFolder(const QString &subfolderName)
{
#ifdef Q_OS_MAC
    if (subfolderName.isEmpty()){
        QUrl rendersPathUrl(Globals::RENDERS_DIRECTORY);
        rendersPathUrl.setScheme("file");
        QDesktopServices::openUrl(rendersPathUrl);
    }
    else{
        QUrl rendersPathUrl(Globals::RENDERS_DIRECTORY +"/"+subfolderName);
        rendersPathUrl.setScheme("file");
        QDesktopServices::openUrl(rendersPathUrl);
    }

#elif defined(Q_OS_WIN)
    if (subfolderName.isEmpty()){
        if (!QDesktopServices::openUrl(Globals::RENDERS_DIRECTORY)){
            ui->statusBar->showMessage(tr("Failed to open %1").arg(Globals::RENDERS_DIRECTORY));
        }
    }
    else{
        if (!QDesktopServices::openUrl(Globals::RENDERS_DIRECTORY + "\\" + subfolderName)){
            ui->statusBar->showMessage(tr("Failed to open %1").arg(Globals::RENDERS_DIRECTORY + "\\" + subfolderName));
        }
    }
#elif defined(Q_OS_LINUX)
    if (subfolderName.isEmpty()){
        if (!QDesktopServices::openUrl(Globals::RENDERS_DIRECTORY)){
            ui->statusBar->showMessage(tr("Failed to open %1").arg(Globals::RENDERS_DIRECTORY));
        }
    }
    else{
        if (!QDesktopServices::openUrl(Globals::RENDERS_DIRECTORY + "/" + subfolderName)){
            ui->statusBar->showMessage(tr("Failed to open %1").arg(Globals::RENDERS_DIRECTORY + "/" + subfolderName));
        }
    }
#endif    

    ui->statusBar->showMessage(tr("Opening folder..."));
    m_trayIcon->setIcon(QIcon(":/icons/icons/logo_small.ico"));
}

void RayPumpWindow::setJobType(const QString &jobTypeString)
{
    if (jobTypeString.isEmpty()){
        uERROR << "empty job type string. Aborting...";
        return;
    }

    uINFO << "blender sets the job type to" << jobTypeString;

    if (jobTypeString == "FREE"){
        m_currentJob.jobType = PT_FREE;
    }
    else if (jobTypeString == "STATIC"){
        m_currentJob.jobType = PT_COMMERCIAL_STATIC;
    }
    else if (jobTypeString == "ANIMATION"){
        m_currentJob.jobType = PT_COMMERCIAL_ANIMATION;
    }
    else if (jobTypeString == "STRESS-TEST"){
        m_currentJob.jobType = PT_STRESSTEST;
    }
    else{
        uERROR << "unhandled job type" << jobTypeString;
    }

}

QString RayPumpWindow::formattedStringFromSeconds(int duration)
{
    uINFO << duration;
    int seconds = (int) (duration % 60);
    duration /= 60;
    int minutes = (int) (duration % 60);
    duration /= 60;
    int hours = (int) (duration % 24);
    return QString("%1:%2:%3 (hh:mm:ss)")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
}

void RayPumpWindow::setupLicenseAgreement()
{
    QSettings settings;
    if (!settings.value("license_agreement", false).toBool()){
        QMessageBox msg;
        msg.setText(tr("License agreement"));
        msg.setInformativeText(tr("RayPump requires your acceptance of <a href=http://www.raypump.com/index.php/help/3-disclaimer>the terms and conditions</a>.<br><br>Have you read and accepted them?"));
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setDefaultButton(QMessageBox::No);

        int ret = msg.exec();
        if (ret != QMessageBox::Yes){
            //QApplication::quit();
            exit(0);
            return;
        }

        uINFO << "Accepted";
        settings.setValue("license_agreement", true);
    }
}

void RayPumpWindow::on_actionQuit_triggered()
{
    m_wantToQuit = true;
    emit close();
}

void RayPumpWindow::on_actionAbout_triggered()
{
#ifndef QT_NO_DEBUG
    QMessageBox::about(this,
                       tr("About RayPump"),
                       tr("<b>RayPump Online Cycles Accelerator for Blender</b><br><a href=http://www.raypump.com>www.RayPump.com</a><br><br>version %1 '%4'<br><br>by michal.mielczynski@gmail.com<br><br>User: %3<br><br><b> DEVELOPER VERSION </b>")
                       .arg(G_VERSION)
                       .arg(ui->lineEditUserName->text())
                       .arg(G_VERSION_NAME)
                       );
#else
    QMessageBox::about(this,
                       tr("About RayPump"),
                       tr("<b>RayPump Online Cycles Accelerator for Blender</b><br><a href=http://www.raypump.com>www.RayPump.com</a><br><br>version %1 '%4'<br><br>by michal.mielczynski@gmail.com<br><br>User: %3<br><br>This version uses Render Points system")
                       .arg(G_VERSION)
                       .arg(ui->lineEditUserName->text())
                       .arg(G_VERSION_NAME)
                       );
#endif
}

void RayPumpWindow::on_actionWebsite_triggered()
{
    QDesktopServices::openUrl(QUrl("http://www.raypump.com/help"));
}

void RayPumpWindow::on_actionDownload_triggered()
{
    ui->statusBar->showMessage(tr("Downloading..."));
    assertSynchroDirectories();
    transferRenders();
}

void RayPumpWindow::on_actionFolder_triggered()
{
    openRenderFolder();
}

void RayPumpWindow::on_actionShow_triggered()
{
    this->show();
    this->raise();
    activateWindow(); // works under Linux, but not always under Windows
}

void RayPumpWindow::on_checkBoxRememberUser_toggled(bool checked)
{
    QSettings settings;
    settings.setValue("autoconnect", checked);
}

/// @todo method not working as expected
void RayPumpWindow::on_actionAlways_on_Top_triggered(bool checked)
{
    Qt::WindowFlags flags = windowFlags();
    if (checked){
        uINFO << "stay on top";
        this->setWindowFlags(flags | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
        this->show();
    }
    else{
        uINFO << "do not stay on top";
        this->setWindowFlags(flags ^ (Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint));
        this->show();
    }
}

void RayPumpWindow::on_lineEditUserName_returnPressed()
{
    if (!ui->actionConnect->isChecked()){
        if (!ui->lineEditUserPass->text().isEmpty()){
            ui->actionConnect->toggle();
        }
        else{
            ui->lineEditUserPass->setFocus();
        }
    }
}

void RayPumpWindow::on_actionAdd_Render_Points_triggered()
{
    QDesktopServices::openUrl(QUrl("http://raypump.com/index.php/buy/renderpoints"));
}

void RayPumpWindow::on_actionCancel_uploading_triggered()
{
    if (m_rsyncSceneProcess->state() != QProcess::Running ){
        uINFO << "rsync not running";
        return;
    }

    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure?"));
    msgBox.setInformativeText(tr("Do you want to abort current scene transfer?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    switch (ret){
    case QMessageBox::Yes:
        //pass
        break;
    default:
        return;
        break;
    }

    m_rsyncSceneProcess->kill();
}

void RayPumpWindow::on_tableWidget_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    openRenderFolder(ui->tableWidget->item(row, 0)->data(IR_JOB_PATH).toString());
}

void RayPumpWindow::on_pushButtonCleanUp_clicked()
{
    QVariantMap args;
    m_remoteClient->sendRayPumpMessage(CC_REQUEST_CLEANUPDONEJOBS, args);
}

void RayPumpWindow::on_pushButtonRefresh_clicked()
{
    QVariantMap args;
    m_remoteClient->sendRayPumpMessage(CC_REQUEST_READQUEUE, args);
}

void RayPumpWindow::on_pushButtonConnect_clicked()
{
    ui->actionConnect->toggle();
}


void RayPumpWindow::on_groupBoxAdvancedMode_toggled(bool toggled)
{
    QSettings settings;
    foreach (QWidget* widget, ui->groupBoxAdvancedMode->findChildren<QWidget *>()){
        widget->setVisible(toggled);
    }

    settings.setValue("advanced", toggled);

    if (!toggled)
    {
        setMinimumSize(200,270);
        setMaximumSize(200,300);
//        ui->groupBoxAdvancedMode->setTitle(tr("Show advanced tools"));
    }
    else{
        setMinimumSize(0,0);
        setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
//        ui->groupBoxAdvancedMode->setTitle(tr("Hide advanced tools"));
    }
    adjustSize();
}

void RayPumpWindow::on_pushButtonCancelJob_clicked()
{
    QMessageBox msg;
    msg.setText(tr("Are you sure?"));
    msg.setInformativeText(tr("All selected jobs will be cancelled"));
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    int ret = msg.exec();
    if (ret != QMessageBox::Yes){
        uINFO << "Aborted";
        return;
    }

    QSet<QString> jobs;
    foreach(QTableWidgetItem *item, ui->tableWidget->selectedItems()){
        jobs << ui->tableWidget->item(item->row(), 0)->text();
    }


    QVariantMap args;
    QStringList jobsList = jobs.toList();
    args.insert("job_name", jobsList);
    m_remoteClient->sendRayPumpMessage(CC_REQUEST_CANCELJOB, args);
    on_actionCancel_uploading_triggered();
}

void RayPumpWindow::on_pushButtonRenderPath_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    QString newPath = dialog.getExistingDirectory(this, tr("Choose directory"), Globals::RENDERS_DIRECTORY, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (newPath != "")
        Globals::RENDERS_DIRECTORY = newPath;
    QSettings settings;
    settings.setValue("renders_directory", Globals::RENDERS_DIRECTORY);
    ui->pushButtonRenderPath->setText("Change renders directory (" + Globals::RENDERS_DIRECTORY + ")");
}
