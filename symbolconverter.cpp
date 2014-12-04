#include "symbolconverter.h"
#include <QSqlQuery>
#include "mydb.h"
#include <QDebug>

symbolConverter::symbolConverter()
{
}

void symbolConverter::toRUS()
{
    QString ENG("ABCEHKMOPTX");
    QString RUS("АВСЕНКМОРТХ");

    QString src;//читаем с файла
    QString dest;

    QSqlQuery queryRead(MyDB::instance()->db);
    queryRead.exec("SELECT pn, sk FROM stations");
    qDebug() << "Number of rows: " << queryRead.numRowsAffected();
    while(queryRead.next()) {
        src = queryRead.value(0).toString();
        while(!src.isEmpty()) {
            dest += convertChar(src.left(1));
            src.remove(0, 1);
        }
        dest = dest.toUtf8();
        //записываем dest в БД
        QSqlQuery queryWrite(MyDB::instance()->db);
        QString strQueryWrite = QString("UPDATE stations SET pn = \'") + dest + QString("\' WHERE sk = \'") + queryRead.value(1).toString() + QString("\'");
        queryWrite.exec(strQueryWrite);
        dest = "";
    }
    qDebug() << "src = " << src;
    qDebug() << "dest = " << dest;
}

QString symbolConverter::convertChar(QString src)
{
    int unic = src.unicode()->toLatin1();
    switch(unic)
    {
    case 'A': return QString("А"); break;
    case 'B': return QString("В"); break;
    case 'E': return QString("Е"); break;
    case 'K': return QString("К"); break;
    case 'M': return QString("М"); break;
    case 'H': return QString("Н"); break;
    case 'O': return QString("О"); break;
    case 'P': return QString("Р"); break;
    case 'C': return QString("С"); break;
    case 'T': return QString("Т"); break;
    case 'X': return QString("Х"); break;
    default: return src;
    }

}
