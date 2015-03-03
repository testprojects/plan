#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include "testplan.h"
#include "programsettings.h"
#include "graph.h"
#include "server.h"
#include "mydb.h"

#include <log4qt/logmanager.h>
#include <log4qt/ttcclayout.h>
#include <log4qt/rollingfileappender.h>
#include <log4qt/consoleappender.h>
#include <log4qt/simplelayout.h>

using namespace Log4Qt;

const QString DB_PATH_WIN = "C:\\plan\\docs\\plan.db";
const QString DB_PATH_APPLE = "/Users/artem/projects/plan/docs/plan.db";

#ifndef TEST
int main(int argc, char** argv)
{
    QCoreApplication a(argc, argv);

    //инициализация корневого журнала
    LogManager::rootLogger();
    LogManager::setHandleQtMessages(true);

    SimpleLayout *simpleLayout = new SimpleLayout();
    simpleLayout->setName(QLatin1String("SimpleLayout"));
    simpleLayout->activateOptions();

    TTCCLayout *ttccLayout = new TTCCLayout(TTCCLayout::ISO8601);
    ttccLayout->setName("TtccLayout");
    ttccLayout->activateOptions();

    ConsoleAppender *consoleAppender = new ConsoleAppender(simpleLayout, ConsoleAppender::STDOUT_TARGET);
    consoleAppender->setName(QLatin1String("ConsoleAppender"));
    consoleAppender->activateOptions();
    LogManager::rootLogger()->addAppender(consoleAppender);

    RollingFileAppender *fileAppender = new RollingFileAppender(ttccLayout, QCoreApplication::applicationName() + ".log", true);
    fileAppender->setName("FileAppender");
    fileAppender->activateOptions();
    fileAppender->setMaximumFileSize(1024 * 1024 * 10);
//    LogManager::logger("FileLogger")->addAppender(fileAppender);
    LogManager::rootLogger()->addAppender(fileAppender);

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
