#ifndef PVR_H
#define PVR_H
#include <QString>

struct pvr
{
    int number;
    QString name;
    int ps;//погрузочная способность
    int pv[60];//погрузочная возможность на 60 дней
};

#endif // PVR_H
