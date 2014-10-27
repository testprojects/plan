#include "ps.h"

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
    str += QString::fromUtf8("ВСЕГО:%1")
            .arg(total);

    return str;
}

//void PS::operator =(const PS &psSrc)
//{
//    this->cist = psSrc.cist;
//    this->krit = psSrc.krit;

//}
