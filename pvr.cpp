#include "pvr.h"

pvr::operator QString() const
{
    QString str;
    QString strPV;
    for(int i = 0; i < 60; i++)
        strPV += QString("%1/%2")
                .arg(i+1)
                .arg(pv[i]);
    str += QString::fromUtf8("ПВР №%1 - %2 (ПС:%3) (ПВ:%4)")
            .arg(number)
            .arg(name)
            .arg(ps)
            .arg(strPV);
}
