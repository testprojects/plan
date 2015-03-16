#ifndef GRAPH_H
#define GRAPH_H

#if defined(__MINGW32__) || defined (__MINGW64__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-local-typedefs"
    #include <boost/config.hpp>
    #include <boost/graph/graph_traits.hpp>
    #include <boost/graph/adjacency_list.hpp>
    #include <boost/graph/dijkstra_shortest_paths.hpp>
    #include <boost/property_map/property_map.hpp>
    #include <boost/graph/filtered_graph.hpp>
    #pragma GCC diagnostic pop
#elif defined (__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
    #include <boost/config.hpp>
    #include <boost/graph/graph_traits.hpp>
    #include <boost/graph/adjacency_list.hpp>
    #include <boost/graph/dijkstra_shortest_paths.hpp>
    #include <boost/property_map/property_map.hpp>
    #include <boost/graph/filtered_graph.hpp>
#endif

#include "station.h"
#include "section.h"
#include "request.h"
#include "stream.h"
#include <QVector>
#include "filteredge.h"
#include "filtervertex.h"
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, Station, Section> graph_t;
typedef boost::graph_traits<graph_t>::vertex_descriptor v;
typedef boost::graph_traits<graph_t>::edge_descriptor e;
typedef boost::filtered_graph <graph_t, FilterEdge, FilterVertex> FilteredGraph;
class Server;


class Graph: public QObject
{
    Q_OBJECT
public:
    //конструктор с аргументами. создание пустого графа не предусматривается
    explicit Graph(const QVector<Station *> &stationList, const QVector<Section *> &sectionList, Server *server);
    ~Graph();
public:
    graph_t g;
    QList <v> nodes;
    QList <e> edges;
    FilterVertex *filterVertex;
    FilterEdge *filterEdge;
    Server *m_server;

    //планирование потока
    Stream *planStream(Request *r, bool loadingPossibility = true, bool passingPossibility = true);
    //рассчёт оптимального маршрута между двумя станциями
    bool optimalPath(int st1, int st2, QVector<Station*> *passedStations
                     , QVector<Section*> fuckedUpSections = QVector<Section*>(), bool loadingPossibility = false, bool passingPossibility = false);
    //рассчёт потимального маршрута с учётом наличия обязательных станций проследования
    bool optimalPathWithOM(int st1, int st2, QList<int> OM, QVector<Station*> *passedStations
                           , QVector<Section *> fuckedUpSections = QVector<Section*>(), bool loadingPossibility = false, bool passingPossibility = false);
    //расстояние до станции по заданному маршруту от 0-ой станции в списке
    static int distanceTillStation(int stationIndexInPassedStations, const QVector<Station *> &_marshrut);
    //расстояние между двумя станциями в маршруте
    static int distanceBetweenStations(int sourceIndex, int destinationIndex, QVector<Station *> _marshrut);
    //рассчёт оптимального маршрута между двумя ОПОРНЫМИ станциями
    QVector<Station*> dijkstraPath(int st1, int st2, const QVector<Section*> &fuckedUpSections = QVector<Section*>(),
                                   bool loadingPossibility = false, bool passingPossibility = false);
    //поиск ближайшей опорной станции
    Station *nearestStation(int srcSt);

    //очищаем фильтры по участкам и станциям (при планировании очередной заявки)
    void clearFilters();
    //добавление станции в фильтр
    void addStationToFilter(Station *st);
    //добавление участка в фильтр
    void addSectionToFilter(Section *sec);
    //поиск участка с максимальным количеством попаданий в список проблемных участков
    //движение по которым затруднено из-за загруженности
    Section *findMostTroubleSection(QVector<Section *> troubleSections);

public:
    bool waitForRespond(int VP, int KP, int NP, int hours);
};

#endif // GRAPH_H
