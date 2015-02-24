#ifndef FILTEREDGE_H
#define FILTEREDGE_H

#if defined(__MINGW32__) || defined (__MINGW64__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-local-typedefs"
    #include <boost/graph/adjacency_list.hpp>
    #pragma GCC diagnostic pop
#elif defined (__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
    #include <boost/graph/adjacency_list.hpp>
    #pragma clang diagnostic pop
#endif

#include "section.h"
#include "station.h"
#include <QVector>


typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, Station, Section> graph_t;
typedef boost::graph_traits<graph_t>::edge_descriptor e;



struct FilterEdge
{
    FilterEdge() {}
    FilterEdge(graph_t *_g): gr(_g) {}
    bool operator() (const e &_e) const;

    QVector<Section*> m_filteredSections;
    graph_t *gr;

    void clearFilter() {m_filteredSections.clear();}
    void addSection(Section *sec);
};

#endif // FILTEREDGE_H
