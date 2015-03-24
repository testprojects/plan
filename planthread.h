#ifndef PLANTHREAD_H
#define PLANTHREAD_H

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

private:
    int VP, KP, NP_Start, NP_End;
    bool SUZ;
    Graph *m_graph;
    QEventLoop *eventLoop;
};

#endif // PLANTHREAD_H
