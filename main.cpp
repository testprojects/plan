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

    MyDB::instance()->createTableRequests();
    MyDB::instance()->clearRequests();
    MyDB::instance()->addRequestsFromFile("C:\\plan\\docs\\файл заявок3.txt");

    time.restart();
    MyDB::instance()->readDatabase();
    qDebug() << QString::fromUtf8("Время на выполнение функции MyDB::instance->readDatabase = %1 мс\n").arg(time.elapsed());

    Graph gr;
    QList<Request> requests;
    requests = MyDB::instance()->requests();
    qDebug() << QString("Requests count: %1").arg(requests.count());

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




