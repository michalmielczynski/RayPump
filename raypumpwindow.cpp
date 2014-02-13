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

#include "raypumpwindow.h"
#include "ui_raypumpwindow.h"

RayPumpWindow::RayPumpWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RayPumpWindow)
{
    /// @note we leave the constructor empty, in case we find other RayPump instance to wake up
}

RayPumpWindow::~RayPumpWindow()
{
    delete ui;
}

void RayPumpWindow::run()
{
    uINFO << "RayPump Client" << G_VERSION << "(c) michal.mielczynski@gmail.com. All rights reserved.";
    m_simpleCryptKey = 1365559412; //:)

    ui->setupUi(this);
#ifndef QT_NO_DEBUG
    setWindowTitle("TEST VERSION");
#endif
#if defined(Q_OS_MAC)
    ui->menuBar->setNativeMenuBar(true);
#endif
    ui->progressBar->hide();
    ui->progressBarRender->hide();
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->actionConnect->setProperty("force", false);

    setupLicenseAgreement();
    setupTrayIcon();
    setupRsyncWrappers();
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
    cleanUpBufferDirectory();

    m_status = tr("Not connected");
    m_currentJob.clear();
    m_blenderAddonVersion = 0.0;

}

void RayPumpWindow::handleInstanceWakeup(const QString &message)
{
    uINFO << message;
    on_actionShow_triggered();
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
        if (m_sceneTransferManager->synchroInProgress()){
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
            if (!m_sceneTransferManager->isHashValid()){
                uINFO << "Received job disconnected to the server";
                m_localServer->sendRetry();
            } else
                m_localServer->confirmSceneScheduled(transferScene(map.value(key).toString()));
        }
        else if (key == "CONNECTED"){
            m_status = tr("Blender connected");
        }
        else if (key == "DISCONNECTED"){
            ui->statusBar->showMessage(tr("Blender disconnected"));
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
        else if (key == "EXTERNAL_PATHS"){
           m_currentJob.externalPaths = map.value(key).toStringList();
        }
        else if (key == "VIEW"){
            if (!m_jobManager->lastReadyRenderPath().isEmpty()){
                openRenderFolder(m_jobManager->lastReadyRenderPath());
            }
        }
        /// @todo more to come?
        else{
            uERROR << "unhandled command" << key;
        }
    }
}

void RayPumpWindow::handleRsyncSceneFinished(bool success)
{
    if (success){
        m_trayIcon->showMessage("RayPump", tr("Uploaded in %1 s").arg(m_sceneTransferManager->elapsed()/1000), QSystemTrayIcon::Information, 3000);
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
    cleanUpBufferDirectory();
}

void RayPumpWindow::handleRsyncRendersFinished(bool success)
{
    if (success){
        m_downloadTryCounter = 0;
        ui->progressBarRender->hide();
        m_trayIcon->showMessage("RayPump", tr("Renders downloaded"), QSystemTrayIcon::Information, 10000);
        ui->statusBar->showMessage(tr("Download complete"));
        m_status = tr("Download complete");
        m_trayIcon->setIcon(QIcon(":/icons/icons/logo_small_glow.ico"));
        uINFO << "Transfer completed";
        QVariantMap map;
        m_remoteClient->sendRayPumpMessage(CC_REQUEST_RENDERS_CLEANUP, map);
        setRenderFilesPermission();

    }
    else if (m_downloadTryCounter > 3){
        ui->statusBar->showMessage(tr("Download failed"));
        m_status = tr("Download failed");
        uERROR << "failed to download renders";
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

void RayPumpWindow::handleRayPumpCommand(CommandCode command, const QVariantMap &arg)
{    
    switch(command){
    case CC_CONFIRM_AUTH:
    {
        uINFO << "auth confirmation required" << arg.value("reason");
        QVariantMap args;
        args["name"] = ui->lineEditUserName->text().toLower();
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
        if (arg.value("valid").toBool()){
            ui->actionConnect->setChecked(true);
            m_trayIcon->setIcon(QIcon(":/icons/icons/logo_small.ico"));
        }
        else{
            ui->actionConnect->setProperty("force", true);
            ui->actionConnect->setChecked(false);
            ui->statusBar->showMessage(tr("Authentication failed"));
        }
        QByteArray accessHash = arg.value("hash").toByteArray();
        m_rendersTransferManager->setAccessHash(accessHash);
        m_sceneTransferManager->setAccessHash(accessHash);

        QVariantMap empty;
        m_remoteClient->sendRayPumpMessage(CC_REQUEST_READQUEUE, empty); // reads also Render Points

        break;
    }
    case CC_CONFIRM_SCENE_PREPARED:
    {
        QString jobName = arg.value("job_name").toString();
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
        int seconds = arg.value("seconds", 0).toInt();
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
        QString sceneName = arg.value("scene_name").toString();
        ui->progressBarRender->setProperty("testing", true);
        m_trayIcon->showMessage("RayPump", tr("Testing: %1").arg(sceneName), QSystemTrayIcon::Information, 2000);
        m_status = tr("Testing scene %1").arg(sceneName);
        uINFO << "testing scene" << sceneName;
    }
        break;
    case CC_CONFIRM_SCENE_READY:
        handleSceneReady(arg.value("scene_name").toString());
        break;
    case CC_CONFIRM_SCENE_RUNNING:
    {
        QString sceneName = arg.value("scene_name").toString();
        uINFO << "scene confirmed (running)" << sceneName;
        if (!sceneName.isEmpty()){
            m_trayIcon->showMessage("RayPump", tr("Starting %1").arg(sceneName), QSystemTrayIcon::Information, 3000);
            m_status = tr("Rendering %1").arg(sceneName);
            uINFO << "rendering scene" << sceneName;
        }
        ui->statusBar->showMessage(tr("Pumping rays..."));
    }
        break;
    case CC_CONFIRM_DOWNLOAD_READY:
    {
        uINFO << "download ready";
        transferRenders();
    }
        break;
    case CC_CONFIRM_JOBLIMIT_EXCEEDED:
    {
        int limit = arg.value("limit").toInt();
        uERROR << "job limit exceeded" << limit;
        show();
        raise();
        activateWindow();
        m_trayIcon->showMessage("RayPump", tr("Queue limit reached"), QSystemTrayIcon::Warning);
        m_status = tr("Queue limit reached");
        QMessageBox::warning(this, tr("RayPump limitation"), tr("User's jobs queue is limited<br><br>Please wait for previously scheduled job(s) to complete").arg(limit));
    }
        break;
    case CC_CONFIRM_DAILYJOBLIMIT_EXCEEDED:
    {
        show();
        raise();
        activateWindow();
        m_trayIcon->showMessage("RayPump", tr("Job limit reached"), QSystemTrayIcon::Warning);
        m_status = tr("Job limit reached");
        QMessageBox::warning(this,
                             tr("RayPump limitation"),
                             tr("%1: daily job count exceeded limit (%2 of %3)")
                             .arg(arg.value("package").toString().toUpper())
                             .arg(arg.value("counter").toInt())
                             .arg(arg.value("limit").toInt()));
    }
        break;
    case CC_ERROR_SCENE_NOT_FOUND:
    {
        QString sceneName = arg.value("scene_name").toString();
        QMessageBox::warning(this, tr("RayPump error"), tr("Failed to start scheduled scene: %1<br><br>Try to re-send your job. If problem presists, please report a bug").arg(sceneName));
    }
        break;
    case CC_ERROR_SCENE_TEST_FAILED:
    {
        QString sceneName = arg.value("scene_name").toString();
        QString reason = arg.value("reason").toString();
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
        m_trayIcon->showMessage("RayPump Farm message:", arg.value("message").toString(), QSystemTrayIcon::Warning);
        break;
    case CC_CONFIRM_IMPORTANT_INFO:
        QMessageBox::warning(this, tr("RayPump Farm message"), arg.value("message").toString());
        break;
    case CC_CONFIRM_QUEUE_STATUS:
    {
        QVariantMap jobs = arg;
        jobs.remove("command");
        m_jobManager->setJobs(jobs);
    }
        break;
    case CC_CONFIRM_QUEUE_PROGRESS:
        handleOtherUserJobProgress(arg.value("queue_progress").toDouble());
        break;
    default:
        uERROR << "unhandled command" << command << arg;
        break;
    }
}

void RayPumpWindow::handleSceneReady(const QString &sceneName)
{
    if (sceneName.isEmpty()){
        uERROR << "empty scene name";
        return;
    }
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

        //m_remoteClient->connectRayPump();
        m_remoteClient->getIP();

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
void RayPumpWindow::setupRsyncWrappers()
{
    m_sceneTransferManager = new RsyncWrapper(this);
    connect(m_sceneTransferManager, SIGNAL(rsyncOutput(QByteArray)), SLOT(handleReadyReadRsyncSceneOutput(QByteArray)));
    connect(m_sceneTransferManager, SIGNAL(finished(bool)), SLOT(handleRsyncSceneFinished(bool)));

    m_rendersTransferManager = new RsyncWrapper(this);
    connect(m_rendersTransferManager, SIGNAL(rsyncOutput(QByteArray)), SLOT(handleReadyReadRsyncRendersOutput(QByteArray)));
    connect(m_rendersTransferManager, SIGNAL(finished(bool)), SLOT(handleRsyncRendersFinished(bool)));

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
    connect(m_remoteClient, SIGNAL(receivedRayPumpMessage(CommandCode,QVariantMap)), SLOT(handleRayPumpCommand(CommandCode,QVariantMap)));
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

    if (m_sceneTransferManager->synchroInProgress()){
        m_trayIcon->showMessage(tr("RayPump is uploading"), tr("Please wait until previous job get uploaded"), QSystemTrayIcon::Warning);
        uERROR << "synchronization already in progress. Aborting...";
        return false;
    }

    if(m_blenderAddonVersion < (qreal)G_ALLOWED_ADDON_VERSION){
        uERROR << m_blenderAddonVersion << "instead of" << G_ALLOWED_ADDON_VERSION << "outdated Blender add-on version. Aborting...";
        QMessageBox::warning(this, tr("RayPump error"), tr("Your Blender's add-on version is outdated.<br><br>Please install the newest version from current RayPump directory"));
        return false;
    }

    if (m_sceneTransferManager->isRunning()){
        uERROR << "wrong process status (running/starting)";
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


    QString destinationDirectory = "rsync://" + ui->lineEditUserName->text().toLower() + "@" + Globals::SERVER_IP + "/share/" + ui->lineEditUserName->text().toLower() + "/BUFFER/" + sceneFileInfo.fileName() + "/";
    m_sceneTransferManager->buffer(Globals::BUFFER_DIRECTORY, destinationDirectory);

    foreach (QString externalPath, m_currentJob.externalPaths){
        m_sceneTransferManager->buffer(externalPath, destinationDirectory);
    }

    if (m_sceneTransferManager->run()){
        ui->statusBar->showMessage(tr("Transferring %1").arg(sceneFileInfo.baseName()));
        ui->progressBar->show();
        ui->progressBar->reset();
        m_status = tr("Transfering scene %1").arg(sceneFileInfo.baseName());
        return true;
    }

    return false;
}

void RayPumpWindow::transferRenders()
{
    if (!m_remoteClient->isConnected()){
        uERROR << "not connected to the server";
        return;
    }

    m_rendersTransferManager->buffer(
                "rsync://" + ui->lineEditUserName->text().toLower() + "@" + Globals::SERVER_IP + "/renders/" + ui->lineEditUserName->text().toLower() + "/",
                Globals::RENDERS_DIRECTORY);

    if (!m_rendersTransferManager->run()){
        uERROR << "failed to run transfer manager";
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

void RayPumpWindow::handleReadyReadRsyncSceneOutput(const QByteArray output)
{
    int indexOfPercent = output.indexOf("%");
    if (indexOfPercent ==-1){
        ui->progressBar->setMaximum(0);
        ui->progressBar->reset();
        ui->statusBar->showMessage(tr("transfering: %1").arg(QString(output)));
        return;
    }

    int percent = output.mid(indexOfPercent-2, 2).toInt();
    ui->progressBar->setValue(percent);
    ui->progressBar->setMaximum(100);

}

void RayPumpWindow::handleReadyReadRsyncRendersOutput(const QByteArray output)
{
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
    else if (m_renderPoints < 100){
        m_trayIcon->showMessage("RayPump", tr("Less than 100 Render Points left - no more commercial jobs can be scheduled"));
    }
//    else if (m_renderPoints < 200){
//        m_trayIcon->showMessage("RayPump", tr("Less than 200 Render Points left"));
//    }
}

void RayPumpWindow::handleOtherUserJobProgress(double progress)
{
    if (!progress){
        return;
    }

    int value = (progress * 100.0);
    int range = 100 * 1+(int)progress;
    ui->progressBarRender->setVisible(true);
    ui->progressBarRender->setMaximum(range);
    ui->progressBarRender->setValue(range-value);
    uINFO << value << range;

    ui->statusBar->showMessage(tr("Awaiting free farm resources"));
}

void RayPumpWindow::assertSynchroDirectories()
{
    QSettings settings;

    Globals::RENDERS_DIRECTORY = settings.value("renders_path", QDir::homePath() + "/RayPump").toString();
    QDir rendersPath(Globals::RENDERS_DIRECTORY);
    if (!rendersPath.mkpath(Globals::RENDERS_DIRECTORY)){
        QMessageBox::critical(0, "", tr("Failed to read/create renders folder %1").arg(Globals::RENDERS_DIRECTORY));
    }

    ui->pushButtonRenderPath->setText(tr("Renders folder: %1 (click to change)").arg(Globals::RENDERS_DIRECTORY));

    /// @test
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString username;
#ifdef Q_OS_WIN
    username = env.value("USERNAME");
#else
    username = env.value("USER");
#endif

    QDir path(QDir::homePath());
    Globals::BUFFER_DIRECTORY = path.tempPath() + "/raypump-" + username;

    if (!path.mkpath(Globals::BUFFER_DIRECTORY)){
        QMessageBox::critical(0, "RayPump failed", "Failed to create buffer directory" + Globals::BUFFER_DIRECTORY);
        exit(EXIT_FAILURE);
    }
    /// @endcode

}

void RayPumpWindow::cleanUpBufferDirectory()
{
    clearDir(Globals::BUFFER_DIRECTORY);
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

/// @todo
void RayPumpWindow::calculateJobTime(const QString &sceneName)
{
    /// @deprecated
    /*
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
    */
}

bool RayPumpWindow::checkMalformedUsername(const QString &userName)
{
    return (!userName.contains(QRegExp("[\\s\\^@!]")));
}

void RayPumpWindow::openRenderFolder(const QString &subfolderName)
{
    QString path = Globals::RENDERS_DIRECTORY;
    m_trayIcon->setIcon(QIcon(":/icons/icons/logo_small.ico"));

    if (!subfolderName.isEmpty()){
        path += "/" + subfolderName;
    }

    if (!QDir(path).exists()){
        ui->statusBar->showMessage(tr("Directory %1 doesn't exist").arg(path));
        return;
    }
#ifdef Q_OS_WIN
    if (!QDesktopServices::openUrl("file:///" + path)){
#else
    if (!QDesktopServices::openUrl("file://" + path)){
#endif
        ui->statusBar->showMessage(tr("Failed to open %1").arg(path));
        uERROR << path << "failed to work with desktop services";
    }
    else{
        ui->statusBar->showMessage(tr("Opening folder %1").arg(path));
    }
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
                       tr("\
                          <b>RayPump<br><br>\
                          Online Cycles Accelerator for Blender</b><br>\
                          <a href=http://www.raypump.com>www.RayPump.com</a><br><br>\
                          version %1 '%4'<br><br>\
                          you: %3<br>")
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
    QDesktopServices::openUrl(QUrl("http://raypump.com/buy-render-points"));
}

void RayPumpWindow::on_actionCancel_uploading_triggered()
{
    if (!m_sceneTransferManager->isRunning()){
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

    m_sceneTransferManager->kill();
}

void RayPumpWindow::on_tableWidget_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    /// @todo: open only if the job has already ended
    openRenderFolder(ui->tableWidget->item(row, 0)->data(IR_JOB_PATH).toString());
}

void RayPumpWindow::on_pushButtonCleanUp_clicked()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QVariantMap args;
    m_remoteClient->sendRayPumpMessage(CC_REQUEST_CLEANUPDONEJOBS, args);
}

void RayPumpWindow::on_pushButtonRefresh_clicked()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
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

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_remoteClient->sendRayPumpMessage(CC_REQUEST_CANCELJOB, args);
    on_actionCancel_uploading_triggered();
}

void RayPumpWindow::on_pushButtonRenderPath_clicked()
{
    QSettings settings;
    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    QString newPath = dialog.getExistingDirectory(this, tr("Choose directory"), Globals::RENDERS_DIRECTORY, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (newPath != "")
        Globals::RENDERS_DIRECTORY = newPath;

    settings.setValue("renders_path", Globals::RENDERS_DIRECTORY);
    assertSynchroDirectories();
}

void RayPumpWindow::on_spinBoxUploadLimit_valueChanged(int arg1)
{
    m_sceneTransferManager->setTransferLimit(arg1);
}

void RayPumpWindow::on_actionCleanRemoteBuffer_triggered()
{
    QMessageBox msg;
    msg.setText(tr("Are you sure?"));
    msg.setInformativeText(tr("Clearing remote buffer will force uploading scenes from scratch. First upload might take a while."));
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    int ret = msg.exec();
    if (ret != QMessageBox::Yes){
        uINFO << "Aborted";
        return;
    }

    QVariantMap args;
    m_remoteClient->sendRayPumpMessage(CC_REQUEST_CLEANUPBUFFER, args);

}
