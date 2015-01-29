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

int main(int argc, char** argv)
{
    QTime time;
    time.start();
    ProgramSettings::instance()->readSettings();
    if(!MyDB::instance()->createConnection("C:\\plan\\docs\\plan.db", "localhost", "artem", "1", "QSQLITE")) {
//    if(!MyDB::instance()->createConnection("postgres", "localhost", "postgres", "postgres", "QPSQL")) {
        qDebug() << "connection failed";
    }
    MyDB::instance()->checkTables();
//    MyDB::instance()->BASE_deleteStreamsFromDB();
    MyDB::instance()->cacheIn();

    Graph gr(MyDB::instance()->stations(), MyDB::instance()->sections());
    qDebug() << "elapsed: " << time.elapsed() << "ms";

    QVector<Request*> requests = MyDB::instance()->requests(24);
    QVector<Request*> failedRequests;
    QVector<Stream*> streams;
    foreach (Request *r, requests) {
        qDebug() << "=======================================================================================================";
        QMap<int, int> loadAtDays;
        int alternativeStationNumber;
        LoadType load_type;
        switch(r->canLoad(&loadAtDays, &alternativeStationNumber)) {
        case 0: load_type = LOAD_NO; break;
        case 1: load_type = LOAD_STATION; break;
        case 2: load_type = LOAD_PVR; break;
        default: assert(0);
        }

        Stream *stream;
        stream = MyDB::instance()->stream(r->VP, r->KP, r->NP);
        if(!stream) {
            stream = gr.planStream(r, true, true);
            qDebug() << "stream adress: " << stream;
            if(stream) {
                stream->m_loadType = load_type;
                stream->m_busyLoadingPossibilities = loadAtDays;
                if(!MyDB::instance()->streams().contains(stream))
                    MyDB::instance()->addToCache(stream);
                streams.append(stream);
            }
            else {
                failedRequests.append(r);
            }
        }
        qDebug() << "=======================================================================================================\n\n";
    }

//    foreach (Stream *s, streams) {
//        qDebug() << s->print(true, true, true, true);
//    }
    qDebug() << "Не спланированные заявки:";
    qDebug() << "-------------------------";
    foreach (Request *r, failedRequests) {
        qDebug() << QString("VP = %1, KP = %2, NP = %3")
                    .arg(r->VP)
                    .arg(r->KP)
                    .arg(r->NP);
    }

    MyDB::instance()->cacheOut();
    return 0;
}
