#include "station.h"

bool station::operator== (const station &st2) const {
    return (this->number == st2.number);
}

bool station::operator!= (const station &st2) const {
    return (this->number != st2.number);
}
