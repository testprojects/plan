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

    MyDB::instance()->readDatabase();

    Graph gr;
    Request r2 = MyDB::instance()->request(24, 82, 3185);
    QVector<Request> requests;// = MyDB::instance()->requests(24);
    requests.append(r2);
    QList<Stream> streams;
    for (int i = 0; i < requests.count(); i++) {
        //при темпе = 0, занятость участков и станций по погрузку не учитывается
        if(requests[i].TZ == 0) {
            streams.append(gr.planStream(&requests[i], 0, 0));
        }
        else {
            streams.append(gr.planStream(&requests[i], 1, 1));
        }
    }

    for (int i = 0; i < streams.count(); i++) {
        qDebug() << streams[i].print(false, true, false, true);
    }
    return 0;
}




