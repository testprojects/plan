#ifndef STATIONBUSY_H
#define STATIONBUSY_H
#include "mytime.h"
#include <QMap>

class StationBusy
{
public:
    int stationNumber;
    int KG;
    int VP;
    int KP;
    int NP;
    QMap<int, int> loadDays;
public:
    StationBusy();
};

#endif // STATIONBUSY_H
