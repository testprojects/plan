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

    Server server;
    return a.exec();
}
#endif
