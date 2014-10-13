#ifndef FILTEREDGE_H
#define FILTEREDGE_H
#include <boost/graph/adjacency_list.hpp>
#include "section.h"
#include "station.h"
#include <QVector>

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, station, section> graph_t;
typedef boost::graph_traits<graph_t>::edge_descriptor e;



struct FilterEdge
{
    FilterEdge() {}
    FilterEdge(graph_t &_g): gr(&_g) {}
    bool operator() (const e &_e) const;

    QVector<section> m_filteredSections;
    graph_t *gr;

    void clearFilter() {m_filteredSections.clear();}
    void addSection(section sec);
};

#endif // FILTEREDGE_H
