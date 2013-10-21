#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

#include <commoncode.h>

class JobManager : public QObject
{
    Q_OBJECT

public:
    explicit JobManager(QTableWidget *tableJobs, QLineEdit *lineEditCounters, QPushButton *pushButtonCancelJobs, QObject *parent = 0);
    ~JobManager() {}
    void setJobs(const QVariantMap &jobs);
    int uploadLimit();

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

};


#endif // JOBMANAGER_H
