#include "section.h"

bool Section::operator ==(const Section& s) const
{
    return((stationNumber1 == s.stationNumber1)&&(stationNumber2 == s.stationNumber2));
}

bool Section::operator < (const Section& s) const
{
    if(stationNumber1 < s.stationNumber1) return true;
    else return false;
}
