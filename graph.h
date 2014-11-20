#ifndef GRAPH_H
#define GRAPH_H
#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/filtered_graph.hpp>
//#include <boost/graph/graph_utility.hpp>
#include "station.h"
#include "section.h"
#include "request.h"
#include "stream.h"
#include <QVector>
#include "filteredge.h"
#include "filtervertex.h"

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, station, section> graph_t;
typedef boost::graph_traits<graph_t>::vertex_descriptor v;
typedef boost::graph_traits<graph_t>::edge_descriptor e;
typedef boost::filtered_graph <graph_t, FilterEdge, FilterVertex> FilteredGraph;

class Graph
{
public:
    graph_t g;
    QList <v> nodes;
    QList <e> edges;
    FilterVertex filterVertex;
    FilterEdge filterEdge;
    int p;

public:
    Graph();
    //считаем оптимальный маршрут
    Stream planStream(Request *r, bool loadingPossibility = true, bool passingPossibility = true);//заявка, учитывать пропускную способность, учитывать погрузочную возможность
    bool optimalPath(int st1, int st2, QList<station> *passedStations, const QList<section> &fuckedUpSections, bool loadingPossibility, bool passingPossibility);
    bool optimalPathWithOM(int st1, int st2, const QList<int> OM, QList<station> *passedStations, const QList<section> &fuckedUpSections, bool loadingPossibility, bool passingPossibility);
    int distanceTillStation(int stationIndexInPassedStations, const QList<station> &_marshrut);
    int distanceBetweenStations(int sourceIndex, int destinationIndex, QList<station> _marshrut);//расчитывает расстояние между двумя станциями, принадлежащими рассчитанному маршруту
    e edgeBetweenStations(const station &st1, const station &st2);
    QList<station> dijkstraPath(int st1, int st2, const QList<section> &fuckedUpSections, bool loadingPossibility, bool passingPossibility);

    void clearFilters();
    void addStationToFilter(station st);
    void addSectionToFilter(section sec);
};

#endif // GRAPH_H
