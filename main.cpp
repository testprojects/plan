#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include "testplan.h"
#include "programsettings.h"
#include "graph.h"
#include "server.h"
#include "mydb.h"

const QString DB_PATH_WIN = "C:\\plan\\docs\\plan.db";
const QString DB_PATH_APPLE = "/Users/artem/projects/plan/docs/plan.db";

#ifndef TEST
int main(int argc, char** argv)
{
    QCoreApplication a(argc, argv);
    ProgramSettings::instance()->readSettings();
#ifdef _WIN32
    if(!MyDB::instance()->createConnection(DB_PATH_WIN, "localhost", "artem", "1", "QSQLITE"))
#elif __APPLE__
    if(!MyDB::instance()->createConnection(DB_PATH_APPLE, "localhost", "artem", "1", "QSQLITE"))
#endif
    {
        qDebug() << "connection failed";
        return 1;
    }
    MyDB::instance()->checkTables();
    QTime time;
    time.start();
    MyDB::instance()->cacheIn();
    qDebug() << QString("CacheIn() time: %1s").arg(time.elapsed() / 1000.0);

    Graph *gr = new Graph(MyDB::instance()->stations(), MyDB::instance()->sections());
    qDebug() << "elapsed: " << time.elapsed() << "ms";
    Server server;
    return a.exec();
}
#endif
