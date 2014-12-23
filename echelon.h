#ifndef ECHELON_H
#define ECHELON_H
#include <QVector>
#include <QTime>
#include "station.h"
#include "ps.h"
#include "mytime.h"

class Stream;

class echelon
{
public:
    int number;
    QList<MyTime> timesArrivalToStations;         //данные о времени въезда на каждую станцию маршрута
    MyTime timeDeparture;                           //время окончания погрузки эшелона
    MyTime timeArrival;                             //время окончания выгрузки эшелона
    PS ps;                                          //подвижной состав
    QString NA;                                     //наименование и количество перевозимого поездом
    Stream *m_stream;

    echelon(Stream *parent);
    echelon(Stream *parent, int num);

    QString getString();
};

#endif // ECHELON_H
