#include "filteredge.h"
#include "graph.h"

bool FilterEdge::operator() (const e &_e) const
{
    //задаём условия для вхождения ребра в отфильтрованный граф
    section s = (*gr)[_e];
//    if(m_filteredSections.contains(s))
//        return false;
    return true;
}

void FilterEdge::addSection(section sec)
{
    m_filteredSections.append(sec);
}
