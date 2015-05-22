#include "planthread.h"
#include "graph.h"
#include "request.h"
#include "../myClient/types.h"
#include "mydb.h"
#include "section.h"
#include "../myClient/types.h"
#include <QDebug>

PlanThread::PlanThread(Graph *gr, int _VP, int _KP, int _NP_Start, int _NP_End, bool _SUZ, QObject *parent) :
    QThread(parent), m_graph(gr), VP(_VP), KP(_KP), NP_Start(_NP_Start), NP_End(_NP_End), SUZ(_SUZ), m_state(PAUSED)
{
    //для передачи сообщения о необходимости смещения планируемой заявки Thread'у (который передаст серверу (который вышлет клиенту))
    connect(m_graph, SIGNAL(signalGraph(QString)), this, SIGNAL(signalPlan(QString)));
    //о смещении
    connect(this, SIGNAL(signalOffsetAccepted(bool)), m_graph, SIGNAL(signalOffsetAccepted(bool)));
}

void PlanThread::run()
{
    m_state = RUNNING;
    QVector<Request*> requests;
#ifndef TEST_MOVE_STREAM
    if(KP == 0)
        requests = MyDB::instance()->requests(VP);
    else
        requests = MyDB::instance()->requests(VP, KP, NP_Start, NP_End);
#else
    Request *req = new Request;
    req->SP = 101072009;    //ЛУГА 1(101072009)
    req->SV = 101050009;    //БОЛОГОЕ-МОСКОВСКОЕ(101050009)
    req->PK = 6;            //6 поездов
    req->TZ = 3;            //с темпом 5 поездов в сутки
    req->DG = 18;           //день готовности - 18
    req->CG = 0;            //час готовности - 0
    req->VP = 23;           //24 вид перевозок
    req->KP = 1;
    req->NP = 1;
    req->KG = 3;            //код груза. 24_GSM
    requests.append(req);

    Section *sec = MyDB::instance()->sectionByNumbers(101072009, 101058302);
    //поток должен сдвинуться
    sec->m_passingPossibilities.insert(16, 2);
    sec->m_passingPossibilities.insert(17, 2);
    sec->m_passingPossibilities.insert(18, 1);
    sec->m_passingPossibilities.insert(19, 1);
    sec->m_passingPossibilities.insert(20, 1);
    sec->m_passingPossibilities.insert(21, 1);
#endif
    QString msg;
    QVector<Request*> failedStreams;
    int iter = 0;
    foreach (Request *req, requests) {
        QMap<int, int> loadAtDays;
        LoadType load_type;
        Stream *stream;
        stream = MyDB::instance()->stream(req->VP, req->KP, req->NP);
        if(!stream) {
            switch(req->canLoad(&loadAtDays)) {
            case 0: load_type = LOAD_NO; break;
            case 1: load_type = LOAD_STATION; break;
            case 2: load_type = LOAD_PVR; break;
            default: assert(0);
            }
            QString errorString;
            if((load_type == LOAD_NO) || (req->VP == 21) || (req->VP == 22) || (!SUZ))
                stream = m_graph->planStream(req, false, false, &errorString);
            else {
                req->load(loadAtDays);
                stream = m_graph->planStream(req, true, true, &errorString);
            }
            if(stream) {
                stream->m_loadType = load_type;
                if(load_type)
                    stream->m_busyLoadingPossibilities = loadAtDays;
                MyDB::instance()->addToCache(stream);
            }
            else {
                req->errorString = errorString;
                QString strFailStream = QString::fromUtf8("STREAM_PLAN_FAILED(%1)")
                        .arg(errorString);
                emit signalPlan(strFailStream);
                failedStreams.append(req);
            }
        }
        iter++;
        msg = QString("STREAM_PLANNED(%1/%2)").arg(iter).arg(requests.count());
        emit signalPlan(msg);
    }

    if(!failedStreams.isEmpty()) {
        msg = QString("FAILED_STREAMS(%1)").arg(failedStreams.count());
        emit signalPlan(msg);
    }
    msg = "PLAN_FINISHED";
    emit signalPlan(msg);
    MyDB::instance()->cacheOut();
    emit signalPlanFinished();
}

void PlanThread::pause() {
    qDebug() << "pausing planThread";
    setState(PAUSED);
}

void PlanThread::resume() {
    qDebug() << "resuming planThread";
    setState(RUNNING);
}

void PlanThread::abort(bool bSavePlannedThreads) {
    qDebug() << "aborting planThread";
    setState(ABORTED);
}

ThreadState PlanThread::state() {
    return m_state;
}

void PlanThread::setState(ThreadState _state) {
    m_state = _state;
}
