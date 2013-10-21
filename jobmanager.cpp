#include "jobmanager.h"

JobManager::JobManager(QTableWidget *tableJobs, QLineEdit *lineEditCounters, QPushButton *pushButtonCancelJobs, QObject *parent) :
    QObject(parent),
    m_tableJobs(tableJobs),
    m_lineEditCounters(lineEditCounters),
    m_pushButtonCancelJob(pushButtonCancelJobs)
{

    m_tableJobs->setHorizontalHeaderLabels(QStringList() << tr("Job name") << tr("State/Cost") << tr("Type"));
    m_tableJobs->setColumnWidth(0, 250);
    m_tableJobs->setColumnWidth(1, 80);
    m_tableJobs->setColumnWidth(2, 80);
    m_tableJobs->sortItems(0);
    connect(m_tableJobs, SIGNAL(itemSelectionChanged()), SLOT(handleSelectionChanged()));
    m_lastSelectedItemsTextList.clear();
}

void JobManager::setJobs(const QVariantMap &jobs)
{
    m_tableJobs->clearContents();
    m_tableJobs->setRowCount(0);

    bool sorting = m_tableJobs->isSortingEnabled();
    m_tableJobs->setSortingEnabled(false);

    bool running = false;
    QMapIterator<QString, QVariant> i(jobs);
    while(i.hasNext()){
        i.next();

        if (i.key() == "counters"){
            m_lineEditCounters->setText(i.value().toString());
            continue;
        }

        if (i.key() == "render_points"){
            emit renderPointsChanged(i.value().toInt());
            continue;
        }

        int rowCount = m_tableJobs->rowCount();

        QVariantMap data = i.value().toMap();
        QString type = data.value("type").toString();
        QString state = data.value("state").toString();
        QByteArray progress = data.value("progress").toByteArray();
        QString path = data.value("job_path").toString();

        if (state.isEmpty()){
            uERROR << "'state' value not found";
            continue;
        }
        if (type.isEmpty()){
            uERROR << "'type' value not found";
            continue;
        }

        m_tableJobs->insertRow(rowCount);

        QTableWidgetItem * item = new QTableWidgetItem(i.key());
        item->setData(IR_JOB_PATH, path);
        m_tableJobs->setItem(rowCount, 0, item);

        QTableWidgetItem *itemState = new QTableWidgetItem(state);
        m_tableJobs->setItem(rowCount, 1, itemState);

        QTableWidgetItem *itemType = new QTableWidgetItem(type);
        m_tableJobs->setItem(rowCount, 2, itemType);


        if (state == "running"){
            running = true;
            itemState->setForeground(QColor(Qt::darkGreen));
            processProgress(progress);
        }
        else if (state == "testing"){
            itemState->setForeground(QColor(Qt::darkBlue));
        }
        else if (state == "done"){
            itemState->setForeground(QColor(Qt::lightGray));
            int cost = data.value("cost").toInt();
            itemState->setText(QString("done (%1RP)").arg(cost));
            if (itemType->text() == "stress-test"){
                emit requestStatusBarMessage(QString("Est. cost: %1RP").arg(cost));
            }
        }
    }

    if (!running){
        processProgress("0/0");
    }

    QAbstractItemView::SelectionMode mode = m_tableJobs->selectionMode();
    m_tableJobs->setSelectionMode(QAbstractItemView::MultiSelection);
    m_tableJobs->blockSignals(true);
    {
        foreach (QString itemText, m_lastSelectedItemsTextList){
            QList<QTableWidgetItem*> foundItems = m_tableJobs->findItems(itemText, Qt::MatchFixedString);
            if (!foundItems.isEmpty()){
                m_tableJobs->selectRow(foundItems.first()->row());
            }
        }
    }
    m_tableJobs->blockSignals(false);
    m_tableJobs->setSelectionMode(mode);

    m_pushButtonCancelJob->setEnabled(!m_tableJobs->selectedItems().isEmpty());

    m_tableJobs->setSortingEnabled(sorting);
}

int JobManager::uploadLimit()
{
    /// @todo missing code
    return 0;
}

void JobManager::processProgress(const QByteArray &progress)
{
    if (progress.isEmpty()){
        uERROR << "empty progress data";
        return;
    }

    QList<QByteArray> progressList(progress.split('/'));
    if (progressList.count() < 2){
        uERROR << "progress list data malformed" << progressList;
        return;
    }

    emit requestProgressDisplay(progressList.first().toInt(), progressList.last().toInt());
}


void JobManager::handleSelectionChanged()
{
    QList<QTableWidgetItem*> items = m_tableJobs->selectedItems();
    if (!items.isEmpty()){
        m_lastSelectedItemsTextList.clear();
        foreach(QTableWidgetItem *item, items){
            m_lastSelectedItemsTextList.insert(m_tableJobs->item(item->row(), 0)->text());
        }
    }
    uINFO << m_lastSelectedItemsTextList;
    m_pushButtonCancelJob->setEnabled(!m_tableJobs->selectedItems().isEmpty());
}

