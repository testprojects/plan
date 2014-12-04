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
//    MyDB::instance()->createTableSections();
//    MyDB::instance()->addSectionsFromFile("C:/plan/docs/UCH.txt");

    MyDB::instance()->readDatabase();

    symbolConverter::toRUS();

    Graph gr;

    if(argc > 0) {
        qDebug() << "Путь до файла: " << argv[0];
        QVector<Request> localRequests = MyDB::instance()->requestsFromFile(argv[0], 1).toVector();
        QList<Stream> localStreams;
        for(int i = 0; i < localRequests.count(); i++) {
            localStreams.append(gr.planStream(&localRequests[i], false, false));
        }
        foreach (Stream s, localStreams) {
            qDebug() << s.print(true, true, true, true);
        }
        qDebug() << "Потоки для внутриокружных перевозок спланированы";
        exit(0);
    }

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
//        if(streams[i].m_failed == true) {
            qDebug() << streams[i].print(false, true, false, true);
//        }
    }
    return 0;
}




