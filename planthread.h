#ifndef PLANTHREAD_H
#define PLANTHREAD_H

#include <QThread>
class Graph;

class PlanThread : public QThread
{
    Q_OBJECT
public:
    explicit PlanThread(Graph *gr, int _VP, int _KP, int _NP_Start, int _NP_End, bool _SUZ, QObject *parent = 0);
    void run();

signals:
    void signalPlanStarted(int requestsAmount);
    void signalPlanFinished();
    void signalStreamPlanned(int number);
    void signalStreamFailed(int count);
    void signalOffsetStream(int number, int hours);

private:
    int VP, KP, NP_Start, NP_End;
    bool SUZ;
    Graph *m_graph;
};

#endif // PLANTHREAD_H
