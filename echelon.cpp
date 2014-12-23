#include "echelon.h"
#include <QString>
#include <stream.h>

echelon::echelon(Stream *parent):m_stream(parent) {}
echelon::echelon(Stream *parent, int num):m_stream(parent), number(num) {}

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
        station st = m_stream->m_passedStations.at(i);
        str += QString::fromUtf8("%1 - %2")
                .arg(time)
                .arg(st);
        str += "\n";
        i++;
    }

    return str;
}
