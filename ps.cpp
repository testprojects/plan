#include "ps.h"

#include <QStringList>

QString PS::getString()
{
    QString str;
    str += QString::fromUtf8("ПАСС:%1, ЛЮДС:%2, КРЫТ:%3, КУХН:%4, ПЛАТ:%5, ПОЛУ:%6, СПЕЦ:%7 ЛЕДН:%8, ЦИСТ:%9")
            .arg(pass)
            .arg(luds)
            .arg(krit)
            .arg(kuhn)
            .arg(plat)
            .arg(polu)
            .arg(spec)
            .arg(ledn)
            .arg(cist);
    str += QString::fromUtf8(", ВСЕГО:%1")
            .arg(total);

    return str;
}

QStringList PS::getPS()
{
    return QStringList() << QString::number(pass)
                         << QString::number(luds)
                         << QString::number(krit)
                         << QString::number(kuhn)
                         << QString::number(plat)
                         << QString::number(polu)
                         << QString::number(spec)
                         << QString::number(ledn)
                         << QString::number(cist)
                         << QString::number(total);
}
