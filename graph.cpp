#include "graph.h"
#include "mydb.h"
#include <QStringList>
#include <QDebug>

Graph::Graph(): filterVertex(FilterVertex(g)), filterEdge(FilterEdge(g))/*, fg(getInt())*/
{
    foreach (station tmp, *(MyDB::instance()->stations())) {
        if(tmp.type != 4) {
            v vert = boost::add_vertex(g);
            nodes.push_back(vert);
            g[vert].number = tmp.number;
            g[vert].name = tmp.name;
        }
    }

    foreach (section tmp, *(MyDB::instance()->sections())) {
        v v1 = 0, v2 = 0;
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

        if((v1 != 0) && (v2 != 0)) {
            e e_tmp = boost::add_edge(v1, v2, g).first;
            g[e_tmp].distance = tmp.distance;
            g[e_tmp].stationNumber1 = tmp.stationNumber1;
            g[e_tmp].stationNumber2 = tmp.stationNumber2;
        }
    }
}

Stream Graph::planStream(Request *r, bool loadingPossibility, bool passingPossibility)
{
    qDebug() << QString::fromUtf8("Планируется поток: ") << r->getString();
    //-----------------------------------------------------------------------------------------------------------------
    //расчёт оптимального маршрута
    //если заявка содержит обязательные станции маршрута, считаем оптимальный путь от начала до конца через эти станции
    static QList<section> fuckedUpSections;
    bool b_pathFound;
    Stream tmpStream(r, this);
    if(!r->OM.isEmpty()) {
        b_pathFound = optimalPathWithOM(r->SP, r->SV,  r->OM, &tmpStream.m_passedStations, fuckedUpSections, loadingPossibility, passingPossibility);
    }
    //если ОМ нет, рассчитываем путь от начала до конца
    else {
        b_pathFound = optimalPath(r->SP, r->SV, &tmpStream.m_passedStations, fuckedUpSections, loadingPossibility, passingPossibility);
    }
    if(!b_pathFound) {
        qDebug() << QString::fromUtf8("Нельзя спланировать поток №%1").arg(r->NP);
        clearFilters();
        fuckedUpSections.clear();
        tmpStream.setFailed("Путь не найден");
        return tmpStream;
    }
    //-----------------------------------------------------------------------------------------------------------------

    //рассчитываем участки, через которые пройдёт маршрут (на основе информации об имеющихся станций)
    //если рассчёт идет от неопорной станции до опорной, выбирается участок, на котором лежат обе этих станции
    tmpStream.fillSections();
    //заполняем эшелоны потока (рассчёт времени проследования по станциям и подвижной состав)
    MyTime t = MyTime(r->DG, r->CG, 0);
    //
    QList<int> sectionSpeeds;
    foreach (section sec, tmpStream.m_passedSections) {
        sectionSpeeds.append(sec.speed);
    }
    tmpStream.m_echelones = tmpStream.fillEchelones(t,r->VP, r->PK, r->TZ, tmpStream.distancesTillStations(), sectionSpeeds);
    //время прибытия последнего эшелона на последнюю станцию маршрута
    tmpStream.m_arrivalTime = tmpStream.m_echelones.last().timesArrivalToStations.last();
    //рассчитываем пропускные возможности, которые будут заняты маршрутом в двумерный массив (участок:день)
    tmpStream.m_busyPassingPossibilities = tmpStream.calculatePV(tmpStream.m_echelones);

    //если планирование идёт с учётом погрузки и пропускной способности
    //перассчитываем поток до тех пор, пока он не сможет пройти по участкам
    //с учётом пропускной возможности

    //I
    //выхода из цикла осуществляется при выполнении следующих двух условий:
    //1)нельзя сместить спланированный поток в заданных пользователем пределах
    //2)не осталось объездных путей (проблемные участки вычёркиваются из графа, если сдвинуть поток в пределах нет возможности

    //II
    //1)нельзя сместить спланированный поток в заданных пользователем пределах
    //2)смещённого времени не хватает на проезд от станции погрузки до станции выгрузки
    if((loadingPossibility && passingPossibility) && (r->DG < 60)) {
        bool b_canPassSections = tmpStream.canPassSections(tmpStream.m_passedSections,
                                                                 tmpStream.m_busyPassingPossibilities,
                                                                 MyTime(0, 0, 0), &fuckedUpSections);
        if(!b_canPassSections) {
            //---------------------------------------------------------------------------------------------------------
            //[0]сравниваем пропускную способность участков с заданным темпом
            bool b_psMoreThanTz = true;
            foreach (section sec, fuckedUpSections) {
                if(sec.ps < tmpStream.m_sourceRequest->TZ) {
                    //сдвиг не возможен
                    station st1 = MyDB::instance()->stationByNumber(sec.stationNumber1);
                    station st2 = MyDB::instance()->stationByNumber(sec.stationNumber2);
                    qDebug() << QString::fromUtf8("Пропускная способность участка %1 - %2 меньше заданного темпа.")
                                .arg(st1.name)
                                .arg(st2.name);
                    b_psMoreThanTz = false;
                    break;
                }
            }
            //[!0]
            if(b_psMoreThanTz) {
                //---------------------------------------------------------------------------------------------------------
                //[1]проверяем, сможем ли сместить поток в допустимых пределах
                MyTime acceptableOffset(3, 0, 0); //допустимое смещение по умолчанию = 3 дням
                int acceptableHours = qAbs(acceptableOffset.toHours());
                int i = 1;

                //здесь сохраним участки, через которые не удалось пройти со смещением времени
                //проблемные участки будут повторяться столько раз, сколько будут встречаться
                //при попытке сдвига времени отправления.
                //(при стандартном сдвиге в 3 дня (72ч + 72ч = 144ч - столько раз участок может быть продублирован здесь)
                QList<section> troubleSections;

                while(i <= acceptableHours) {
                    MyTime offsetTime(0, i, 0);
                    MyTime offsetTimeBack(0, -i, 0);
                    if(tmpStream.canBeShifted(offsetTime, &troubleSections)) {
                        //смещаем поток (перерасчитываем время проследования эшелонов и время убытия/прибытия потока,
                        //а также погрузочную и пропускную возможность по дням
                        tmpStream.shiftStream(offsetTime);
                        b_canPassSections = true;
                        break;
                    }
                    if(tmpStream.canBeShifted(offsetTimeBack, &troubleSections)) {
                        tmpStream.shiftStream(offsetTimeBack);
                        b_canPassSections = true;
                        break;
                    }
                    ++i;
                }                
                //[!1]

                //если поток можно сдвинуть, сдвигаем его и продолжаем работу над следующим
                if(b_canPassSections) {
                    clearFilters();
                    fuckedUpSections.clear();
                    tmpStream.setPlanned(true);
                    return tmpStream;
                }
                //иначе проверяем в чём проблема
                else {
                    //если есть проблемные участки, добавляем самый проблемный в фильтр
                    if(!troubleSections.isEmpty()) {
                        section mostTroubleSection = findMostTroubleSection(troubleSections);
                        fuckedUpSections.append(mostTroubleSection);
                    }
                    //если же проблемных участков нет - дело в том, что потоку не хватает времени для того
                    //чтобы пройти маршрут (даже с учётом сдвига)
                    //такой поток не может быть спланирован
                    else {
                        clearFilters();
                        fuckedUpSections.clear();
                        tmpStream.setFailed("Потоку не хватает времени для того, чтобы пройти маршрут");
                        return tmpStream;
                    }
                }
                //---------------------------------------------------------------------------------------------------------
            }

            //---------------------------------------------------------------------------------------------------------
            //[2]если мы добрались до этого момента, значит смещение не удалось и надо перерасчитывать маршрут
            //с учётом новых станций в фильтре
            tmpStream = planStream(tmpStream.m_sourceRequest, loadingPossibility, passingPossibility);
            //[!2]
            //---------------------------------------------------------------------------------------------------------
        }
    }
    clearFilters();
    fuckedUpSections.clear();
    tmpStream.setPlanned(true);
    return tmpStream;
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
                int uchDist = stCur.distanceTillEnd + stCur.distanceTillEnd;
                if(stCur.distanceTillStart < stNext.distanceTillStart) {
                    distance += uchDist - (stCur.distanceTillStart + stNext.distanceTillEnd);
                }
                else {
                    distance += uchDist - (stNext.distanceTillStart + stCur.distanceTillEnd);
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
        //[2]если первая станция - неопорная, а вторая - опорная
        else if((stCur.type == 4) && (stNext.type != 4)) {
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
        //[!2]

        //[3]
        else if((stCur.type != 4) && (stNext.type == 4)) {
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
        //[!3]
        //[4]если обе станции - опорные
        else {
            //находим ребро графа между станциями и вытаскиваем расстояние
            e _e = edgeBetweenStations(stCur, stNext);
            distance += g[_e].distance;
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

    //заполняем структуры станций погрузки и выгрузки - они нам понадобятся при планировании
    //---------------------------------------------------------------------------------------------------
    station SP, SV;//станции погрузки и выгрузки из структуры заявок

    SP = MyDB::instance()->stationByNumber(st1);
    SV = MyDB::instance()->stationByNumber(st2);
    //---------------------------------------------------------------------------------------------------------
    QList<station> startStations;
    QList<station> endStations;

    //если станции погрузки или выгрузки не являются опорными
    //проверяем, лежат ли они на одном участке
    //если так, дийкстра нам не нужен
    //---------------------------------------------------------------------------------------------------------
    if(SP.type == 4) {
        //[3-4]
        //если обе станции находятся на одном участке - возвращаем
        //маршрут из этих двух станций
        if(SV.type == 4) {
            if((SP.startNumber == SV.startNumber)&&(SP.endNumber == SV.endNumber)) //[4]
            {
                //возвращаем маршрут из двух станций (SP, SV)
                passedStations->append(SP);
                passedStations->append(SV);
                return true;
            }
        }
        else {
            if((SP.startNumber == SV.number) || (SP.endNumber == SV.number)) //[3]
            {
                //возвращаем маршрут из двух станций (SP, SV)
                passedStations->append(SP);
                passedStations->append(SV);
                return true;
            }
        }
        //[!3-4]
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
        //[3]
        //если обе станции находятся на одном участке - возвращаем
        //маршрут из этих двух станций
        if((SV.startNumber == SP.number) || (SV.endNumber == SP.number)) //[3]
        {
            //возвращаем маршрут из двух станций (SP, SV)
            passedStations->append(SP);
            passedStations->append(SV);
            return true;
        }
        //[!3]
        station SV_start, SV_end;
        SV_start = MyDB::instance()->stationByNumber(SV.startNumber);
        SV_end = MyDB::instance()->stationByNumber(SV.endNumber);
        endStations.append(SV_start);
        endStations.append(SV_end);
    }
    else {
        endStations.append(SV);
    }

    //---------------------------------------------------------------------------------------------------------
    //[!0]
    //заполняем списки маршрутов опорными станциями
    QList<QList<station> > paths;
    for(int i = 0; i < startStations.size(); i++) {
        for(int j = 0; j < endStations.size(); j++) {
            //[!1]
            //заполняем вектор станций маршрута для возврата из функции
            QList<station> stationList = dijkstraPath(startStations.at(i).number, endStations.at(j).number, fuckedUpSections, loadingPossibility, passingPossibility);
            if(!stationList.isEmpty()) {
                if(stationList.first() != SP) stationList.prepend(SP);
                if(stationList.last() != SV) stationList.append(SV);
                paths.append(stationList);
            }
        }
    }

    if(paths.isEmpty()) {
        //не найдено путей обхода
        return false;
    }
    QList<int> lengths;
    foreach (QList<station> stList, paths) {
        int dist = distanceBetweenStations(0, stList.size() - 1, stList);
        lengths.append(dist);
    }

    qDebug() << "Lengths are: " << lengths;

    //смотрим, у какого из маршрутов меньше длина (ищем индекс)
    int min_index = 0;
    for(int i = 0; i < lengths.size() - 1; i++)
    {
        if(lengths.at(i+1) < lengths.at(min_index))
            min_index = i+1;
    }

    //добавляем в конец найденные станции маршрута
    foreach (station st, paths.at(min_index)) {
        passedStations->append(st);
    }

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

QList<station> Graph::dijkstraPath(int st1, int st2, const QList<section> &fuckedUpSections, bool loadingPossibility, bool passingPossibility)
{
    foreach (section sec, fuckedUpSections) {
        addSectionToFilter(sec);
    }
    boost::filtered_graph <graph_t, FilterEdge, FilterVertex> fg(g, filterEdge, filterVertex);

    QVector<int> d(boost::num_vertices(g));
    QVector<v> p(boost::num_vertices(g));//predecessor map
    //[1]рассчитываем маршрут
    //ищем вершины соответствующие станциям погрузки и выгрузки - они понадобятся при расчёте оптимального пути
    //---------------------------------------------------------------------------------------------------------
    v v1, v2;
    foreach (v tmp, nodes) {
        if(g[tmp].number == st1) {
            v1 = tmp;
            break;
        }
    }
    foreach (v tmp, nodes) {
        if(g[tmp].number == st2) {
            v2 = tmp;
            break;
        }
    }
    //----------------------------------------------------------------------------------------------------
    //если вершина-начала = вершине-концу, возвращаем пустой путь
    if(v1 == v2) {

    }


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
    QList<station> stations;

    path.push_front(u);
    resultStationsNumbers << g[u].number;
    while(p[u] != u) {
        path.push_front(p[u]);
        resultStationsNumbers << g[p[u]].number;
        u = p[u];
    }

    if(path.count() == 1) {
        //если в пути содержится одна конечная вершина, значит пути до этой вершины не существует
        return stations;
    }

    QList<int> orderedResultStationsNumber;
    foreach (int num, resultStationsNumbers) {
        orderedResultStationsNumber.push_front(num);
    }

    foreach (int n, orderedResultStationsNumber) {
        station st = MyDB::instance()->stationByNumber(n);
        stations.append(st);
    }
    return stations;
}

section Graph::findMostTroubleSection(const QList<section> &troubleSections)
{
    QMap<section, int> troubleMap;
    foreach (section s, troubleSections) {
        if(troubleMap.contains(s)) {
            //если такой участок уже есть в Map'e, добавляем +1 к количеству совпадений и продолжаем
            int counts = troubleMap.value(s);
            troubleMap.insert(s, counts + 1);
        }
        else {
            //если нет, создаём новый
            troubleMap.insert(s, 1);
        }
    }

    //находим ключ с максимальным значением совпадений
    int max = 0;
    foreach (int n, troubleMap.values()) {
        max = qMax(max, n);
    }
    section sec = troubleMap.key(max);

    return sec;
}
