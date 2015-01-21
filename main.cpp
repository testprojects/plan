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
    QTime time;
    time.start();
    ProgramSettings::instance()->readSettings();
    if(!MyDB::instance()->createConnection("C:\\plan\\docs\\plan.db", "localhost", "artem", "1", "QSQLITE")) {
        qDebug() << "connection failed";
    }
    MyDB::instance()->checkTables();
    MyDB::instance()->cacheIn();
    Graph gr(MyDB::instance()->stations(), MyDB::instance()->sections());
    qDebug() << "elapsed: " << time.elapsed() << "ms";
    return 0;
}



