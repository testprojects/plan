#include "echelon.h"
#include <QString>
#include <stream.h>

echelon::echelon(){}
echelon::echelon(int num):number(num) {}

QString echelon::getString()
{
    QString str;  
    str += QString::fromUtf8("\n%1 эшелон:\nНаименование и количество перевозимого: %2\n"
                             "время отправления: %3, время прибытия: %4\n")
        .arg(number)
        .arg(NA)
        .arg(timeDeparture)
        .arg(timeArrival);
    int i = 0;
    foreach (MyTime time, timesArrivalToStations) {
        str += QString::fromUtf8("%1")
                .arg(time);
        str += "\n";
        i++;
    }

    return str;
}
