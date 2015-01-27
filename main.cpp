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
    MyDB::instance()->BASE_deleteStreamsFromDB(23, 15, 55);
    MyDB::instance()->cacheIn();

    Graph gr(MyDB::instance()->stations(), MyDB::instance()->sections());
    qDebug() << "elapsed: " << time.elapsed() << "ms";

    Request *r = MyDB::instance()->request(23, 15, 55);
    QMap<int, int> loadAtDays;
    int alternativeStationNumber;
    LoadType load_type;
    switch(r->canLoad(&loadAtDays, &alternativeStationNumber)) {
    case 0: load_type = LOAD_NO; break;
    case 1: load_type = LOAD_STATION; break;
    case 2: load_type = LOAD_PVR; break;
    default: assert(0);
    }

    Stream *s1;
    s1 = MyDB::instance()->stream(r->VP, r->KP, r->NP);
    if(!s1) {
        s1 = gr.planStream(r, true, true);
        if(s1) {
            s1->m_loadType = load_type;
            s1->m_busyLoadingPossibilities = loadAtDays;
            if(!MyDB::instance()->streams().contains(s1))
                MyDB::instance()->addToCache(s1);
            qDebug() << s1->print(true, true, true, true);
        }
    }
    MyDB::instance()->cacheOut();
    return 0;
}



