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

#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

#include "commoncode.h"

class JobManager : public QObject
{
    Q_OBJECT

public:
    explicit JobManager(QTableWidget *tableJobs, QLineEdit *lineEditCounters, QPushButton *pushButtonCancelJobs, QObject *parent = 0);
    ~JobManager() {}
    void setJobs(const QVariantMap &jobs);
    inline QString lastReadyRenderPath() { return m_lastReadyRenderPath; }

signals:
    void requestProgressDisplay(int progress, int total);
    void renderPointsChanged(int renderPoints);
    void requestStatusBarMessage(const QString &message);

private slots:
    void handleSelectionChanged();

private:

    void processProgress(const QByteArray &progress);

    QSet<QString> m_lastSelectedItemsTextList;
    QTableWidget *m_tableJobs;
    QLineEdit *m_lineEditCounters;
    QPushButton *m_pushButtonCancelJob;
    QString m_lastReadyRenderPath;

};


#endif // JOBMANAGER_H
