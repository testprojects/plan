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

    MyDB::instance()->createTableStationsBusy();

    Graph gr;
    Request r2 = MyDB::instance()->request(24, 82, 3185);
    if(r2.canLoad()) {
        qDebug() << QString("%1\nПоток может быть погружен");
    }
    else {
        qDebug() << QString("%1\nПоток не может быть погружен");
    }

//    QVector<Request> requests;// = MyDB::instance()->requests(24);
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
//    return 0;
}



