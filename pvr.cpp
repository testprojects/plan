#include "pvr.h"
#include <assert.h>

PVR::operator QString() const
{
    assert(this);
    QString str;
    QString strPV;
    for(int i = 0; i < 60; i++)
        strPV += QString("%1:[%2/%3] ")
                .arg(i+1)
                .arg(ps - loadingPossibilities23[i])
                .arg(ps);
    str += QString::fromUtf8("ПВР №%1 - %2 (ПС:%3) (ПВ:%4)")
            .arg(number)
            .arg(name)
            .arg(ps)
            .arg(strPV);
    return str;
}
