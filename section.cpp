#include "section.h"
#include "station.h"
#include "mydb.h"

bool Section::operator ==(const Section& s) const
{
    return((stationNumber1 == s.stationNumber1)&&(stationNumber2 == s.stationNumber2));
}

bool Section::operator < (const Section& s) const
{
    if(stationNumber1 < s.stationNumber1) return true;
    else return false;
}

Section::operator QString() const
{
    QString str;
    Station *st1 = MyDB::instance()->stationByNumber(stationNumber1),
            *st2 = MyDB::instance()->stationByNumber(stationNumber2);
    str = QString::fromUtf8("%1 (%2) - %3 (%4)")
            .arg(st1->name)
            .arg(st1->number)
            .arg(st2->name)
            .arg(st2->number);
    return str;
}
