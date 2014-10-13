#include "filtervertex.h"
bool FilterVertex::operator() (const v &_v) const
{
        //задаём условия для вхождения вершины в отфильтрованный граф
        if((*gr)[_v].name == "ЯНИСЬЯРВИ")
            return false;
        return true;
}

void FilterVertex::addStation(station st)
{
    m_filteredStations.append(st);
}
