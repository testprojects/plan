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
    if(!MyDB::instance()->createConnection("postgres", "localhost", "postgres", "postgres")) {
        qDebug() << "connection failed";
    }
//    MyDB::instance()->addSectionsFromFile("C:/plan/docs/UCH.txt");
    MyDB::instance()->readDatabase();
    Graph gr;

    Request r = MyDB::instance()->request(24, 82, 2001);
    QVector<Request> requests;// = MyDB::instance()->requests(24);
    requests.append(r);
    QList<Stream> streams;
    for (int i = 0; i < requests.count(); i++) {
        streams.append(gr.planStream(&requests[i], 1, 1));
    }

    for (int i = 0; i < streams.count(); i++) {
        qDebug() << streams[i].print();
    }
    return 0;
}




