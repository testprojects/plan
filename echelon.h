#ifndef ECHELON_H
#define ECHELON_H
#include <QVector>
#include <QTime>
#include "station.h"
#include "ps.h"
#include "mytime.h"

class Route;

class echelon
{
public:
    int number;
    QVector<MyTime> timesArrivalToStations;         //данные о времени въезда на каждую станцию маршрута
    MyTime timeDeparture;                           //время окончания погрузки эшелона
    MyTime timeArrival;                             //время окончания выгрузки эшелона
    PS ps;                                          //подвижной состав

    echelon();
    echelon(int num);

    QString getString();
};

#endif // ECHELON_H
