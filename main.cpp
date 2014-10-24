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

    time.restart();
    MyDB::instance()->readDatabase();
    qDebug() << QString::fromUtf8("Время на выполнение функции MyDB::instance->readDatabase = %1 мс\n").arg(time.elapsed());


    Graph gr;

    Request req;
    req = MyDB::instance()->request(23, 10, 102);
    qDebug() << req.getString();

//    req.OM.clear();
//    req.SP = 101639608;
//    req.SV = 101704506;
//    req.OM.append(101666906);
//    req.OM.append(101660100);
//    req.OM.append(101671209);
//    req.OM.append(101721003);
//    req.OM.append(101707307);
//    req.OM.append(101710009);
//    req.OM.append(101711406);

    Stream rou;
    if(req.canLoad()) {
        rou = gr.planStream(&req, 1, 1);
    }

    qDebug() << rou.print();

//    QVector<Request> requests;
//    requests.append(MyDB::instance()->request(23, 10, 101));
//    requests.append(MyDB::instance()->request(23, 10, 102));
//    requests.append(MyDB::instance()->request(23, 10, 103));
//    requests.append(MyDB::instance()->request(23, 10, 104));
//    requests.append(MyDB::instance()->request(23, 10, 105));

//    QVector<Route> routes;
//    for(int i = 0; i < requests.count(); i++) {
//        if(requests[i].canLoad()) {
//            routes.append(gr.planStream(&requests[i], true, true));
//        }
//    }

//    foreach (Route rou, routes) {
//        qDebug() << rou.print();
//    }
    return 0;
}




