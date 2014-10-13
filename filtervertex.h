#ifndef FILTERVERTEX_H
#define FILTERVERTEX_H
#include <boost/graph/adjacency_list.hpp>
#include "station.h"
#include "section.h"
#include <QVector>
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, station, section> graph_t;
typedef boost::graph_traits<graph_t>::vertex_descriptor v;

struct FilterVertex
{
    FilterVertex() {}
    FilterVertex(graph_t &_g): gr(&_g) {}
    bool operator() (const v &_v) const;


    graph_t *gr;
    QVector<station> m_filteredStations;

    void clearFilter() {m_filteredStations.clear();}
    void addStation(station st);
};

#endif // FILTERVERTEX_H
