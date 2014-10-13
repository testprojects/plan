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
    case 'A': return QString("А");
    case 'B': return QString("В");
    case 'C': return QString("С");
    case 'E': return QString("Е");
    case 'H': return QString("Н");
    case 'K': return QString("К");
    case 'M': return QString("М");
    case 'O': return QString("О");
    case 'P': return QString("Р");
    case 'T': return QString("Т");
    case 'X': return QString("Х");
    default: return src;
    }

}
