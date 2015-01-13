#ifndef PVRBUSY_H
#define PVRBUSY_H
#include "mytime.h"
#include <QMap>

class PvrBusy
{
public:
    int pvrNumber;
    int VP;
    int KP;
    int NP;
    QMap<int, int> loadDays;
public:
    PvrBusy();
};

#endif // PVRBUSY_H
