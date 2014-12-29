#ifndef STATIONBUSY_H
#define STATIONBUSY_H
#include "mytime.h"

class stationBusy
{
public:
    int stationNumber;
    int KG;
    MyTime startLoadFirstTrain;
    MyTime finishLoadLastTrain;
public:
    stationBusy();
};

#endif // STATIONBUSY_H
