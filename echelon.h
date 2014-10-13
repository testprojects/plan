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
    MyTime timeFinishLoad;                          //время окончания погрузки эшелона
    MyTime timeFinishUnload;                        //время окончания выгрузки эшелона
    Route* parent;                                  //указатель на маршрут (родитель)
    PS ps;                                          //подвижной состав

    echelon();
    echelon(int num, Route* par);
};

#endif // ECHELON_H
