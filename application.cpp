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

#include "application.h"
#include "commoncode.h"

Application::Application(int &argc, char **argv):QApplication(argc, argv)
{
}

Application::~Application()
{
    delete _singular;   //lock is released and file is deleted when the object is destroyed
    QDir dir(Globals::BUFFER_DIRECTORY);
    dir.removeRecursively();
}

bool Application::lock()
{
    /* Create a local buffer for each user.
     * Qt has no standard way of getting the user name, so we read from the environment.
     * Some windows versions use the same temporary folder for all users, some have individual ones,
     * so using the usename might be unneeded under some systems.
     * The lock file will be placed inside the temporary directory.
     */

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

    QSettings settings;
    Globals::RENDERS_DIRECTORY = path.absoluteFilePath(settings.value("renders_directory", "RayPump").toString());

    _singular = new QLockFile(Globals::BUFFER_DIRECTORY + "/raypump_lock");

    return _singular->tryLock(0);
}
