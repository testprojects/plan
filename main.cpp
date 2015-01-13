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
    if(!MyDB::instance()->createConnection("C:\\plan\\docs\\plan.db", "localhost", "artem", "1")) {
        qDebug() << "connection failed";
    }

    QTime time;
    time.start();
    MyDB::instance()->readDatabase();
    qDebug() << "readDatabase: " << time.elapsed() << " ms";

    MyDB::instance()->cropTableStationLoad();
    MyDB::instance()->cropTablePVRLoad();

    QVector<Request> requests;
    requests.append(MyDB::instance()->request(23, 15, 33));

    foreach (Request r, requests) {
        QMap<int, int> loadAtDays;
        int alternativeStationNumber = 0;
        station sp = MyDB::instance()->stationByNumber(r.SP);

        switch (r.canLoad(&loadAtDays, &alternativeStationNumber)) {
        case 0:
            qDebug() << "Заявка не погружена:\n" <<r;
            break;
        case 1:
            MyDB::instance()->loadRequestAtStation(r.SP, r.KG, r.VP, r.KP, r.NP, loadAtDays);
            break;
        case 2:
            MyDB::instance()->loadRequestAtStation(r.SP, r.KG, r.VP, r.KP, r.NP, loadAtDays);
            MyDB::instance()->loadRequestAtPVR(sp.pvrNumber, r.KG, r.VP, r.KP, r.NP, loadAtDays);
            break;
        }
    }

    return 0;
}



