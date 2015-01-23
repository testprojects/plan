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
    qDebug() << "FilterEdge::addSection() section adress: " << sec << " N1 = " << sec->stationNumber1 << " N2 = " << sec->stationNumber2;
    if(!m_filteredSections.contains(sec))
        m_filteredSections.append(sec);
}
