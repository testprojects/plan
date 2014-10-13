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
#include "route.h"
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
    QVector <v> nodes;
    QVector <e> edges;
    FilterVertex filterVertex;
    FilterEdge filterEdge;
    int p;

public:
    Graph();
    //считаем оптимальный маршрут
    Route planStream(Request *r, bool loadingPossibility = true, bool passingPossibility = true);//заявка, учитывать пропускную способность, учитывать погрузочную возможность
    QVector<station> optimalPath(int st1, int st2, bool loadingPossibility, bool passingPossibility);
    int distanceTillStation(int destStationNumber, const QVector<station> &_marshrut);
    int distanceBetweenStations(int source, int destination, QVector<station> _marshrut);//расчитывает расстояние между двумя станциями, принадлежащими рассчитанному маршруту
    e edgeBetweenStations(const station &st1, const station &st2);
    QVector<echelon> fillEchelones(Route *route);

    void clearFilters();
    void addStationToFilter(station st);
    void addSectionToFilter(section sec);
};

#endif // GRAPH_H
