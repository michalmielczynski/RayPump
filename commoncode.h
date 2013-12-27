/* Copyright 2013 michal.mielczynski@gmail.com. All rights reserved.
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

#ifndef COMMONCODE_H
#define COMMONCODE_H

#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QHostAddress>
#include <QDir>
#include <QApplication>

#define uINFO qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "info: (" << __FUNCTION__ << ")"
#define uWARNING qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "WARNING: (" << __FUNCTION__ << ")"
#define uERROR qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "ERROR: (" << __FUNCTION__ << ")"

const static qreal G_VERSION = 0.994f;
const static qreal G_ALLOWED_ADDON_VERSION = 0.993f;
const static QString G_VERSION_NAME =  "Winter White";

// global variables
struct Globals{
    static bool SERVER_VIRTUALIZED;
    static QString SERVER_HOST_NAME;
    static QString SERVER_IP;
    static QString BUFFER_DIRECTORY;
    static QString RENDERS_DIRECTORY;

};

enum ItemRole {
    IR_JOB_PATH = Qt::UserRole + 1
};

#endif // COMMONCODE_H

