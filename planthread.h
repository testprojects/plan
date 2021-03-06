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

class PlanThread : public QThread
{
    Q_OBJECT
public:
    explicit PlanThread(Graph *gr, int _VP, int _KP, int _NP_Start, int _NP_End, bool _SUZ, QObject *parent = 0);
    void run();

signals:
    void signalPlan(QString);
    void signalPlanFinished();
    void signalOffsetAccepted(bool);

private:
    Graph *m_graph;
    int VP, KP, NP_Start, NP_End;
    bool SUZ;
};

#endif // PLANTHREAD_H
