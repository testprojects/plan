#ifndef FILTERVERTEX_H
#define FILTERVERTEX_H

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

#include "station.h"
#include "section.h"
#include <QVector>
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, Station, Section> graph_t;
typedef boost::graph_traits<graph_t>::vertex_descriptor v;

struct FilterVertex
{
    FilterVertex() {}
    FilterVertex(graph_t *_g): gr(_g) {}
    bool operator() (const v &_v) const;


    graph_t *gr;
    QVector<Station*> m_filteredStations;

    void clearFilter() {m_filteredStations.clear();}
    void addStation(Station *st);
};

#endif // FILTERVERTEX_H
