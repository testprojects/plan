#include "station.h"

bool station::operator== (const station &st2) const {
    return (this->number == st2.number);
}

bool station::operator!= (const station &st2) const {
    return (this->number != st2.number);
}

station::operator QString () const
{
    return QString("%1 (%2)")
            .arg(name)
            .arg(number);
}
