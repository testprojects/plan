#ifndef PLANTHREAD_H
#define PLANTHREAD_H
/*
Отдельный поток для планирования нам нужен для того, чтобы
синхронное (линейное) выполнение процесса планирования
не тормозило клиент-серверное взаимодествие (отправку/приём сообщений)
*/

#include <QThread>
class Graph;
class QEventLoop;

enum ThreadState {RUNNING, PAUSED, ABORTED};

class PlanThread : public QThread
{
    Q_OBJECT
public:
    explicit PlanThread(Graph *gr, int _VP, int _KP, int _NP_Start, int _NP_End, bool _SUZ, QObject *parent = 0);
    void run();
    void pause();
    void abort(bool bSavePlannedThreads = true);
    ThreadState state();
    void setState(ThreadState);

signals:
    void signalPlan(QString);
    void signalPlanFinished();
    void signalOffsetAccepted(bool);
    void signalCacheOut();

    void signalPausePlanning();
    void signalResumePlanning();
    void signalAbortPlanning(bool bSaveChanges);

    void signalPlanPaused();
    void signalPlanResumed();
    void signalPlanAborted();

private:
    Graph *m_graph;
    ThreadState m_state;
    int VP, KP, NP_Start, NP_End;
    bool SUZ;
};

#endif // PLANTHREAD_H
