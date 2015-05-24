#include "graph.h"
#include "mydb.h"
#include "../myClient/types.h"
#include <QStringList>
#include <QDebug>
#include <QObject>
#include <QEventLoop>
#include "server.h"
#include "../myClient/packet.h"
#include "loopwrapper.h"
#include <QThread>
#include <QTcpSocket>
#include <QTimer>
#include "pauser.h"

//неопорная станция. не является узлом графа. но может быть станцией отправления/назначения
const int STATION_NOT_BEARING = 4;

Graph::Graph(const QVector<Station*> &stationList, const QVector<Section*> &sectionList, Server *server):
    QObject(server), m_server(server)
{

    foreach (Station *tmp, stationList) {
        if(tmp->type != STATION_NOT_BEARING) {
            v vert = boost::add_vertex(g);
            nodes.push_back(vert);
            g[vert].number = tmp->number;
            g[vert].name = tmp->name;
        }
    }

    foreach (Section *tmp, sectionList) {
        v v1 = 0, v2 = 0;
        foreach (v tmp_stat1, nodes) {
            if(tmp->stationNumber1 == g[tmp_stat1].number) {
                v1 = tmp_stat1;
                break;
            }
        }
        foreach (v tmp_stat2, nodes) {
            if(tmp->stationNumber2 == g[tmp_stat2].number) {
                v2 = tmp_stat2;
                break;
            }
        }

        if((v1 != 0) && (v2 != 0)) {
            e e_tmp = boost::add_edge(v1, v2, g).first;
            edges.push_back(e_tmp);
            g[e_tmp].distance = tmp->distance;
            g[e_tmp].time = tmp->time;
            g[e_tmp].stationNumber1 = tmp->stationNumber1;
            g[e_tmp].stationNumber2 = tmp->stationNumber2;
        }
    }
    filterEdge = new FilterEdge(&g);
    filterVertex = new FilterVertex(&g);
}

Graph::~Graph()
{

}

Stream* Graph::planStream(Request *r, bool loadingPossibility, bool passingPossibility, QString *errorString)
{
    Station *SP, *SV;
    SP = MyDB::instance()->stationByNumber(r->SP);
    if(!SP) {
        *errorString = QString::fromUtf8("Станция погрузки в заявке: VP=%1, KP=%2, NP=%3 с номером %4 не существует в базе. Планирование невозможно.")
                .arg(r->VP)
                .arg(r->KP)
                .arg(r->NP)
                .arg(r->SP);
        qDebug() << errorString;
        return NULL;
    }
    SV = MyDB::instance()->stationByNumber(r->SV);
    if(!SV) {
        *errorString = QString::fromUtf8("Станция выгрузки в заявке: VP=%1, KP=%2, NP=%3 с номером %4 не существует в базе. Планирование невозможно.")
                .arg(r->VP)
                .arg(r->KP)
                .arg(r->NP)
                .arg(r->SV);
        qDebug() << errorString;
        return NULL;
    }

    qDebug() << QString::fromUtf8("Планируется поток: ") << *r;
    //-----------------------------------------------------------------------------------------------------------------
    //расчёт оптимального маршрута
    //если заявка содержит обязательные станции маршрута, считаем оптимальный путь от начала до конца через эти станции
    static QVector<Section*> fuckedUpSections;
    bool b_pathFound;
    assert(r);
    Stream *tmpStream = MyDB::instance()->stream(r->VP, r->KP, r->NP);
    if(tmpStream != NULL) {
        if(errorString) {
            *errorString = QString::fromUtf8("Поток %1 уже спланирован").arg(*r);
            qDebug() << *errorString;
        }
        return NULL;
    }
    else {
        tmpStream = new Stream(r);
    }
    if(!r->OM.isEmpty()) {
        b_pathFound = optimalPathWithOM(r->SP, r->SV,  r->OM, &tmpStream->m_passedStations, fuckedUpSections, loadingPossibility, passingPossibility);
    }
    //если ОМ нет, рассчитываем путь от начала до конца
    else {
        b_pathFound = optimalPath(r->SP, r->SV, &tmpStream->m_passedStations, fuckedUpSections, loadingPossibility, passingPossibility);
    }
    if(!b_pathFound) {
        if(errorString) {
            *errorString = QString::fromUtf8("Нельзя спланировать поток №%1: Оптимальный путь не найден.").arg(r->NP);
            qDebug() << *errorString;
        }
        clearFilters();
        fuckedUpSections.clear();
        if(tmpStream)
            delete tmpStream;
        return NULL;
    }
    QString strRoute;
    foreach (Station *sRoute, tmpStream->m_passedStations) {
        strRoute += QString("%1, ").arg(*sRoute);
    }
    strRoute.chop(2);

    qDebug() << QString::fromUtf8("Оптимальный маршрут: %1").arg(strRoute);
    qDebug() << QString::fromUtf8("Длина: %1")
                .arg(distanceBetweenStations(0, tmpStream->m_passedStations.count() - 1, tmpStream->m_passedStations));
    //-----------------------------------------------------------------------------------------------------------------

    //рассчитываем участки, через которые пройдёт маршрут (на основе информации об имеющихся станций)
    //если рассчёт идет от неопорной станции до опорной, выбирается участок, на котором лежат обе этих станции
    tmpStream->m_passedSections = Stream::fillSections(tmpStream->m_passedStations);
    if(tmpStream->m_passedSections.isEmpty()) {
        *errorString = QString::fromUtf8("Ошибка при нахождении участков между станциями маршрута: ");
        foreach (Station *st, tmpStream->m_passedStations) {
            *errorString += '\n';
            *errorString += *st;
        }
        qDebug() << *errorString;
        return NULL;
    }
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
    //заполняем эшелоны потока (рассчёт времени проследования по станциям и подвижной состав)
    MyTime t = MyTime(r->DG - 1, r->CG, 0);
    //
    QList<int> sectionSpeeds;
    foreach (Section *sec, tmpStream->m_passedSections) {
        sectionSpeeds.append(sec->speed);
    }
    QList<float>distances = tmpStream->distancesBetweenStations(true);
    tmpStream->m_echelones = tmpStream->fillEchelonesInMinutes(t,r->VP, r->PK, r->TZ, distances, sectionSpeeds);
    //время прибытия последнего эшелона на последнюю станцию маршрута
    tmpStream->m_arrivalTime = tmpStream->m_echelones.last().timesArrivalToStations.last();
    if((loadingPossibility && passingPossibility) && (r->DG < 60)) {

        //рассчитываем пропускные возможности, которые будут заняты маршрутом в двумерный массив (участок:день)
        tmpStream->m_busyPassingPossibilities = tmpStream->calculatePV(tmpStream->m_echelones);

        int i_canPassSections = tmpStream->canPassSections(tmpStream->m_passedSections,
                                                                 tmpStream->m_busyPassingPossibilities, &fuckedUpSections);
        switch(i_canPassSections)
        {
        //[0]--------------------------------------------------------------------------------------------------------
        case 0:
        {
            planStream(r, loadingPossibility, passingPossibility);
            break;
        }
        //[!0]--------------------------------------------------------------------------------------------------------
        //[1]--------------------------------------------------------------------------------------------------------
        case 1:
        {
            //[1]проверяем, сможем ли сместить поток в допустимых пределах
            MyTime acceptableOffset(10, 0, 0); //допустимое смещение по умолчанию = 3 дням
            int acceptableHours = qAbs(acceptableOffset.toHours());
            int i = 1;

            //здесь сохраним участки, через которые не удалось пройти со смещением времени
            //проблемные участки будут повторяться столько раз, сколько будут встречаться
            //при попытке сдвига времени отправления.
            //(при стандартном сдвиге в 3 дня (72ч + 72ч = 144ч - столько раз участок может быть продублирован здесь)
            QList<Section*> troubleSections;
            bool b_canBeShifted = false;
            bool b_wantToBeShifted = true;

            while(i <= acceptableHours) {
                if(tmpStream->canBeShifted(-i, &troubleSections)) {
                    //смещаем поток (перерасчитываем время проследования эшелонов и время убытия/прибытия потока,
                    //а также погрузочную и пропускную возможность по дням
#ifdef WAIT_FOR_RESPOND
                    b_wantToBeShifted = (bool) waitForRespond(tmpStream, troubleSections, -i);
#endif
                    if(b_wantToBeShifted)
                        tmpStream->shiftStream(-i);
                    b_canBeShifted = true;
                    break;
                }
                else if(tmpStream->canBeShifted(i, &troubleSections)) {
#ifdef WAIT_FOR_RESPOND
                     b_wantToBeShifted = (bool) waitForRespond(tmpStream, troubleSections, i);
#endif
                    if(b_wantToBeShifted)
                        tmpStream->shiftStream(i);
                    b_canBeShifted = true;
                    break;
                }
                ++i;
            }
            //[!1]

            //если поток можно сдвинуть, сдвигаем его и продолжаем работу над следующим
            if(b_canBeShifted && b_wantToBeShifted) {
                clearFilters();
                fuckedUpSections.clear();
                qDebug() << QString::fromUtf8("Заявка спланирована (сдвинута). Вид перевозок = %1, Код получателя = %2, Поток = %3")
                            .arg(tmpStream->m_sourceRequest->VP)
                            .arg(tmpStream->m_sourceRequest->KP)
                            .arg(tmpStream->m_sourceRequest->NP);
                return tmpStream;
            }
            //иначе проверяем в чём проблема
            else {
                //если есть проблемные участки, добавляем самый проблемный в фильтр
                if(!troubleSections.isEmpty()) {
                    //если проблемный участок - начальный или конечный, удалим от греха подальше
                    if(troubleSections.contains(tmpStream->m_passedSections.first())) {
                        troubleSections.removeAll(tmpStream->m_passedSections.first());
                    }
                    if(troubleSections.contains(tmpStream->m_passedSections.last())) {
                        troubleSections.removeAll(tmpStream->m_passedSections.last());
                    }

                    Section *mostTroubleSection = findMostTroubleSection(troubleSections.toVector());
                    if(mostTroubleSection) {
                        qDebug() << QString("Наиболее проблемный участок: %1").arg(*mostTroubleSection);
                        fuckedUpSections.append(mostTroubleSection);
                        planStream(r, loadingPossibility, passingPossibility);
                    }
                    else {
                        if(errorString) {
                            *errorString = QString::fromUtf8("Поток №%1. Наиболее проблемный участок в начале или конце маршрута. Планирование невозможно.")
                                    .arg(tmpStream->m_sourceRequest->NP);
                            qDebug() << *errorString;
                        }
                        clearFilters();
                        fuckedUpSections.clear();
                        delete tmpStream;
                        return NULL;
                    }
                }
                //если же проблемных участков нет - дело в том, что потоку не хватает времени для того
                //чтобы пройти маршрут (даже с учётом сдвига)
                //такой поток не может быть спланирован
                else {
                    if(errorString) {
                        *errorString = QString::fromUtf8("Заявка НЕ спланирована (потоку не хватает времени для того, чтобы пройти маршрут). Вид перевозок = %1, Код получателя = %2, Поток = %3")
                                .arg(tmpStream->m_sourceRequest->VP)
                                .arg(tmpStream->m_sourceRequest->KP)
                                .arg(tmpStream->m_sourceRequest->NP);
                        qDebug() << *errorString;
                    }
                    clearFilters();
                    fuckedUpSections.clear();
                    delete tmpStream;
                    return NULL;
                }
            }
            break;
        }
        //[!1]-------------------------------------------------------------------------------------------------
        //[2]--------------------------------------------------------------------------------------------------
        case 2:
        {
            clearFilters();
            fuckedUpSections.clear();
            tmpStream->passSections(tmpStream->m_passedSections, tmpStream->m_busyPassingPossibilities);
            qDebug() << QString::fromUtf8("Заявка спланирована. Вид перевозок = %1, Код получателя = %2, Поток = %3")
                        .arg(tmpStream->m_sourceRequest->VP)
                        .arg(tmpStream->m_sourceRequest->KP)
                        .arg(tmpStream->m_sourceRequest->NP);
            return tmpStream;
        }
        //[!2]-------------------------------------------------------------------------------------------------
        }
    }
    qDebug() << QString::fromUtf8("Заявка спланирована (БУЗ). Вид перевозок = %1, Код получателя = %2, Поток = %3")
                .arg(tmpStream->m_sourceRequest->VP)
                .arg(tmpStream->m_sourceRequest->KP)
                .arg(tmpStream->m_sourceRequest->NP);
    return tmpStream;
}

int Graph::distanceTillStation(int stationIndexInPassedStations, const QVector<Station*> &_marshrut)
{
    int l = distanceBetweenStations(0, stationIndexInPassedStations, _marshrut);
    return l;
}

int Graph::distanceBetweenStations(int sourceIndex, int destinationIndex, QVector<Station*> _marshrut)
{
    int distance = 0;

    if(sourceIndex == destinationIndex) return 0;
    if(sourceIndex > destinationIndex) {
        qDebug() << QString::fromUtf8("Нельзя рассчитать расстояние между станциями %1 и %2, т.к. они идут в обратном порядке маршрута")
                    .arg(_marshrut[sourceIndex]->name)
                    .arg(_marshrut[destinationIndex]->name);
        exit(5);
    }

    for(int i = sourceIndex; i < destinationIndex; i++)
    {
        Station *stCur = _marshrut[i];
        Station *stNext = _marshrut[i + 1];
        //[1]если обе станции являются неопорными
        if((stCur->type == STATION_NOT_BEARING) && (stNext->type == STATION_NOT_BEARING)) {
            //[1.1]если они принадлежат одному участку
            if((stCur->startNumber == stNext->startNumber) && (stCur->endNumber == stNext->endNumber)) {
                int uchDist = stCur->distanceTillEnd + stCur->distanceTillStart;
                if(stCur->distanceTillStart < stNext->distanceTillStart) {
                    distance += uchDist - (stCur->distanceTillStart + stNext->distanceTillEnd);
                }
                else {
                    distance += uchDist - (stNext->distanceTillStart + stCur->distanceTillEnd);
                }
            }
            //[!1.1]
            //[1.2]если принадлежат смежным участкам
            else {
                if(stCur->endNumber == stNext->startNumber) {
                    distance += stCur->distanceTillEnd + stNext->distanceTillStart;
                }
                else if(stCur->startNumber == stNext->endNumber) {
                    distance += stCur->distanceTillStart + stNext->distanceTillEnd;
                }
            //[!1.2]
                else {
                    //2 неопорные станции не принадлежат смежному участку
                    qDebug() << QString::fromUtf8("Две неопорные станции (%1, %2) идущие друг за другом не лежат на смежных участках: "
                                                  "проблема функции построения оптимального маршрута (искать там)")
                                .arg(stCur->name)
                                .arg(stNext->name);
                    exit(7);
                }
            }
        }
        //[!1]
        //[2]если первая станция - неопорная, а вторая - опорная
        else if((stCur->type == STATION_NOT_BEARING) && (stNext->type != STATION_NOT_BEARING)) {
            if(stCur->endNumber == stNext->number) {
                distance += stCur->distanceTillEnd;
            }
            else if(stCur->startNumber == stNext->number) {
                distance += stCur->distanceTillStart;
            }
            else {
                qDebug() << "distanceBetweenStations error";
                exit(8);
            }
        }
        //[!2]

        //[3]
        else if((stCur->type != STATION_NOT_BEARING) && (stNext->type == STATION_NOT_BEARING)) {
            if(stNext->startNumber == stCur->number) {
                distance += stNext->distanceTillStart;
            }
            else if(stNext->endNumber == stCur->number) {
                distance += stNext->distanceTillEnd;
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

            distance += MyDB::instance()->sectionByNumbers(stCur->number, stNext->number)->distance;
        }
        //[!4]
    }
    return distance;
}

void Graph::clearFilters()
{
    filterEdge->clearFilter();
    filterVertex->clearFilter();
}

void Graph::addStationToFilter(Station *st)
{
    filterVertex->addStation(st);
}

void Graph::addSectionToFilter(Section *sec)
{
    filterEdge->addSection(sec);
}

bool Graph::optimalPath(int st1, int st2, QVector<Station*> *passedStations, QVector<Section*> fuckedUpSections,
                        bool loadingPossibility, bool passingPossibility)
{
    //[0]заполняем станции
    if(st1 == st2) {
        qDebug() << QString::fromUtf8("Расчитывается маршрут, где станция назначения равна станции отправления. Такого быть не должно =)");
        return false;
        passedStations->append(MyDB::instance()->stationByNumber(st1));
        return true;
    }

    //заполняем структуры станций погрузки и выгрузки - они нам понадобятся при планировании
    //---------------------------------------------------------------------------------------------------
    Station *SP, *SV;//станции погрузки и выгрузки из структуры заявок

    SP = MyDB::instance()->stationByNumber(st1);
    SV = MyDB::instance()->stationByNumber(st2);
    if(!SP || !SV)
        return false;

    //---------------------------------------------------------------------------------------------------------
    QList<Station*> startStations;
    QList<Station*> endStations;

    //если станции погрузки или выгрузки не являются опорными
    //проверяем, лежат ли они на одном участке
    //если так, дийкстра нам не нужен
    //---------------------------------------------------------------------------------------------------------
    if(SP->type == STATION_NOT_BEARING) {
        //[3-4]
        //если обе станции находятся на одном участке - возвращаем
        //маршрут из этих двух станций
        if(SV->type == STATION_NOT_BEARING) {
            if((SP->startNumber == SV->startNumber)&&(SP->endNumber == SV->endNumber)) //[4]
            {
                //возвращаем маршрут из двух станций (SP, SV)
                passedStations->append(SP);
                passedStations->append(SV);
                return true;
            }
        }
        else {
            if((SP->startNumber == SV->number) || (SP->endNumber == SV->number)) //[3]
            {
                //возвращаем маршрут из двух станций (SP, SV)
                passedStations->append(SP);
                passedStations->append(SV);
                return true;
            }
        }
        //[!3-4]
        Station *SP_start, *SP_end;
        SP_start = MyDB::instance()->stationByNumber(SP->startNumber);
        SP_end = MyDB::instance()->stationByNumber(SP->endNumber);
        if(SP_start)
            startStations.append(SP_start);
        if(SP_end)
            startStations.append(SP_end);
    }
    else {
        if(SP)
            startStations.append(SP);
    }
    if(SV->type == STATION_NOT_BEARING) {
        //[3]
        //если обе станции находятся на одном участке - возвращаем
        //маршрут из этих двух станций
        if((SV->startNumber == SP->number) || (SV->endNumber == SP->number)) //[3]
        {
            //возвращаем маршрут из двух станций (SP, SV)
            passedStations->append(SP);
            passedStations->append(SV);
            return true;
        }
        //[!3]
        Station *SV_start, *SV_end;
        SV_start = MyDB::instance()->stationByNumber(SV->startNumber);
        SV_end = MyDB::instance()->stationByNumber(SV->endNumber);
        if(SV_start)
            endStations.append(SV_start);
        if(SV_end)
            endStations.append(SV_end);
    }
    else {
        if(SV)
            endStations.append(SV);
    }

    //---------------------------------------------------------------------------------------------------------
    //[!0]
    //заполняем списки маршрутов опорными станциями
    QVector<QVector<Station*> > paths;
    for(int i = 0; i < startStations.size(); i++) {
        for(int j = 0; j < endStations.size(); j++) {
            //[!1]
            //заполняем вектор станций маршрута для возврата из функции
            QVector<Station*> stationList = dijkstraPath(startStations.at(i)->number, endStations.at(j)->number,
                                                         fuckedUpSections, loadingPossibility, passingPossibility);
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
    foreach (QVector<Station*> stList, paths) {
        int dist = distanceBetweenStations(0, stList.size() - 1, stList);
        lengths.append(dist);
    }

//    qDebug() << QString::fromUtf8("Lengths are: %1")
//                .arg(lengths);

    //смотрим, у какого из маршрутов меньше длина (ищем индекс)
    int min_index = 0;
    for(int i = 0; i < lengths.count(); i++)
    {
        if(lengths.at(i) < lengths.at(min_index))
            min_index = i;
    }

    //добавляем в конец найденные станции маршрута
    foreach (Station *st, paths.at(min_index)) {
        passedStations->append(st);
    }

    return true;
}

bool Graph::optimalPathWithOM(int st1, int st2, QList<int> OM, QVector<Station*> *passedStations,
                              QVector<Section*> fuckedUpSections, bool loadingPossibility, bool passingPossibility)
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

QVector<Station*> Graph::dijkstraPath(int st1, int st2,
                                      const QVector<Section *> &fuckedUpSections, bool loadingPossibility, bool passingPossibility)
{
    foreach (Section *sec, fuckedUpSections) {
        addSectionToFilter(sec);
    }
    boost::filtered_graph <graph_t, FilterEdge, FilterVertex> fg(g, *filterEdge, *filterVertex);

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
        Station *st = MyDB::instance()->stationByNumber(st1);
        QVector<Station*> sts;
        sts.append(st);
        return sts;
    }

    //выбор алгоритма в зависимости от параметров планирования
    //----------------------------------------------------------------------------------------------------
    if(loadingPossibility && passingPossibility) {
        boost::dijkstra_shortest_paths(fg, v1, boost::weight_map(get(&Section::time, g))
                .distance_map(boost::make_iterator_property_map(d.begin(), get(boost::vertex_index, g)))
                .predecessor_map(boost::make_iterator_property_map(p.begin(), get(boost::vertex_index, g))));
    }
    else {
        boost::dijkstra_shortest_paths(g, v1, boost::weight_map(get(&Section::time, g))
                .distance_map(boost::make_iterator_property_map(d.begin(), get(boost::vertex_index, g)))
                .predecessor_map(boost::make_iterator_property_map(p.begin(), get(boost::vertex_index, g))));
    }
    //----------------------------------------------------------------------------------------------------

    int i = nodes.indexOf(v2);

    boost::graph_traits<graph_t>::vertex_descriptor u = nodes[i];

    QList<v> path;
    QList<int> resultStationsNumbers;
    QVector<Station*> stations;

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
        Station *st = MyDB::instance()->stationByNumber(n);
        stations.append(st);
    }
    return stations;
}

Station* Graph::nearestStation(int srcSt)
{
    if(edges.isEmpty())
        return NULL;
    QVector<int> d(boost::num_vertices(g));
    //[1]рассчитываем маршрут
    //ищем вершины соответствующие станциям погрузки и выгрузки - они понадобятся при расчёте оптимального пути
    //---------------------------------------------------------------------------------------------------------
    v vSrc;
    foreach (v tmp, nodes) {
        if(g[tmp].number == srcSt) {
            vSrc = tmp;
            break;
        }
    }
    //----------------------------------------------------------------------------------------------------
    boost::dijkstra_shortest_paths(g, vSrc, boost::weight_map(get(&Section::time, g))
            .distance_map(boost::make_iterator_property_map(d.begin(), get(boost::vertex_index, g))));
    //----------------------------------------------------------------------------------------------------

    //находим индекс с минимальным расстоянием
    int i_min = 0;
    for(int i = 0; i < d.count(); i++) {
        if(d[i] < d[i_min])
            i_min = i;
    }

    //находим номер станции
    int nearestStNumber = g[nodes[i_min]].number;
    Station *nearestSt = MyDB::instance()->stationByNumber(nearestStNumber);
    return nearestSt;
}

Section* Graph::findMostTroubleSection(QVector<Section*> troubleSections)
{
    if(troubleSections.isEmpty()) return NULL;
    QMap<Section*, int> troubleMap;
    foreach (Section *s, troubleSections) {
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
    Section *sec = troubleMap.key(max);

    return sec;
}

int Graph::waitForRespond(Stream* stream, QList<Section *> troubleSections, int hours)
{
    QStringList strPassedStations;//пройденные станции маршрута
    QList<int> troubleStationsNumbers;//номера проблемных станций

    //выбираем станции, проезд по которым затруднен, чтобы выделить их красным цветом
    foreach (Section *sec, troubleSections) {
        if(!troubleStationsNumbers.contains(sec->stationNumber1))
            troubleStationsNumbers.append(sec->stationNumber1);
        if(!troubleStationsNumbers.contains(sec->stationNumber2))
            troubleStationsNumbers.append(sec->stationNumber2);
    }

    //собственно выделяем
    foreach (Station *st, stream->m_passedStations) {
            if(troubleStationsNumbers.contains(st->number)) {
                QString strTroubleStation = *st;
                strTroubleStation.prepend("<font color='red'>");
                strTroubleStation.append("</font>");
                strPassedStations.append(strTroubleStation);
            }
            else {
                strPassedStations.append(*st);
            }
    }

    Pauser *pauser = new Pauser();
    connect(this, SIGNAL(signalOffsetAccepted(bool)), pauser, SLOT(accept(bool)));
    QString msg = QString("OFFSET_STREAM(%1,%2,%3,%4)")
            .arg(strPassedStations.join(';'))
            .arg((QString)stream->m_departureTime)
            .arg(0)
            .arg(hours);

    emit signalGraph(msg);
    qDebug() << QString::fromUtf8("Entering pauser loop");
    int answer = pauser->exec();
    if(answer == 1) {
        qDebug() << QString::fromUtf8("Offset accepted by client");
    }
    else if(answer == 0) {
        qDebug() << QString::fromUtf8("Offset denied by client");
    }
    delete pauser;
    return answer;
}
