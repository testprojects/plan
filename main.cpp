#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QtCore/QTextCodec>
#include "testplan.h"
#include "programsettings.h"
#include "graph.h"
#include "server.h"
#include "mydb.h"

#include <log4qt/src/logmanager.h>
#include <log4qt/src/ttcclayout.h>
#include <log4qt/src/rollingfileappender.h>
#include <log4qt/src/consoleappender.h>
#include <log4qt/src/simplelayout.h>

using namespace Log4Qt;

const QString DB_PATH_WIN = "D:/qt_projects/plan/docs/plan_full.db";
const QString DB_PATH_APPLE = "/Users/artem/projects/plan/docs/plan_full.db";

#ifndef TEST
int main(int argc, char** argv)
{
    QCoreApplication a(argc, argv);

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

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

    ProgramSettings::instance()->writeSettings();
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
