#include "section.h"

bool section::operator ==(const section& s) const
{
    return((this->stationNumber1 == s.stationNumber1)&&(this->stationNumber2 == s.stationNumber2));
}

bool section::operator < (const section& s) const
{
    if(this->stationNumber1 < s.stationNumber1) return true;
    else return false;
}
