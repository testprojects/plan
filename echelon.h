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
    echelon() {}
    echelon(int num): number(num) {}
public:

    int number;
    QVector<MyTime> timesArrivalToStations;         //данные о времени въезда на каждую станцию маршрута
    PS ps;                                          //подвижной состав
    QString NA;                                     //наименование и количество перевозимого поездом

    operator QString() const;
};

#endif // ECHELON_H
