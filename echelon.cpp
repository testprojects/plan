#include "echelon.h"
#include <QString>

echelon::echelon() {}
echelon::echelon(int num): number(num)
{
}

QString echelon::getString()
{
    QString str;
    QString strTimes;
    for(int i = 0; i < timesArrivalToStations.count(); i++) {
        strTimes += QString::fromUtf8("%1ст:%2").arg(i+1).arg(timesArrivalToStations[i]);
    }
    str += QString::fromUtf8("№ эшелона:%1, Время прибытия на станции: %2\n");
    return str;
}
