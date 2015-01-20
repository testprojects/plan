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
    ProgramSettings::instance()->writeSettings();
    ProgramSettings::instance()->readSettings();
    if(!MyDB::instance()->createConnection("C:\\plan\\docs\\plan.db", "localhost", "artem", "1", "QSQLITE")) {
        qDebug() << "connection failed";
    }
    MyDB::instance()->cacheIn();
    Graph gr;
    MyDB::instance()->DB_createTableStationLoad();
    MyDB::instance()->DB_createTablePVRLoad();
    MyDB::instance()->DB_createTableEchelones();
    MyDB::instance()->DB_cropTableStationLoad();
    MyDB::instance()->DB_cropTablePVRLoad();
    MyDB::instance()->DB_cropTableEchelones();

    QVector<Request> requests;
    QVector<Stream> streams;
    requests = MyDB::instance()->requests(23, 15);

    for(int i = 0; i < requests.count(); i++) {
        QMap<int, int> loadAtDays;
        int alternativeStationNumber = 0;
        Station sp = MyDB::instance()->stationByNumber(requests[i].SP);
        Stream s;
        switch (requests[i].canLoad(&loadAtDays, &alternativeStationNumber)) {
        case 0:
            qDebug() << "Заявка не погружена:\n" << requests[i];
            break;
        case 1:
            MyDB::instance()->loadRequestAtStation(requests[i].SP, requests[i].KG, requests[i].VP, requests[i].KP, requests[i].NP, loadAtDays);
            s = gr.planStream(&requests[i], false, false);
            s.m_busyLoadingPossibilities = loadAtDays;
            streams.append(s);
            break;
        case 2:
            if(alternativeStationNumber != 0)
                MyDB::instance()->loadRequestAtStation(alternativeStationNumber, requests[i].KG, requests[i].VP, requests[i].KP, requests[i].NP, loadAtDays);
            else
                MyDB::instance()->loadRequestAtStation(requests[i].SP, requests[i].KG, requests[i].VP, requests[i].KP, requests[i].NP, loadAtDays);
            MyDB::instance()->loadRequestAtPVR(sp.pvrNumber, requests[i].KG, requests[i].VP, requests[i].KP, requests[i].NP, loadAtDays);
            s = gr.planStream(&requests[i], false, false);
            s.m_busyLoadingPossibilities = loadAtDays;
            streams.append(s);
            break;
        }
    }

    foreach (Stream s, streams) {
        qDebug() << s.print(true, true, true, true);
    }

    return 0;
}



