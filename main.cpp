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

/** @note for developers
 * search throught the whole code for @todo, @note and  @badcode entries
 * to find out the possible assigments.
 *
 * Ideas other than that should be discussed first :)
 *
 */

#include <QApplication>
#include <QtDebug>
#include <stdio.h>
#include <stdlib.h>

#include "raypumpwindow.h"
#include "application.h"


#if QT_VERSION < 0X050000
void myMessageOutput(QtMsgType type, const char *msg)
#else
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
#endif
{
    QFile outFile("debuglog.txt");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << msg << endl;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("RayPump");
    QCoreApplication::setOrganizationDomain("raypump.com");
    QCoreApplication::setApplicationName("RayPump client");

    Application a(argc, argv);
    if (!a.lock()){
        QMessageBox::critical(0, "Error", "Application is already running");
        exit(1);
    }

    QCoreApplication::addLibraryPath("lib");

#if QT_VERSION > 0X040800
    //qInstallMessageHandler(myMessageOutput);
#endif

    RayPumpWindow w;
    w.show();
    return a.exec();
}
