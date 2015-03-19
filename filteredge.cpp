#include "filteredge.h"
#include "graph.h"
#include "mydb.h"
#include <QDebug>

bool FilterEdge::operator() (const e &_e) const
{
    //задаём условия для вхождения ребра в отфильтрованный граф
    Section *sec = MyDB::instance()->sectionByNumbers((*gr)[_e].stationNumber1, (*gr)[_e].stationNumber2);
    Section *reverseSec = MyDB::instance()->sectionByNumbers((*gr)[_e].stationNumber2, (*gr)[_e].stationNumber1);
    assert(sec);
    assert(reverseSec);
    if(m_filteredSections.contains(sec) || m_filteredSections.contains(reverseSec))
        return false;
    return true;
}

void FilterEdge::addSection(Section *sec)
{
    if(!m_filteredSections.contains(sec))
        m_filteredSections.append(sec);
}
