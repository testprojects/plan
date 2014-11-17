#include <QVector>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QTime>

#include "mydb.h"
#include "station.h"
#include "section.h"
#include "stream.h"
#include "filtervertex.h"
#include "filteredge.h"
#include "graph.h"


int main(int argc, char** argv)
{
    QTime time;
    time.start();
    if(!MyDB::instance()->createConnection("postgres", "localhost", "postgres", "postgres")) {
        qDebug() << "connection failed";
    }
    qDebug() << QString::fromUtf8("Время на выполнение функции MyDB::instance->createConection() = %1 мс\n").arg(time.elapsed());

//    MyDB::instance()->createTableRequests();
//    MyDB::instance()->clearRequests();
//    MyDB::instance()->addRequestsFromFile("C:\\plan\\docs\\файл заявок3.txt");
    QMap<int, QString> roadsMap = MyDB::instance()->roads("C:\\plan\\docs\\файл железных дорог.txt");

    time.restart();
    MyDB::instance()->readDatabase();
    qDebug() << QString::fromUtf8("Время на выполнение функции MyDB::instance->readDatabase = %1 мс\n").arg(time.elapsed());

    Graph gr;
    QList<Request> requests;
    requests = MyDB::instance()->requests();

    QList<QList<Request> > roadsRequests;
    QVector<int> roadNumbers;
    int roadsCount = 0;



    foreach (Request r, requests) {
        station st1 = MyDB::instance()->stationByNumber(r.SP);
        if(!roadNumbers.contains(st1.roadNumber))
        {
            ++roadsCount;
            roadNumbers.append(st1.roadNumber);
        }
    }
    qSort(roadNumbers);

    for(int i = 0; i < roadsCount; i++) {
        QList<Request> tmpReqList = MyDB::instance()->requestsBySPRoadNumber(roadNumbers.at(i));
        foreach (Request r, tmpReqList) {
            //если заявка не спланирована
            //или день готовности к отправлению > 10
            //или ЛЮД и ПАСС == 0
            //удаляем заявку
            if((r.PL == 0) || (r.DG > 10) || ((r.ps.luds == 0)&&(r.ps.pass == 0)) || (roadNumbers.at(i) > 20))
            {
                tmpReqList.removeOne(r);
            }
        }

        roadsRequests.append(tmpReqList);
    }

    for(int i = 0; i < roadsRequests.count(); i++) {
        qDebug() << QString::fromUtf8("\t\t\t---------------====%1 ж.д.====----------------")
                    .arg(roadsMap.value(roadNumbers[i]));
        PS psRoad;
        psRoad.luds = 0;
        psRoad.pass = 0;
        for(int j = 0; j < roadsRequests.at(i).count(); j++) {
            Request r = roadsRequests.at(i).at(j);
            psRoad.luds += r.ps.luds;
            psRoad.pass += r.ps.pass;
            station sp = MyDB::instance()->stationByNumber(r.SP);
            station sv = MyDB::instance()->stationByNumber(r.SV);
            qDebug() << QString::fromUtf8("%1, %2, ЛЮД:%3, ПАСС:%4, Время отправления:%5, Эшелоны:%6")
                        .arg(sp.name.leftJustified(22, ' '))
                        .arg(sv.name.leftJustified(22, ' '))
                        .arg(QString::number(r.ps.luds).leftJustified(7, ' '))
                        .arg(QString::number(r.ps.pass).leftJustified(7, ' '))
                        .arg(QString(QString::number(r.DG) + "." + QString::number(r.CG)).leftJustified(QString("xx.xx").count(), ' '))//время
                        .arg(r.NE.join(", "));
        }
        qDebug() << QString::fromUtf8("Общий ПС за дорогу: ЛЮДСКИЕ: %1, ПАССАЖИРСКИЕ: %2")
                    .arg(psRoad.luds)
                    .arg(psRoad.pass);
    }

    qDebug() << QString("Requests count: %1").arg(requests.count());
    for(int i = 0; i < roadsRequests.count(); i++) {
        qDebug() << QString::fromUtf8("В дороге %1: %2 заявок")
                    .arg(roadNumbers.at(i))
                    .arg(roadsRequests.at(i).count());
    }



//    QVector<Stream> streams;
//    for(int i = 0; i < requests.count(); i++) {
//        if(requests[i].canLoad()) {
//            streams.append(gr.planStream(&requests[i], true, true));
//        }
//    }

//    foreach (Stream stream, streams) {
//        qDebug() << stream.print();
//    }
    return 0;
}




