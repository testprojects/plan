#include "station.h"
#include <QDataStream>

bool Station::operator== (const Station &st2) const {
    return (this->number == st2.number);
}

bool Station::operator!= (const Station &st2) const {
    return (this->number != st2.number);
}

Station::operator QString () const
{
    return QString("%1 (%2)")
            .arg(name)
            .arg(number);
}
