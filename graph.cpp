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
    //-----------------------------------------------------------------------------------------------------------------
    //расчёт оптимального маршрута
    //если заявка содержит обязательные станции маршрута, считаем оптимальный путь от начала до конца через эти станции
    static QVector<section> fuckedUpSections;
    bool b_pathFound;
    Route tmpRoute(r, this);
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
    tmpRoute.m_echelones = fillEchelones(&tmpRoute, MyTime(r->DG, r->CG, 0));
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
                    //tmpRoute.shift(offsetTime);
                    b_canPassSections = true;
                    break;
                }
                if(tmpRoute.canBeShifted(offsetTimeBack)) {
                    //tmpRoute.shift(offsetTimeNegative);
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

int Graph::distanceTillStation(int stationIndexInPassedStations, const QVector<station> &_marshrut)
{
    int l = distanceBetweenStations(0, stationIndexInPassedStations, _marshrut);
    return l;
}

int Graph::distanceBetweenStations(int sourceIndex, int destinationIndex, QVector<station> _marshrut)
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

bool Graph::optimalPath(int st1, int st2, QVector<station> *passedStations, const QVector<section> &fuckedUpSections, bool loadingPossibility = true, bool passingPossibility = true)
{
    QVector<section> manuallSections; //участки, в которые входят неопорные станции. Такие участки должны проверяться вручную, есть ли они в фильтре.
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
    station S1, S2;//станции, с которой и до которой реально рассчитывается маршрут

    SP = MyDB::instance()->stationByNumber(st1);
    SV = MyDB::instance()->stationByNumber(st2);
    //---------------------------------------------------------------------------------------------------------


    //если станции погрузки и выгрузки не являются узловыыми или промежуточными опорными, смотрим ближайшие
    //к ним узловые или опорные станции. Станции в структуре заявки и станции, идущие на вход алгоритма
    //тогда будут отличаться. И ЭТО ЯЛВЯЕТСЯ ПРОБЛЕМОЙ
    //---------------------------------------------------------------------------------------------------------
    if((SP.type != 1)&&(SP.type != 2)&&(SP.type != 3)) {
        section sectionStart;
        //если расстояние от неопорной станции до начала участка короче, используем для рассчёта ближайшую опорную
        if(SP.distanceTillStart < SP.distanceTillEnd) {
            S1 = MyDB::instance()->stationByNumber(SP.startNumber);
            sectionStart = MyDB::instance()->sectionByStations(S1, SP);
        }
        else {
            S1 = MyDB::instance()->stationByNumber(SP.endNumber);
            sectionStart = MyDB::instance()->sectionByStations(SP, S1);
        }
        if(!(S1 == SP))
            manuallSections.append(sectionStart);
    }
    else {
        S1 = SP;
    }
    //тоже самое делаем для станции выгрузки
    if((SV.type != 1)&&(SV.type != 2)&&(SV.type != 3)) {
        section sectionEnd;
        if(SV.distanceTillStart < SV.distanceTillEnd) {
            S2 = MyDB::instance()->stationByNumber(SV.startNumber);
            sectionEnd = MyDB::instance()->sectionByStations(S2, SV);
        }
        else {
            S2 = MyDB::instance()->stationByNumber(SV.endNumber);
            sectionEnd = MyDB::instance()->sectionByStations(SV, S2);
        }
        if(!(S2 == SV))
            manuallSections.append(sectionEnd);
    }
    else {
        S2 = SV;
    }
    //---------------------------------------------------------------------------------------------------------

    if(S1 == S2)
    {
        //здесь обрабатываем ситуацию, когда опорная станция начала == опорной станции конца.
        //это происходит при выполнении двух условий одновременно:
        //1)станция отправления или прибытия является неопорной (или обе неопорные)
        //2)неопорные станции находятся ближе к одной и той же опорной станции

        //соответственно формируем маршрут самостоятельно (вручную)
        if((S1 == SP) || (S1 == SV)) {
            station tmpS1 = SP, tmpS2 = SV;
            passedStations->append(tmpS1);
            passedStations->append(tmpS2);
            manuallSections.append(MyDB::instance()->sectionByStations(tmpS1, tmpS2));
        }
        else {
            station tmpS1 = SP, tmpS2 = S1, tmpS3 = SV;
            passedStations->append(tmpS1);
            passedStations->append(tmpS2);
            passedStations->append(tmpS3);
            manuallSections.append(MyDB::instance()->sectionByStations(tmpS1, tmpS2));
            manuallSections.append(MyDB::instance()->sectionByStations(tmpS2, tmpS3));
        }
        //проверяем, входят ли участки в список запрещённых к планированию
        foreach (section sec, manuallSections) {
            if(fuckedUpSections.contains(sec))
                return false;
        }
        return true;
    }
    //проверяем, входят ли участки в список запрещённых к планированию
    foreach (section sec, manuallSections) {
        if(fuckedUpSections.contains(sec))
            return false;
    }

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

    QVector<v> path;
    QVector<int> resultStationsNumbers;

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

    qDebug() << "dijkstra output: ";
    foreach (v _v, path) {
        qDebug() << g[_v].name;
    }

    QVector<int> orderedResultStationsNumber;
    foreach (int num, resultStationsNumbers) {
        orderedResultStationsNumber.push_front(num);
    }

//    если после расчёта оказалось, что вторая по счёту станция маршрута - равна станции, которая была дальней по отношению к
//    неопорной станции погрузки, исключаем первую станцию из маршрута и добавляем расстояние от неопорной до дальней
//    к ней опорной станции маршрута. WAT?!?
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

    //заполняем вектор станций маршрута для возврата из функции
    foreach (int n, orderedResultStationsNumber) {
        passedStations->append(MyDB::instance()->stationByNumber(n));
    }
    return true;
}

bool Graph::optimalPathWithOM(int st1, int st2, const QVector<int> OM, QVector<station> *passedStations, const QVector<section> &fuckedUpSections, bool loadingPossibility, bool passingPossibility)
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

//НАДО ИСПРАВИТЬ, ЧТОБЫ БЫЛА ВОЗМОЖНОСТЬ ЗАДАТЬ ВРЕМЯ СМЕЩЕНИЯ ОТПРАВЛЕНИЯ!!!
QVector<echelon> Graph::fillEchelones(Route *route, MyTime departureTime)
{
    QVector<echelon> echs;
    MyTime startTime = departureTime;
    for(int i = 0; i < route->m_sourceRequest->PK; i++) {
        int delay = 24 / route->m_sourceRequest->TZ;
        //если i-ый эшелон кратен темпу перевозки, добавлять разницу во времени отправления к следующему эшелону
        echelon ech(i, route);
        int j = 0;
        foreach (station st, route->m_passedStations) {
            //расчёт времени въезда каждого эшелона на очередную станцию маршрута
            MyTime  elapsedTime; // = Расстояние до станции / скорость
            double hours = ((double)distanceTillStation(j, route->m_passedStations) * 24.0) / 600.0; //часов до станции
            if(hours > int(hours))
                elapsedTime = MyTime::timeFromHours(hours + delay * i + 1) + startTime;
            else
                elapsedTime = MyTime::timeFromHours(hours + delay * i) + startTime;
            ech.timesArrivalToStations.append(elapsedTime);
            j++;
        }
        echs.append(ech);
    }
//    route->m_arrivalTime = echs.last().timesArrivalToStations.last();
    return echs;
}
