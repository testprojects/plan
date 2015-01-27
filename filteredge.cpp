#include "filteredge.h"
#include "graph.h"
#include "mydb.h"
#include <QDebug>

bool FilterEdge::operator() (const e &_e) const
{
    //задаём условия для вхождения ребра в отфильтрованный граф
    Section *sec = MyDB::instance()->sectionByNumbers((*gr)[_e].stationNumber1, (*gr)[_e].stationNumber2);
    assert(sec);
    if(m_filteredSections.contains(sec))
        return false;
    return true;
}

void FilterEdge::addSection(Section *sec)
{
    if(!m_filteredSections.contains(sec))
        m_filteredSections.append(sec);
}
