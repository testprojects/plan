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

Stream Graph::planStream(Request *r, bool loadingPossibility, bool passingPossibility)
{
    //-----------------------------------------------------------------------------------------------------------------
    //расчёт оптимального маршрута
    //если заявка содержит обязательные станции маршрута, считаем оптимальный путь от начала до конца через эти станции
    static QList<section> fuckedUpSections;
    bool b_pathFound;
    Stream tmpRoute(r, this);
    if(!r->OM.isEmpty()) {
        b_pathFound = optimalPathWithOM(r->SP, r->SV,  r->OM, &tmpRoute.m_passedStations, fuckedUpSections, loadingPossibility, passingPossibility);
    }
    //если ОМ нет, рассчитываем путь от начала до конца
    else {
        b_pathFound = optimalPath(r->SP, r->SV, &tmpRoute.m_passedStations, fuckedUpSections, loadingPossibility, passingPossibility);
    }
    if(!b_pathFound) {
        qDebug() << QString::fromUtf8("Нельзя спланировать поток №%1").arg(r->NP);
        clearFilters();
        tmpRoute.setFailed("Путь не найден");
        return tmpRoute;
    }
    //-----------------------------------------------------------------------------------------------------------------

    //рассчитываем участки, через которые пройдёт маршрут (на основе информации об имеющихся станций)
    //если рассчёт идет от неопорной станции до опорной, выбирается участок, на котором лежат обе этих станции
    tmpRoute.fillSections();
    //заполняем эшелоны потока (рассчёт времени проследования по станциям и подвижной состав)
    MyTime t = MyTime(r->DG, r->CG, 0);
    //
    QList<int> sectionSpeeds;
    foreach (section sec, tmpRoute.m_passedSections) {
        sectionSpeeds.append(sec.speed);
    }
    tmpRoute.m_echelones = tmpRoute.fillEchelones(t, r->PK, r->TZ, tmpRoute.distancesTillStations(), sectionSpeeds);
    //время прибытия последнего эшелона на последнюю станцию маршрута
    tmpRoute.m_arrivalTime = tmpRoute.m_echelones.last().timesArrivalToStations.last();
    //рассчитываем пропускные возможности, которые будут заняты маршрутом в двумерный массив (участок:день)
    tmpRoute.m_busyPassingPossibilities = tmpRoute.calculatePV(tmpRoute.m_echelones);

    //если планирование идёт с учётом погрузки и пропускной способности
    //перассчитываем поток до тех пор, пока он не сможет пройти по участкам
    //с учётом пропускной возможности

    //выхода из цикла осуществляется при выполнении следующих двух условий:
    //1)нельзя сместить спланированный поток в заданных пользователем пределах
    //2)не осталось объездных путей (проблемные участки вычёркиваются из графа, если сдвинуть поток в пределах нет возможности
    if(loadingPossibility && passingPossibility) {
        bool b_canPassSections = tmpRoute.canPassSections(tmpRoute.m_passedSections,
                                                                 tmpRoute.m_busyPassingPossibilities,
                                                                 MyTime(0, 0, 0), &fuckedUpSections);
        if(!b_canPassSections) {
            //---------------------------------------------------------------------------------------------------------
            //[0]сравниваем пропускную способность участков с заданным темпом
            bool b_psMoreThanTz = true;
            foreach (section sec, fuckedUpSections) {
                if(sec.ps < tmpRoute.m_sourceRequest->TZ) {
                    //сдвиг не возможен
                    qDebug() << QString::fromUtf8("Пропускная способность участка %1 - %2 меньше заданного темпа. Нельзя спланировать маршрут")
                                .arg(MyDB::instance()->stationByNumber(sec.stationNumber1).name)
                                .arg(MyDB::instance()->stationByNumber(sec.stationNumber2).name);
                    b_psMoreThanTz = false;
                    break;
                }
            }
            //[!0]
            if(b_psMoreThanTz) {
                //---------------------------------------------------------------------------------------------------------
                //[1]проверяем, сможем ли сместить поток в допустимых пределах
                MyTime acceptableOffset(10, 0, 0); //допустимое смещение по умолчанию = 3 дням
                int acceptableHours = qAbs(acceptableOffset.toHours());
                int i = 1;

                while(i <= acceptableHours) {
                    MyTime offsetTime(0, i, 0);
                    MyTime offsetTimeBack(0, -i, 0);
                    if(tmpRoute.canBeShifted(offsetTime)) {
                        //смещаем поток (перерасчитываем время проследования эшелонов и время убытия/прибытия потока,
                        //а также погрузочную и пропускную возможность по дням
                        tmpRoute.shiftStream(offsetTime);
                        b_canPassSections = true;
                        break;
                    }
                    if(tmpRoute.canBeShifted(offsetTimeBack)) {
                        tmpRoute.shiftStream(offsetTimeBack);
                        b_canPassSections = true;
                        break;
                    }
                    ++i;
                }

                if(b_canPassSections) {
                    clearFilters();
                    tmpRoute.setPlanned(true);
                    return tmpRoute;
                }
                //[!1]
                //---------------------------------------------------------------------------------------------------------
            }

            //---------------------------------------------------------------------------------------------------------
            //[2]если мы добрались до этого момента, значит смещение не удалось и надо перерасчитывать маршрут

            //добавляем станции, через которые не удалось пройти, в фильтр
            foreach (section sec, fuckedUpSections) {
                addSectionToFilter(sec);
            }
            //ненавижу рекурсию...
            tmpRoute = planStream(tmpRoute.m_sourceRequest, loadingPossibility, passingPossibility);
            //[!2]
            //---------------------------------------------------------------------------------------------------------
        }
    }
    clearFilters();
    tmpRoute.setPlanned(true);
    return tmpRoute;
}

int Graph::distanceTillStation(int stationIndexInPassedStations, const QList<station> &_marshrut)
{
    int l = distanceBetweenStations(0, stationIndexInPassedStations, _marshrut);
    return l;
}

int Graph::distanceBetweenStations(int sourceIndex, int destinationIndex, QList<station> _marshrut)
{
    int distance = 0;

    if(sourceIndex == destinationIndex) return 0;
    if(sourceIndex > destinationIndex) {
        qDebug() << QString::fromUtf8("Нельзя рассчитать расстояние между станциями %1 и %2, т.к. они идут в обратном порядке маршрута")
                    .arg(_marshrut[sourceIndex].name)
                    .arg(_marshrut[destinationIndex].name);
        exit(5);
    }

    for(int i = sourceIndex; i < destinationIndex; i++)
    {
        station stCur = _marshrut[i];
        station stNext = _marshrut[i + 1];
        //[1]если обе станции являются неопорными
        if((stCur.type == 4) && (stNext.type == 4)) {
            //[1.1]если они принадлежат одному участку
            if((stCur.startNumber == stNext.startNumber) && (stCur.endNumber == stNext.endNumber)) {
                if(stCur.distanceTillStart < stNext.distanceTillStart) {
                    distance += stCur.distanceTillStart + stNext.distanceTillEnd;
                }
                else {
                    distance += stNext.distanceTillStart + stCur.distanceTillEnd;
                }
            }
            //[!1.1]
            //[1.2]если принадлежат смежным участкам
            else {
                if(stCur.endNumber == stNext.startNumber) {
                    distance += stCur.distanceTillEnd + stNext.distanceTillStart;
                }
                else if(stCur.startNumber == stNext.endNumber) {
                    distance += stCur.distanceTillStart + stNext.distanceTillEnd;
                }
            //[!1.2]
                else {
                    //2 неопорные станции не принадлежат смежному участку
                    qDebug() << QString::fromUtf8("Две неопорные станции (%1, %2) идущие друг за другом не лежат на смежных участках: "
                                                  "проблема функции построения оптимального маршрута (искать там)")
                                .arg(stCur.name)
                                .arg(stNext.name);
                    exit(7);
                }
            }
        }
        //[!1]

        //[2]если обе станции - опорные
        else if((stCur.type == 1) && (stNext.type == 1)) {
            //находим ребро графа между станциями и вытаскиваем расстояние
            e _e = edgeBetweenStations(stCur, stNext);
            distance += g[_e].distance;
        }
        //[!2]

        //[3]если первая станция - неопорная, а вторая - опорная
        else if((stCur.type == 4) && (stNext.type == 1)) {
            if(stCur.endNumber == stNext.number) {
                distance += stCur.distanceTillEnd;
            }
            else if(stCur.startNumber == stNext.number) {
                distance += stCur.distanceTillStart;
            }
            else {
                qDebug() << "distanceBetweenStations error";
                exit(8);
            }
        }
        //[!3]

        //[4]
        else if((stCur.type == 1) && (stNext.type == 4)) {
            if(stNext.startNumber == stCur.number) {
                distance += stNext.distanceTillStart;
            }
            else if(stNext.endNumber == stCur.number) {
                distance += stNext.distanceTillStart;
            }
            else {
                qDebug() << "distanceBetweenStations error";
                exit(9);
            }
        }
        //[!4]
    }

    return distance;
}

e Graph::edgeBetweenStations(const station &st1, const station &st2)
{
    for(boost::graph_traits<graph_t>::edge_iterator it = boost::edges(g).first; it != boost::edges(g).second; ++it) {
        if(g[*it].stationNumber1 == st1.number)
            if(g[*it].stationNumber2 == st2.number)
                return *it;
        if(g[*it].stationNumber1 == st2.number)
            if(g[*it].stationNumber2 == st1.number)
                return *it;
    }
    qDebug() << QString::fromUtf8("Не удалось найти ребро графа между станциями: %1 и %2")
                .arg(st1.name)
                .arg(st2.name);
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

bool Graph::optimalPath(int st1, int st2, QList<station> *passedStations, const QList<section> &fuckedUpSections, bool loadingPossibility = true, bool passingPossibility = true)
{
    //[0]заполняем станции
    if(st1 == st2) {
        qDebug() << "Расчитывается маршрут, где станция назначения равна станции отправления. Такого быть не должно. Ну да ладно. Рассчитаем =)";
        passedStations->append(MyDB::instance()->stationByNumber(st1));
        return true;
    }
    boost::filtered_graph <graph_t, FilterEdge, FilterVertex> fg(g, filterEdge, filterVertex);

    QVector<int> d(boost::num_vertices(g));
    QVector<v> p(boost::num_vertices(g));//predecessor map

    //заполняем структуры станций погрузки и выгрузки - они нам понадобятся при планировании
    //---------------------------------------------------------------------------------------------------
    station SP, SV;//станции погрузки и выгрузки из структуры заявок

    SP = MyDB::instance()->stationByNumber(st1);
    SV = MyDB::instance()->stationByNumber(st2);
    //---------------------------------------------------------------------------------------------------------
    QList<station> startStations;
    QList<station> endStations;
    //если станции погрузки и выгрузки не являются узловыыми или промежуточными опорными, добавляем в маршруты ближайшие
    //к ним опорные станции.
    //---------------------------------------------------------------------------------------------------------
    if(SP.type == 4) {
        station SP_start, SP_end;
        SP_start = MyDB::instance()->stationByNumber(SP.startNumber);
        SP_end = MyDB::instance()->stationByNumber(SP.endNumber);
        startStations.append(SP_start);
        startStations.append(SP_end);
    }
    else {
        startStations.append(SP);
    }
    if(SV.type == 4) {
        station SV_start, SV_end;
        SV_start = MyDB::instance()->stationByNumber(SP.startNumber);
        SV_end = MyDB::instance()->stationByNumber(SP.endNumber);
        endStations.append(SV_start);
        endStations.append(SV_end);
    }
    else {
        endStations.append(SV);
    }

    //---------------------------------------------------------------------------------------------------------
    //[!0]
    QList<QList<station> > paths;
    for(int i = 0; i < startStations.size(); i++) {
        for(int j = 0; j < endStations.size(); j++) {
            //[1]рассчитываем маршрут
            //ищем вершины соответствующие станциям погрузки и выгрузки - они понадобятся при расчёте оптимального пути
            //---------------------------------------------------------------------------------------------------------
            v v1, v2;
            foreach (v tmp, nodes) {
                if(g[tmp].number == startStations.at(i).number) {
                    v1 = tmp;
                    break;
                }
            }
            foreach (v tmp, nodes) {
                if(g[tmp].number == endStations.at(j).number) {
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

            boost::graph_traits<graph_t>::vertex_descriptor u = nodes[i];

            QList<v> path;
            QList<int> resultStationsNumbers;

            path.push_front(u);
            resultStationsNumbers << g[u].number;
            while(p[u] != u) {
                path.push_front(p[u]);
                resultStationsNumbers << g[p[u]].number;
                u = p[u];
            }

            if(path.count() == 1) {
                //если в пути содержится одна конечная вершина, значит пути до этой вершины не существует
                return false;
            }

            QList<int> orderedResultStationsNumber;
            foreach (int num, resultStationsNumbers) {
                orderedResultStationsNumber.push_front(num);
            }
            //[!1]
            //заполняем вектор станций маршрута для возврата из функции
            QList<station> stationList;
            foreach (int num, orderedResultStationsNumber) {
                station st = MyDB::instance()->stationByNumber(num);
                stationList.append(st);
            }
            paths.append(stationList);
        }
    }

    QList<int> lengths;
    foreach (QList<station> stList, paths) {
        if(stList.first() != SP) stList.prepend(SP);
        if(stList.last() != SV) stList.append(SV);
        int dist = distanceBetweenStations(0, stList.size() - 1, stList);
        lengths.append(dist);
    }

    qDebug() << "Lengths are: " << lengths;

//    foreach (int n, orderedResultStationsNumber) {
//        passedStations->append(MyDB::instance()->stationByNumber(n));
//    }
    return true;
}

bool Graph::optimalPathWithOM(int st1, int st2, const QList<int> OM, QList<station> *passedStations, const QList<section> &fuckedUpSections, bool loadingPossibility, bool passingPossibility)
{
    if(!OM.isEmpty()) {
        if(!optimalPath(st1, OM[0], passedStations, fuckedUpSections, loadingPossibility, passingPossibility)) return false;
        int i = 1;
        while(i < OM.count())
        {
            passedStations->removeLast();
            if(!optimalPath(OM[i-1], OM[i], passedStations, fuckedUpSections, loadingPossibility, passingPossibility)) return false;
            i++;
        }
        passedStations->removeLast();
        if(!optimalPath(OM.last(), st2, passedStations, fuckedUpSections, loadingPossibility, passingPossibility)) return false;
    }
    return true;
}

