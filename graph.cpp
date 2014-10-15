#include "graph.h"
#include "mydb.h"
#include <QStringList>
#include <QDebug>

Graph::Graph(): filterVertex(FilterVertex(g)), filterEdge(FilterEdge(g))/*, fg(getInt())*/
{
    foreach (station tmp, *(MyDB::instance()->stations())) {
        v vert = boost::add_vertex(g);
        nodes.push_back(vert);
        g[vert].number = tmp.number;
        g[vert].name = tmp.name;
    }

    foreach (section tmp, *(MyDB::instance()->sections())) {
        v v1, v2;
        foreach (v tmp_stat1, nodes) {
            if(tmp.stationNumber1 == g[tmp_stat1].number) {
                v1 = tmp_stat1;
                break;
            }
        }
        foreach (v tmp_stat2, nodes) {
            if(tmp.stationNumber2 == g[tmp_stat2].number) {
                v2 = tmp_stat2;
                break;
            }
        }

        e e_tmp = boost::add_edge(v1, v2, g).first;
        g[e_tmp].distance = tmp.distance;
        g[e_tmp].stationNumber1 = g[v1].number;
        g[e_tmp].stationNumber2 = g[v2].number;
    }
}

Route Graph::planStream(Request *r, bool loadingPossibility, bool passingPossibility)
{
    Route tmpRoute(r, this);
    if(!r->OM.isEmpty()) {
        tmpRoute.m_passedStations = optimalPath(r->SP, r->OM[0], loadingPossibility, passingPossibility);
        int i = 1;
        while(i < r->OM.count())
        {
            tmpRoute.m_passedStations.removeLast();
            tmpRoute.m_passedStations += optimalPath(r->OM[i-1], r->OM[i], loadingPossibility, passingPossibility);
            i++;
        }
        tmpRoute.m_passedStations.removeLast();
        tmpRoute.m_passedStations += optimalPath(r->OM.last(), r->SV, loadingPossibility, passingPossibility);
    }
    else {
        tmpRoute.m_passedStations = optimalPath(r->SP, r->SV, loadingPossibility, passingPossibility);
    }
    tmpRoute.fillSections();
    tmpRoute.m_echelones = fillEchelones(&tmpRoute);
    tmpRoute.m_busyPassingPossibilities = tmpRoute.calculatePV(tmpRoute.m_echelones);

    if(loadingPossibility && passingPossibility) {
        QVector<section> fuckedUpSections;
        while(!tmpRoute.canPassSections(tmpRoute.m_passedSections, tmpRoute.m_busyPassingPossibilities, MyTime(0, 0, 0), &fuckedUpSections)) {
            foreach (section sec, fuckedUpSections) {
                addSectionToFilter(sec);
            }
            qDebug() << tmpRoute.print();
            tmpRoute = planStream(tmpRoute.m_sourceRequest, true, true);
            clearFilters();
            tmpRoute.setPlanned(true);
            return tmpRoute;
        }
    }
    clearFilters();
    tmpRoute.setPlanned(true);
    return tmpRoute;
}

int Graph::distanceTillStation(int destStationNumber, const QVector<station> &_marshrut)
{
    int src = _marshrut[0].number;
    int l = distanceBetweenStations(src, destStationNumber, _marshrut);
    return l;
}

int Graph::distanceBetweenStations(int source, int destination, QVector<station> _marshrut)
{
    int distance = 0;
    if(source == destination) return 0;
    station src, dest;
    src = MyDB::instance()->stationByNumber(source);
    dest = MyDB::instance()->stationByNumber(destination);
        //если маршрут не содержит хотя бы одной из этих станций, выдаём ошибку и выходим
//    if(!_marshrut.contains(src)) {
//        qDebug() << "Нельзя найти расстояние до станции " << src.name << " так как её нет в маршруте";
//        exit(1);
//    }
//    if(!_marshrut.contains(dest)) {
//        qDebug() << "Нельзя найти расстояние до станции " << dest.name << " так как её нет в маршруте";
//        exit(1);
//    }

    QVector<station> relianceStations;// опорные станции, между которыми будет считаться расстояние
    int indSrc, indDest;
    indSrc = _marshrut.indexOf(src);
    indDest = _marshrut.indexOf(dest);

    //если первая станция - неопорная, прибавляем расстояние от неё до следующей
    if(src.type == 4) {
        if(src.endNumber == _marshrut.at(indSrc + 1).number)
            distance += src.distanceTillEnd;
        else
            distance += src.distanceTillStart;
    }
    //если последняя станция - неопорная, прибавляем расстояние от последней до неё
    if(dest.type == 4) {
        if(dest.startNumber == _marshrut.at(indDest - 1).number)
            distance += dest.distanceTillStart;
        else
            distance += dest.distanceTillEnd;
    }
    //считаем расстояние между всеми опорными станциями

    int i = indSrc;
    do {
            if(_marshrut.at(i).type != 4)
                relianceStations.append(_marshrut.at(i));
            i++;
    }
    while(i != indDest + 1);

    for(int i = 0; i < relianceStations.count() - 1; i++)
    {
        e _e = edgeBetweenStations(relianceStations[i], relianceStations[i+1]);
        distance += g[_e].distance;
    }

    return distance;
}

e Graph::edgeBetweenStations(const station &st1, const station &st2)
{
    for(boost::graph_traits<graph_t>::edge_iterator it = boost::edges(g).first; it != boost::edges(g).second; ++it) {
        section sec = g[*it];
        if(g[*it].stationNumber1 == st1.number)
            if(g[*it].stationNumber2 == st2.number)
                return *it;
        if(g[*it].stationNumber1 == st2.number)
            if(g[*it].stationNumber2 == st1.number)
                return *it;
    }
    exit(5);
}

void Graph::clearFilters()
{
    filterEdge.clearFilter();
    filterVertex.clearFilter();
}

void Graph::addStationToFilter(station st)
{
    filterVertex.addStation(st);
}

void Graph::addSectionToFilter(section sec)
{
    filterEdge.addSection(sec);
}

QVector<station> Graph::optimalPath(int st1, int st2, bool loadingPossibility = true, bool passingPossibility = true)
{
    boost::filtered_graph <graph_t, FilterEdge, FilterVertex> fg(g, filterEdge, filterVertex);

    QVector<int> d(boost::num_vertices(g));
    QVector<v> p(boost::num_vertices(g));//predecessor map

    //заполняем структуры станций погрузки и выгрузки - они нам понадобятся при планировании
    //---------------------------------------------------------------------------------------------------
    station SP, SV;//станции погрузки и выгрузки из структуры заявок
    station S1, S2;//станции, с которой и до которой реально рассчитывается маршрут

    SP = MyDB::instance()->stationByNumber(st1);
    SV = MyDB::instance()->stationByNumber(st2);
    //---------------------------------------------------------------------------------------------------------


    //если станции погрузки и выгрузки не являются узловыыми или промежуточными опорными, смотрим ближайшие
    //к ним узловые или опорные станции. Станции в структуре заявки и станции, идущие на вход алгоритма
    //тогда будут отличаться
    //---------------------------------------------------------------------------------------------------------
    if((SP.type != 1)&&(SP.type != 2)&&(SP.type != 3)) {
        //если расстояние от неопорной станции до начала участка короче, используем для рассчёта ближайшую опорную
        if(SP.distanceTillStart < SP.distanceTillEnd) {
            S1 = MyDB::instance()->stationByNumber(SP.startNumber);
        }
        else {
            S1 = MyDB::instance()->stationByNumber(SP.endNumber);
        }
    }
    else {
        S1 = SP;
    }
    //тоже самое делаем для станции выгрузки
    if((SV.type != 1)&&(SV.type != 2)&&(SV.type != 3)) {
        if(SV.distanceTillStart < SV.distanceTillEnd) {
            S2 = MyDB::instance()->stationByNumber(SV.startNumber);
        }
        else {
            S2 = MyDB::instance()->stationByNumber(SV.endNumber);
        }
    }
    else {
        S2 = SV;
    }
    //---------------------------------------------------------------------------------------------------------

    //ищем вершины соответствующие станциям погрузки и выгрузки - они понадобятся при расчёте оптимального пути
    //---------------------------------------------------------------------------------------------------------
    v v1, v2;
    foreach (v tmp, nodes) {
        if(g[tmp].number == S1.number) {
            v1 = tmp;
            break;
        }
    }
    foreach (v tmp, nodes) {
        if(g[tmp].number == S2.number) {
            v2 = tmp;
            break;
        }
    }
    //----------------------------------------------------------------------------------------------------


    //выбор алгоритма в зависимости от параметров планирования
    //----------------------------------------------------------------------------------------------------
    if(loadingPossibility && passingPossibility) {
        boost::dijkstra_shortest_paths(fg, v1, boost::weight_map(get(&section::distance, g))
                .distance_map(boost::make_iterator_property_map(d.begin(), get(boost::vertex_index, g)))
                .predecessor_map(boost::make_iterator_property_map(p.begin(), get(boost::vertex_index, g))));
    }
    else {
        boost::dijkstra_shortest_paths(g, v1, boost::weight_map(get(&section::distance, g))
                .distance_map(boost::make_iterator_property_map(d.begin(), get(boost::vertex_index, g)))
                .predecessor_map(boost::make_iterator_property_map(p.begin(), get(boost::vertex_index, g))));
    }
    //----------------------------------------------------------------------------------------------------

    int i = nodes.indexOf(v2);

//    qDebug() << "Path length from vertice " << S1.name << " to vertice " << S2.name << ": " << My << QString::fromUtf8("км.");
//    qDebug()<< "Маршрут " << r->NP << " потока:";

    boost::graph_traits<graph_t>::vertex_descriptor u = nodes[i];

    QVector<int> resultStationsNumbers;
    QStringList pathList;
    resultStationsNumbers << g[u].number;
    pathList << g[u].name;
    while(p[u] != u) {
        pathList << g[p[u]].name;
        resultStationsNumbers << g[p[u]].number;
        u = p[u];
    }
    QVector<int> orderedResultStationsNumber;
    QStringList pathListOrdered;
    foreach (QString str, pathList) {
        pathListOrdered.push_front(str);
    }
    foreach (int num, resultStationsNumbers) {
        orderedResultStationsNumber.push_front(num);
    }

//    если после расчёта оказалось, что вторая по счёту станция маршрута - равна станции, которая была дальней по отношению к
//    неопорной станции погрузки, исключаем первую станцию из маршрута и добавляем расстояние от неопорной до дальней
//    к ней опорной станции маршрута.
//    это позволяет избежать ситуации, когда нам прийдётся дойти до ближайщей опорной станции и вернуться назад, чтобы продолжить
//    движение
//    БУДЕТ ОШИБКА, ЕСЛИ МАРШРУТ ЛЕЖИТ НА ОДНОМ УЧАСТКЕ И СТАНЦИЯ ОТПРАВЛЕНИЯ НЕ ЯВЛЯЕТСЯ ОПОРНОЙ
    if(SP.number != S1.number)
    {
        if(SP.distanceTillStart < SP.distanceTillEnd)
        {
            if(SP.endNumber == orderedResultStationsNumber[1])
            {
                orderedResultStationsNumber.removeAt(0);
            }
        }
        else
        {
            if(SP.startNumber == orderedResultStationsNumber[1])
            {
                orderedResultStationsNumber.removeAt(0);
            }
        }
        orderedResultStationsNumber.push_front(SP.number);
    }

    if(SV.number != S2.number)
    {
        if(SV.distanceTillEnd < SV.distanceTillStart)
        {
            if(SV.startNumber == orderedResultStationsNumber[orderedResultStationsNumber.count() - 2])
            {
                orderedResultStationsNumber.removeAt(orderedResultStationsNumber.count() - 1);
            }
        }
        else
        {
            if(SV.endNumber == orderedResultStationsNumber[orderedResultStationsNumber.count() - 2])
            {
                orderedResultStationsNumber.removeAt(orderedResultStationsNumber.count() - 1);
            }
        }
        orderedResultStationsNumber.push_back(SV.number);
    }

    QStringList marshrut;
    QVector<station> passedStations;
    foreach (int n, orderedResultStationsNumber) {
        marshrut << MyDB::instance()->stationByNumber(n).name;
        passedStations.append(MyDB::instance()->stationByNumber(n));
    }
    return passedStations;
}

QVector<echelon> Graph::fillEchelones(Route *route)
{
    QVector<echelon> echs;
    MyTime startTime(route->m_sourceRequest->DG, route->m_sourceRequest->CG, 0);
    for(int i = 0; i < route->m_sourceRequest->PK; i++) {
        int delay = 24 / route->m_sourceRequest->TZ;
        //если i-ый эшелон кратен темпу перевозки, добавлять разницу во времени отправления к следующему эшелону
        echelon ech(i, route);
        foreach (station st, route->m_passedStations) {
            //расчёт времени въезда каждого эшелона на очередную станцию маршрута
            MyTime  elapsedTime; // = Расстояние до станции / скорость
            double hours = ((double)distanceTillStation(st.number, route->m_passedStations) * 24.0) / 600.0; //часов до станции
            if(hours > int(hours))
                elapsedTime = MyTime::timeFromHours(hours + delay * i + 1) + startTime;
            else
                elapsedTime = MyTime::timeFromHours(hours + delay * i) + startTime;
            ech.timesArrivalToStations.append(elapsedTime);
        }
        echs.append(ech);
    }
    route->m_departureTime = echs.first().timesArrivalToStations.first();
    route->m_arrivalTime = echs.last().timesArrivalToStations.last();
    return echs;
}
