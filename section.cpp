#include "section.h"

bool section::operator ==(const section& s) const
{
    return((this->stationNumber1 == s.stationNumber1)&&(this->stationNumber2 == s.stationNumber2));
}
