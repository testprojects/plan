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

    MyDB::instance()->createTablePVRLoad();
    MyDB::instance()->createTableStationLoad();
    MyDB::instance()->cropTableStationLoad();
    MyDB::instance()->cropTablePVRLoad();

    Graph gr;
    QVector<Request> requests = MyDB::instance()->requests();
//    Request r2 = MyDB::instance()->request(23, 10, 300);
//    qDebug() << r2;
    foreach (Request r, requests) {
        QMap<int, int> loadAtDays;
        if(r.canLoad(&loadAtDays)) {
//            qDebug() << QString("\nПоток может быть погружен");
            r.load(loadAtDays);
        }
        else {
            qDebug() << r;
//            qDebug() << QString("\nПоток не может быть погружен");
        }
    }

//    requests.append(r2);
//    QList<Stream> streams;
//    for (int i = 0; i < requests.count(); i++) {
//        //при темпе = 0, занятость участков и станций по погрузку не учитывается
//        if(requests[i].TZ == 0) {
//            streams.append(gr.planStream(&requests[i], 0, 0));
//        }
//        else {
//            streams.append(gr.planStream(&requests[i], 1, 1));
//        }
//    }

//    for (int i = 0; i < streams.count(); i++) {
//        qDebug() << streams[i].print(false, true, false, true);
//    }
    return 0;
}



