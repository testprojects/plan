#include <QVector>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QTime>
#include <QSettings>

#include "mydb.h"
#include "station.h"
#include "section.h"
#include "stream.h"
#include "filtervertex.h"
#include "filteredge.h"
#include "graph.h"
#include "programsettings.h"
#include "testplan.h"

int main(int argc, char** argv)
{
    ProgramSettings::instance()->readSettings();
    if(!MyDB::instance()->createConnection("C:\\plan\\docs\\plan.db", "localhost", "artem", "1", "QSQLITE")) {
//    if(!MyDB::instance()->createConnection("postgres", "localhost", "postgres", "postgres", "QPSQL")) {
        qDebug() << "connection failed";
    }
    MyDB::instance()->checkTables();
    MyDB::instance()->BASE_deleteStreamsFromDB();
    QTime time;
    time.start();
    MyDB::instance()->cacheIn();
    qDebug() << QString("CacheIn() time: %1s").arg(time.elapsed() / 1000.0);

    Graph gr(MyDB::instance()->stations(), MyDB::instance()->sections());
    qDebug() << "elapsed: " << time.elapsed() << "ms";

    QVector<Request*> requests = MyDB::instance()->requests();
    QVector<Request*> failedRequests;
    QVector<Stream*> streams;
    foreach (Request *r, requests) {
        qDebug() << "=======================================================================================================";
        QMap<int, int> loadAtDays;
        LoadType load_type;
        switch(r->canLoad(&loadAtDays)) {
        case 0: load_type = LOAD_NO; break;
        case 1: load_type = LOAD_STATION; break;
        case 2: load_type = LOAD_PVR; break;
        default: assert(0);
        }

        Stream *stream;
        stream = MyDB::instance()->stream(r->VP, r->KP, r->NP);
        if(!stream) {
            if((load_type == LOAD_NO) || (r->VP == 21) || (r->VP == 22))
                stream = gr.planStream(r, false, false);
            else {
                r->load(loadAtDays);
                stream = gr.planStream(r, true, true);
            }
            qDebug() << "stream adress: " << stream;
            if(stream) {
                stream->m_loadType = load_type;
                stream->m_busyLoadingPossibilities = loadAtDays;
                MyDB::instance()->addToCache(stream);
                streams.append(stream);
            }
            else {
                failedRequests.append(r);
            }
        }
        qDebug() << "=======================================================================================================\n\n";
    }

    qDebug() << "Не спланированные заявки:";
    qDebug() << "-------------------------";
    foreach (Request *r, failedRequests) {
        qDebug() << QString("VP = %1, KP = %2, NP = %3")
                    .arg(r->VP)
                    .arg(r->KP)
                    .arg(r->NP);
    }

    qDebug() << QString("Заявок: %1\nСпланировано: %2\nНе спланировано: %3")
                .arg(requests.count())
                .arg(streams.count())
                .arg(failedRequests.count());
    MyDB::instance()->cacheOut();
    return 0;
}
