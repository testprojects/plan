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
    ProgramSettings::instance()->readSettings();
    if(!MyDB::instance()->createConnection("C:\\plan\\docs\\plan.db", "localhost", "artem", "1", "QSQLITE")) {
        qDebug() << "connection failed";
    }
    MyDB::instance()->cacheIn();
    Graph gr;

    return 0;
}



