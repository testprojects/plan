#include "section.h"

bool Section::operator ==(const Section& s) const
{
    return((this->stationNumber1 == s.stationNumber1)&&(this->stationNumber2 == s.stationNumber2));
}

bool Section::operator < (const Section& s) const
{
    if(this->stationNumber1 < s.stationNumber1) return true;
    else return false;
}
