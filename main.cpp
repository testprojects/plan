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
    qDebug() << QString::fromUtf8("MyDB::instance->createConection() = %1 мс\n").arg(time.elapsed());
    time.restart();
    MyDB::instance()->readDatabase();
    qDebug() << QString::fromUtf8("MyDB::instance->readDatabase = %1 мс\n").arg(time.elapsed());

    Graph gr;

    QVector<Request> requests = MyDB::instance()->requests();
    QList<Stream> streams;
    for (int i = 0; i < requests.count(); i++) {
        streams.append(gr.planStream(&requests[i], 1, 1));
    }

    for (int i = 0; i < streams.count(); i++) {
        qDebug() << streams[i].print();
    }
    return 0;
}




