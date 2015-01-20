#include "stream.h"
#include "request.h"
#include "mydb.h"
#include "graph.h"
#include <QDebug>

Stream::Stream(): m_planned(false), m_departureTime(MyTime(0,0,0)), m_failed(false) {}

Stream::Stream(Request* request, Graph *gr): m_sourceRequest(request), m_graph(gr), m_planned(false), m_departureTime(MyTime(request->DG - 1, request->CG, 0)), m_failed(false) {}

QVector< QVector<int> > Stream::calculatePV(const QList <echelon> &echelones)
{
    QVector< QVector<int> > tmpBusyPassingPossibilities;
    if(!tmpBusyPassingPossibilities.isEmpty()) tmpBusyPassingPossibilities.clear();

    int c = m_passedStations.count() - 1;
    tmpBusyPassingPossibilities.resize(c);
    for(int i = 0; i < c; i++)
        tmpBusyPassingPossibilities[i].resize(60);
    //темп рассчётный = MIN(пропускные возможности участков на дни их занятости с темпом заданным в заявке)
    if(echelones.isEmpty()) {
        qDebug() << "Нельзя рассчитать занятую пропускную возможность для потока, когда эшелоны в нём не заполнены";
        return tmpBusyPassingPossibilities;
    }
    for(int i = 0; i < echelones.count(); i++) {
        for (int j = 1; j < echelones[i].timesArrivalToStations.count(); j++) {
            MyTime t = echelones[i].timesArrivalToStations[j];//t - время прибытия на j-ую станцию i-го поезда
            MyTime t_prev = echelones[i].timesArrivalToStations[j-1];//t_prev - время прибытия на (j-1)-ую станцию i-го поезда
            for(int k = 0; k < 60; k++) {
                if(k == t_prev.days()) //заполняет только на въезд
                    tmpBusyPassingPossibilities[j-1][k] += 1;
            }
        }
    }
    return tmpBusyPassingPossibilities;
}

void Stream::fillSections()
{
    if(!m_passedSections.isEmpty()) m_passedSections.clear();

    for(int i = 0; i < m_passedStations.count(); i++) {
        if(m_passedStations[i] == m_passedStations.last()) return;
        section s;
        s = MyDB::instance()->sectionByStations(m_passedStations[i], m_passedStations[i+1]);
        if(!((s.stationNumber1 == 0 ) && (s.stationNumber2)))
            m_passedSections.append(s);
    }
 }

//может ли поток пройти участки маршрута (0 - не может пройти и нельзя сместить; 1 - не может пройти но можно сместить; 2 - может пройти)
int Stream::canPassSections(const QList<section> &passedSections, const QVector< QVector<int> > &busyPassingPossibilities, MyTime timeOffset, QList<section> *fuckedUpSections)
{
    MyTime offsettedStartTime = m_departureTime + timeOffset;    //сдвинутое время начала перевозок
    MyTime offsettedFinishTime = m_arrivalTime + timeOffset;     //сдвинутое время окончания перевозок

    if(offsettedStartTime.toMinutes() < 0) return 1;
    if(offsettedFinishTime.toMinutes() >= 60 * 24 * 60) return 1;

    qDebug() << QString::fromUtf8("Смещённое время отправления: день:%1 час:%2").arg(offsettedStartTime.days()).arg(offsettedStartTime.hours());
    qDebug() << QString::fromUtf8("Смещённое время прибыия: день:%1 час:%2").arg(offsettedFinishTime.days()).arg(offsettedFinishTime.hours());

    bool can = 2;
    for(int i = 0; i < passedSections.count(); i++) {
        //проверка, хватит ли пропускной способности
        int max = busyPassingPossibilities[i][0];
        for(int it = 0; it < busyPassingPossibilities[i].count(); it++)
            if(busyPassingPossibilities[i][it] > max)
                max = busyPassingPossibilities[i][it];
        if(passedSections[i].ps < max) {
            //сдвиг не возможен
            station st1 = MyDB::instance()->stationByNumber(passedSections[i].stationNumber1);
            station st2 = MyDB::instance()->stationByNumber(passedSections[i].stationNumber2);
            qDebug() << QString::fromUtf8("Пропускная способность участка %1 - %2 меньше заданного темпа.")
                        .arg(st1.name)
                        .arg(st2.name);
            fuckedUpSections->append(passedSections[i]);
            return 0;
        }
        //------------------------------------------

        int k = offsettedStartTime.days();  //итератор по всем дням
        for(int j = offsettedStartTime.days(); j <= offsettedFinishTime.days(); j++) {
            //на 60-ый день ничего не проверяем, иначе - выход за пределы массива
            assert(j<60);
            if(passedSections[i].passingPossibilities[j] < busyPassingPossibilities[i][k])
            {
                section sec = passedSections[i];
                station st1 = MyDB::instance()->stationByNumber(sec.stationNumber1),
                        st2 = MyDB::instance()->stationByNumber(sec.stationNumber2);
                QString strSection = QString::fromUtf8("%1 (%2) - %3 (%4)")
                        .arg(st1.name)
                        .arg(st1.number)
                        .arg(st2.name)
                        .arg(st2.number);
                qDebug() << QString::fromUtf8("пропускная возможность участка %1 на %2 день = %3 , а количество проходящих в этот день поездов по участку в потоке %4 = %5")
                            .arg(strSection)
                            .arg(j+1)
                            .arg(passedSections[i].passingPossibilities[j])
                            .arg(m_sourceRequest->NP)
                            .arg(busyPassingPossibilities[i][k]);
                if(fuckedUpSections != 0) {
                    fuckedUpSections->append(passedSections[i]);
                }
                can = 1;
            }
            k++;
        }
    }
    qDebug() << "\n";
    return can;
}

bool Stream::canBeShifted(int days, int hours, int minutes, QList<section> *fuckedUpSections = NULL)
{
    //чтобы проверить, может ли поток быть сдвинут на days дней вперёд или назад, необходимо, чтобы выполнились следующие условия:
    //1)от времени прибытия на каждую станцию отнять время, идущее в параметре функции
    //2)перерасчитать пропусную возможность по дням для данного маршрута
    //3)посмотреть, сможет ли пройти маршрут по участкам с новым временем прибытия (функция bool Route::canPassSections(NULL))
    if(m_failed) return false;

    MyTime requestDepartureTime = MyTime(m_sourceRequest->DG - 1, m_sourceRequest->CG, 0);
    MyTime offset(days, hours, minutes);

    QVector< QVector<int> > tmpBusyPassingPossibilities;    //перерасчитанная занятость участков
    QList<float> distances = distancesBetweenStations();
    QList<int> sectionSpeeds;
    foreach (section sec, m_passedSections) {
        sectionSpeeds.append(sec.speed);
    }
    QList< echelon > tmpEchelones = fillEchelones(requestDepartureTime + offset, m_sourceRequest->VP, m_sourceRequest->PK, m_sourceRequest->TZ, distances, sectionSpeeds);          //делаем копию эшелонов, т.к. будем их менять

    tmpBusyPassingPossibilities = calculatePV(tmpEchelones);
    int res = canPassSections(m_passedSections, tmpBusyPassingPossibilities, MyTime(days, hours, minutes), fuckedUpSections);
    if(res == 0)
        return false;
    else if(res == 1)
        return false;
    else if(res == 2)
        return true;
}

bool Stream::canBeShifted(const MyTime &offsetTime, QList<section> *fuckedUpSections = NULL)
{
    return canBeShifted(offsetTime.days(), offsetTime.hours(), offsetTime.minutes(), fuckedUpSections);
}

void Stream::shiftStream(int days, int hours)
{
    MyTime requestDepartureTime = MyTime(m_sourceRequest->DG - 1, m_sourceRequest->CG, 0);
    MyTime offset(days, hours, 0);

    QList<float> distances = distancesBetweenStations();
    QList<int> sectionSpeeds;
    foreach (section sec, m_passedSections) {
        sectionSpeeds.append(sec.speed);
    }
    m_echelones = fillEchelones(requestDepartureTime + offset, m_sourceRequest->VP, m_sourceRequest->PK, m_sourceRequest->TZ, distances, sectionSpeeds);          //делаем копию эшелонов, т.к. будем их менять
    m_busyPassingPossibilities = calculatePV(m_echelones);
    qDebug() << QString::fromUtf8("Поток №%1 сдвинут на %2ч.\n")
                .arg(m_sourceRequest->NP)
                .arg(offset.toHours());
}

void Stream::shiftStream(const MyTime &offsetTime)
{
    shiftStream(offsetTime.days(), offsetTime.hours());
}

int Stream::length()
{
    if(!m_passedSections.isEmpty())
        return m_graph->distanceBetweenStations(0, m_passedStations.count() - 1, m_passedStations);
    else
        return -1;
}

QString Stream::print(bool b_PSInfo/*=false*/, bool b_RouteInfo/*=true*/,
                      bool b_BusyPossibilities/*=false*/, bool b_echelonsTimes/*=false*/)
{
    if(m_failed) {
        return QString::fromUtf8("Поток №%1 не спланирован. Причина: %2").arg(m_sourceRequest->NP).arg(m_failString);
    }

    QString str;
    str += *m_sourceRequest;

    //Подвижной состав
    if(b_PSInfo) {
        str += QString::fromUtf8("Общая ПС: %1\n").arg(m_sourceRequest->ps.getString());
        foreach (echelon tmpEch, m_echelones) {
            str += QString::fromUtf8("Эшелон №%1: %2\n")
                    .arg(tmpEch.number + 1)
                    .arg(tmpEch.ps.getString());
        }
        str += QString::fromUtf8("\nВремя отправление первого эшелона потока: %1").arg(m_echelones.first().timeDeparture);
        str += QString::fromUtf8("\nВремя прибытия первого эшелона потока: %1").arg(m_echelones.first().timeArrival);
        str += QString::fromUtf8("\nВремя отправления последнего эшелона потока: %1").arg(m_echelones.last().timeDeparture);
        str += QString::fromUtf8("\nВремя прибытия последнего эшелона потока: %1").arg(m_echelones.last().timeArrival);
    }

    //Информация о погрузке
    station sp = MyDB::instance()->stationByNumber(m_sourceRequest->SP);
    pvr p = MyDB::instance()->PVRByNumber(sp.pvrNumber);
    QString strDayLoads;
    foreach (int day, m_busyLoadingPossibilities.keys()) {
        strDayLoads += QString::fromUtf8("[%1/%2] ")
                .arg(day + 1)
                .arg(m_busyLoadingPossibilities.value(day));
    }
    switch(m_loadType) {
    case 0:
        str += QString::fromUtf8("\nЗаявка не погружена");
        break;
    case 1:
        str += QString::fromUtf8("\nЗаявка погружена на станции %1. [день погрузки/количество поездов]: %2")
                .arg(sp)
                .arg(strDayLoads);
        break;
    case 2:
        str += QString::fromUtf8("\nЗаявка погружена на станции %1 и ПВР %2. [день погрузки/количество поездов]: %3")
                .arg(sp)
                .arg(p)
                .arg(strDayLoads);
        break;
    default:
    {}
    }

    //Информация о маршруте
    if(b_RouteInfo) {
        str += QString::fromUtf8("\nДлина маршрута = %1 км").arg(m_graph->distanceTillStation(m_passedStations.count() - 1, m_passedStations));
        str += QString::fromUtf8("\n\nМаршрут потока: ");
        foreach (station tmpSt, m_passedStations) {
            str += QString::fromUtf8("%1(%2)  -  ")
                    .arg(tmpSt.name)
                    .arg(tmpSt.number);
        }
        str.chop(5);
    }

    //Занятость участков по дням
    if(b_BusyPossibilities) {
        str += QString::fromUtf8("\nЗанятость участков по дням:");
        for(int j = 0; j < m_passedSections.count(); j++) {
            str += QString::fromUtf8("\n%1 - %2 (Проп. сп-ть = %3): \n")
                    .arg(MyDB::instance()->stationByNumber(m_passedSections[j].stationNumber1).name)
                    .arg(MyDB::instance()->stationByNumber(m_passedSections[j].stationNumber2).name)
                    .arg(m_passedSections[j].ps);
            for(int k = 0; k < m_busyPassingPossibilities[j].count(); k++) {
                if(m_busyPassingPossibilities[j][k] != 0)
                    str += QString::fromUtf8("%1 день: %2/%3, ")
                            .arg(k+1)
                            .arg(m_busyPassingPossibilities[j][k])
                            .arg(m_passedSections[j].passingPossibilities[k]);
            }
            str.chop(2);
        }
    }

    //Эшелоны
    if(b_echelonsTimes) {
        foreach (echelon ech, m_echelones) {
            str += ech.getString();
        }
    }

    str += "\n\n";
    return str;
}

void Stream::setFailed(QString errorString)
{
    m_failed = true;
    m_planned = false;
    m_failString = errorString;
    //очищаем всю информацию
    m_passedStations.clear();
    m_passedSections.clear();
    m_echelones.clear();
    m_departureTime = MyTime(0, 0, 0);
    m_arrivalTime = MyTime(0, 0, 0);
}

QList<float> Stream::distancesBetweenStations()
{
    QList<float> dists;
    if(m_passedStations.isEmpty()) {
        qDebug() << "Route::distancesTillStations: no stations";
        return dists;
    }

    for(int i = 1; i < m_passedStations.count(); i++) {
        float dist = m_graph->distanceBetweenStations(i - 1, i, m_passedStations);
        dists.append(dist);
    }
    return dists;
}

QList<echelon> Stream::fillEchelones(const MyTime departureTime, int VP, int PK, int TZ,
                                     const QList<float> &distancesBetweenStations, const QList<int> &sectionsSpeed)
{
    QList<echelon> echs;
    QVector <int> sectionSpeedVector = sectionsSpeed.toVector();

    //если PK = 0, TZ = 0 - скорость V/2, БУЗ
    if((TZ == 0) && (PK == 0)) {
        for(int i = 0; i < sectionSpeedVector.count(); i++) {
            sectionSpeedVector[i] /= 2;
        }
        PK = 1;
        TZ = 1;
    }

    //если PK = 1, TZ = 0 - скорость V, БУЗ, но с учётом времени работы в ППР (мы не обрабатываем этот вариант)
    else if(TZ == 0) TZ = 1;

    QList<PS> ps_list = dividePS(*m_sourceRequest);
    QStringList NA_list = divideNA(*m_sourceRequest);
    int delayBetweenTemp = 0;
    if(VP == 24) {
        //исправить. в БД имеет значение int (соотв при ТЗ от 0 до 9 будет ошибка)
        delayBetweenTemp = QString::number(TZ).left(1).toInt() * 24;
        TZ = QString::number(TZ).right(1).toInt();
    }
    int delay = 24.0 / TZ;

    //для 24 вида перевозок TZ представляет собой следующее:
    //TZ == xy
    //x - интервал в днях между отправлениями TZ-эшелонами в днях
    //y - темп заданный

    for(int i = 0; i < PK; i++) {
        //если i-ый эшелон кратен темпу перевозки, добавлять разницу во времени отправления к следующему эшелону
        echelon ech(i);

        //время отправления каждого эшелона задерживается на величину = 24 / TZ * № эшелона
        ech.timeDeparture = departureTime + MyTime::timeFromHours(i * delay);
        //также необходимо отнять сутки от времени отправления, т.к. отправление в первый день - это нулевой день по факту
        if(departureTime.days() > 0) {
            ech.timeDeparture = ech.timeDeparture;
        }
        else {
            qDebug() << "День готовности к отправлению меньше единицы - ОШИБКА";
        }
        //время прибытия на первую станцию = времени отправления
        ech.timesArrivalToStations.append(ech.timeDeparture);


        int j = 0;
        MyTime  elapsedTime = ech.timeDeparture; //время прибытия к (j+1) станции
        foreach (float dist, distancesBetweenStations) {
            //расчёт времени въезда каждого эшелона на очередную станцию маршрута
            //должен осуществляться относительно времени доезда на прошлую станцию
            double hours = (dist * 24.0) / sectionSpeedVector[j]; //часов от прошлой до текущей станции
            //если число часов содержит дробную часть, откидываем её и добавляем целый час
            if(hours > (int)hours)
                elapsedTime = elapsedTime + MyTime::timeFromHours(hours + 1);
            else
                elapsedTime = elapsedTime + MyTime::timeFromHours(hours);
            //тут мы считаем задержку между отправлениями поездов для 24 вида перевозок
            //она будет равна TZ * 24 часов
            //если текущий номер поезда кратен темпу, добавляем задержку в TZ * 24 часа
            //и не забываем отнять обычную задержку между эшелонами
            if((VP == 24)&&(i!=0)&&(i%TZ==0)) elapsedTime = elapsedTime + MyTime::timeFromHours(delayBetweenTemp - delay);
            ech.timesArrivalToStations.append(elapsedTime);
            j++;
        }
        ech.timeArrival = ech.timesArrivalToStations.last();
        //распределение ПС по поездам
        ech.ps = ps_list.first();
        ps_list.pop_front();
        //распределение NA по поездам
        ech.NA = NA_list.first();
        NA_list.pop_front();

        echs.append(ech);
    }
    return echs;
}

QList<echelon> Stream::fillEchelonesInMinutes(const MyTime departureTime, int VP, int PK, int TZ,
                                     const QList<float> &distancesBetweenStations, const QList<int> &sectionsSpeed)
{
    QList<echelon> echs;
    QVector <int> sectionSpeedVector = sectionsSpeed.toVector();

    //если PK = 0, TZ = 0 - скорость V/2, БУЗ
    if((TZ == 0) && (PK == 0)) {
        for(int i = 0; i < sectionSpeedVector.count(); i++) {
            sectionSpeedVector[i] /= 2;
        }
        PK = 1;
        TZ = 1;
    }

    //если PK = 1, TZ = 0 - скорость V, БУЗ, но с учётом времени работы в ППР (мы не обрабатываем этот вариант)
    else if(TZ == 0) TZ = 1;

    QList<PS> ps_list = dividePS(*m_sourceRequest);
    QStringList NA_list = divideNA(*m_sourceRequest);

    int delayBetweenTemp = 0;
    if(VP == 24) {
        //исправить. в БД имеет значение int (соотв при ТЗ от 0 до 9 будет ошибка)
        delayBetweenTemp = QString::number(TZ).left(1).toInt() * 24 * 60;
        TZ = QString::number(TZ).right(1).toInt();
    }
    int delay = 24.0 * 60.0 / TZ;

    //для 24 вида перевозок TZ представляет собой следующее:
    //TZ == xy
    //x - интервал в днях между отправлениями TZ-эшелонами в днях
    //y - темп заданный

    for(int i = 0; i < PK; i++) {
        //если i-ый эшелон кратен темпу перевозки, добавлять разницу во времени отправления к следующему эшелону
        echelon ech(i);

        //время отправления каждого эшелона задерживается на величину = 24 / TZ * № эшелона
        //также необходимо отнять сутки от времени отправления, т.к. отправление в первый день - это нулевой день по факту
        ech.timeDeparture = departureTime + MyTime::timeFromMinutes(i * delay);
        //время прибытия на первую станцию = времени отправления
        ech.timesArrivalToStations.append(ech.timeDeparture);


        int j = 0;
        MyTime  elapsedTime = ech.timeDeparture; //время прибытия к (j+1) станции
        foreach (float dist, distancesBetweenStations) {
            //расчёт времени въезда каждого эшелона на очередную станцию маршрута
            //должен осуществляться относительно времени доезда на прошлую станцию
            int minutes = (int)((dist * 24 * 60) / sectionSpeedVector[j]); //минут от прошлой до текущей станции
            elapsedTime = elapsedTime + MyTime::timeFromMinutes(minutes); //раскомментировать, если надо считать с точностью до минут
            //тут мы считаем задержку между отправлениями поездов для 24 вида перевозок
            //она будет равна TZ * 24 часов
            //если текущий номер поезда кратен темпу, добавляем задержку в TZ * 24 часа
            //и не забываем отнять обычную задержку между эшелонами
            if((VP == 24)&&(i!=0)&&(i%TZ==0)) elapsedTime = elapsedTime + MyTime::timeFromMinutes(delayBetweenTemp - delay);
            ech.timesArrivalToStations.append(elapsedTime);
            j++;
        }
        ech.timeArrival = ech.timesArrivalToStations.last();
        //распределение ПС по поездам
        ech.ps = ps_list.first();
        ps_list.pop_front();
        //распределение NA по поездам
        ech.NA = NA_list.first();
        NA_list.pop_front();

        echs.append(ech);
    }
    m_echelones = echs;
    return echs;
}


QList<PS> Stream::dividePS(const Request &req)
{
    PS srcPS = req.ps;
    int PK = req.PK;
    int VP = req.VP;
//    qDebug() << QString::fromUtf8("ОБЩАЯ ПС: %1").arg(srcPS.getString());
    QList<int> psTotal_l;
    QList<PS> psList;
    psTotal_l.append(srcPS.cist);
    psTotal_l.append(srcPS.krit);
    psTotal_l.append(srcPS.kuhn);
    psTotal_l.append(srcPS.ledn);
    psTotal_l.append(srcPS.luds);
    psTotal_l.append(srcPS.pass);
    psTotal_l.append(srcPS.plat);
    psTotal_l.append(srcPS.polu);
    psTotal_l.append(srcPS.spec);
    //
    if(PK == 0) {
//        qDebug() << "Косяк при распределении ПС по поездам: Количество поездов = 0";
//        return psList;
        PK = 1;
    }
    //разбиваем общий подвижный состав по вагонам
    for(int i = 0; i < PK; i++) {
        QVector<int> ps_l;
        ps_l.resize(psTotal_l.size());
        //разбиваем
        int k = 0;//итератор по номерам вагонов

        foreach (int tmp, psTotal_l) {
            if(tmp == 0) {++k; continue;}
            //[1]
            if(tmp < PK)
            {
                if((i + 1) <= (tmp % PK))
                    ps_l[k] = 1;
            }
            //[!1]
            //[2]
            else if(tmp % PK == 0)
            {
                ps_l[k] = tmp / PK;
            }
            //[!2]
            //[3]
            else if((tmp > PK)&&(tmp % PK != 0))
            {
                ps_l[k] = tmp / PK;
                if((i + 1) <= (tmp % PK)) {
                    if(VP == 24)
                        ps_l[ps_l.count() - 1] += 1;
                    else
                        ps_l[k] += 1;
                }
            }
            //[!3]
            k++;
        }

        PS ps;
        ps.cist = ps_l[0];
        ps.krit = ps_l[1];
        ps.kuhn = ps_l[2];
        ps.ledn = ps_l[3];
        ps.luds = ps_l[4];
        ps.pass = ps_l[5];
        ps.plat = ps_l[6];
        ps.polu = ps_l[7];
        ps.spec = ps_l[8];

        ps.total = 0;
        foreach (int tmp, ps_l) {
            ps.total += tmp;
        }
        psList.append(ps);
    }

//    int j = 0;
//    foreach (PS tmpPS, psList) {
//        qDebug() << QString::fromUtf8("Эшелон №%1: %2")
//                    .arg(j)
//                    .arg(tmpPS.getString());
//        j++;
//    }

    return psList;
}

QStringList Stream::divideNA(const Request &req)
{
    QStringList trainList;
    int PK = req.PK;
    if(PK == 0) PK = 1;
    //если код принадлежности груза = 11, значит везём порожняк
    if(req.PG.toInt() == 11) {
        for(int i = 0; i < req.PK; i++)
            trainList.append(QString::fromUtf8("ВОЗВРАТ ПОДВИЖНОГО СОСТАВА"));
        return trainList;
    }
    if(req.VP == 23) {
        for(int i = 0; i < PK; i++) {
            trainList.append(req.NA);
        }
        return trainList;
    }
    if(req.VP == 25) {
        for(int i = 0; i < PK; i++) {
            trainList.append(req.NA);
        }
        return trainList;
    }

    //строка для каждого отдельного вагона
    QString strTrain;
    //разделяем каждое наименование (чередуется через ',')
    QStringList listGoods = req.NA.split(',');
    for(int i = 0; i < PK; i++)
        trainList.append("");
    foreach (QString strGood, listGoods) {
        //строка формата 'ТОПЛИВО-2М - 200Т'
        strGood = strGood.trimmed();

        int length = strGood.length();
        //единица измерения
        int measureLength = length - strGood.lastIndexOf(QRegExp("[0-9|\\s]")) - 1;
        QString measure = strGood.right(measureLength);
        measure = measure.trimmed();
        //его количество
        QString cuttedStr = strGood.left(length - measureLength).trimmed();
        int amountIndex = cuttedStr.lastIndexOf(' ');
        int amountLength = length - measureLength - amountIndex;
        float amount = strGood.mid(amountIndex, amountLength).toFloat();
        //наименование груза
        int goodLength = strGood.indexOf(' ');
        QString strGoodName = strGood.left(goodLength + 1);
        strGoodName = strGoodName.trimmed();


        /*                        24 BИД ПEPEBOЗOK.
                                  -----------------
          ДЛЯ BCEX  BИДOB ГPУЗA CHAБЖEHЧECKИX ПEPEBOЗOK  KOЛИЧECTBO ПEPEBOЗИMOГO
        УKAЗЫBAETCЯ, KAK ПPABИЛO,  ЦEЛЫMИ ЧИCЛAMИ.  B ПPOTИBHOM  CЛУЧAE B HAИME-
        HOBAHИИ OCTAETCЯ ДPOБHAЯ BEЛИЧИHA, A  ДЛЯ ПOДCЧETA OБЩEГO KOЛИЧECTBA ПE-
        PEBOЗИMOГO B  ГPAФOПЛAHE ИCПOЛЬЗУETCЯ ЗHAЧEHИE, OKPУГЛEHHOE  ПO ПPABИЛAM
        MATEMATИKИ.
          PAЗPEШAETCЯ ПOCЛE  OБЩEГO HAИMEHOBAHИЯ И KOЛИЧECTBA  ПEPEBOЗИMOГO УKA-
        ЗЫBATЬ ДETAЛИЗAЦИЮ  ПEPEBOЗИMOГO ГPУЗA.  ДETAЛИЗAЦИЯ OT  OБЩEГO HAИMEHO-
        BAHИЯ И KOЛИЧECTBA ПEPEBOЗИMOГO OTДEЛЯETCЯ CИMBOЛOM "*"
                  ПPИMEP:
                           TOПЛИBO 100 T*AИ 93 - 50 T, AИ 76 - 50 T.
          ДЛЯ ПOTOKOB, ПOMEЧEHHЫX CИMBOЛOM '*', HEOБXOДИMO УЧИTЫBATЬ, ЧTO:
          1. KOЛИЧECTBO ПEPEBOЗИMOГO B ЗAЯBKE (BEC, УKAЗAHHЫЙ ДO "*") ДEЛИTCЯ
             HA KOЛИЧECTBO ПOEЗДOB B ПOTOKE. ЦEЛAЯ ЧACTЬ ПOЛУЧEHHOГO BECA
             ПPEДCTABЛЯET COБOЙ BEC ПEPEBOЗИMOГO ДЛЯ BCEX ПOEЗДOB, KPOME
             ПOCЛEДHEГO.
             BEC ПEPEBOЗИMOГO ПOCЛEДHИM ПOEЗДOM OПPEДEЛЯETCЯ KAK PAЗHOCTЬ MEЖДУ
             OБЩИM BECOM И BECOM, ПEPEBOЗИMЫM ПPEДЫДУЩИMИ ПOEЗДAMИ.
          2. TEKCTOBAЯ ЧACTЬ HAИMEHOBAHИЯ ПEPEBOЗИMOГO ДЛЯ BCEX OДИHOЧHЫX
             ПOEЗДOB OДHA И TA ЖE (KAK OПPEДEЛEHO B ЗAЯBKE).
          3. ДЛЯ ПOCЧETA OБЩEГO KOЛИЧECTBA ПEPEBOЗИMOГO B ГPAФOПЛAHE ДЛЯ ПOCЛEД-
             HEГO ПOEЗДA ИCПOЛЬЗУETCЯ OKPУГЛEHHAЯ BEЛИЧИHA BECA ПEPEBOЗИMOГO ИM
             ГPУЗA.
          4. TEKCTOBAЯ ЧACTЬ ФOPMИPУEMOГO HAИMEHOBAHИЯ ПEPEBOЗИMOГO, ЗAПИCAHHAЯ
             ПOCЛE "*" (BKЛЮЧAЯ И BEC), ПEPEHOCИTCЯ B ПOЛE "HAИMEHOBAHИE И
             KOЛИЧECTBO ПEPEBOЗИMOГO" ДЛЯ KAЖДOГO ПOEЗДA БEЗ ИЗMEHEHИЙ.
             ПPИMEP 1: HAИMEHOBAHИE ПEPEBOЗИMOГO:
                         БOEПPИПACЫ 1000 T
                       KOЛИЧECTBO  ПOEЗДOB:
                         20
              KAЖДOMУ TPAHCПOPTУ ПPИCBAИBAETCЯ HAИMEHOBAHИE:
              БOEПPИПACЫ 50 T
             ПPИMEP 2: HAИMEHOBAHИE ПEPEBOЗИMOГO:
                         ПPOДOBOЛЬCTBИE - 25.5T MУKA - 5T, KPУПA - 3T,
                       KOЛИЧECTBO ПOEЗДOB:
                         3
             ПEPBЫM ДBУM TPAHCПOPTAM ФOPMИPУETCЯ HAИMEHOBAHИE:
              ПPOДOBOЛЬCTBИE 8T*MУKA - 1T, KPУПA - 1T
             ДЛЯ ПOCЛEДHEГO ПOEЗДA:
              ПPOДOBOЛЬCTBИE 9.5T*MУKA - 3T, KPУПA - 1T
     */
        if(req.VP == 24) {
            QVector<float> amounts;
            //если около единицы измерения стоит *
            //делим перевозимое по вагонам
            if(measure.contains('*')) {
                //если вес не кратен количеству поездов
                if((int)(amount / PK) != (amount / PK)) {
                    int k = (int)(amount / req.PK);
                    for(int i = 0; i < req.PK; i++) {
                        amounts.append(k);
                        amount -= k;
                    }
                    //добавляем в последний поезд остаток
                    amounts[PK-1] += amount;
                }
                else {
                    for(int i = 0; i < PK; i++) {
                        amounts.append(amount / PK);
                    }
                }
            }
            //если звёздочки нет
            //оставляем значения, как есть
            else {
                for(int i = 0; i < PK; i++) {
                    amounts.append(amount);
                }
            }

            QString clearMeasure = measure.remove('*');
            for(int i = 0; i < PK; i++) {
                strTrain = QString::fromUtf8("%1 %2%3, ")
                        .arg(strGoodName)
                        .arg(amounts.at(i))
                        .arg(clearMeasure);
                trainList[i].append(strTrain);
            }
        }
        /*                           25 BИД ПEPEBOЗOK.
                                     -----------------
             ДЛЯ OБЫЧHЫX  ПOTOKOB (HE ЭЛEMEHTOB 'POCCЫПИ')  УKAЗЫBAЮTCЯ ПEPEBOЗИMЫE
           PECУPCЫ:
             - ЛЮДCKИE;
             - TEXHИKA C ЛИЧHЫM COCTABOM, COПPOBOЖДAЮЩИM ДAHHУЮ TEXHИKУ;
             - ABTOШИHЫ;
             - ЛЮБOЙ ДPУГOЙ TEKCT.
             ПEPEЧEHЬ ПOЛHЫX И COKPAЩEHHЫX HAИMEHOBAHИЙ PECУPCOB:
                OФИЦEPЫ                OФ,
                CEPЖAHTЫ И COЛДATЫ     C/C,
                ABTOMAШИHЫ             A/M,
                ПPИЦEПЫ                ПPЦ,
                TPAKTOPA               TP,
                ABTOШИHЫ             ABTOШИHЫ,
                ABTOKPAHЫ            ABTOKPAHЫ  ИЛИ  A/K,
                KPAHЫ                KPAHЫ.
             ПPИ BЫДAЧE  ГPAФOПЛAHA, B  ЗABИCИMOCTИ OT  ПEPEBOЗИMOГO, ИHФOPMAЦИЯ  O
           ПOTOKE ПOПAДAET B PAЗHЫE PAЗДEЛЫ:
             - OФИЦEPЫ, CEPЖAHTЫ И COЛДATЫ - B PAЗДEЛ "ЛЮДИ";
             - OФ, C/C, A/M, ПPЦ, TP, A/K  - B PAЗДEЛ "ABTOTPAHCПOPT C Л.C.";
             - ABTOШИHЫ                    - B PAЗДEЛ "ABTOШИHЫ".
             ECЛИ B  ЗAЯBKE УKAЗAH  ДPУГOЙ TEKCT, TO  ПPИ BЫДAЧE  ГPAФOПЛAHA ДAHHЫE
           ПOTOKИ ПEЧATAЮTCЯ B PAЗДEЛE "ПPOЧИE PECУPCЫ".
             ПPИMEP ЗAПOЛHEHИЯ ГPAФЫ '2' ДЛЯ OБЫЧHЫX ПOTOKOB:
             OФ-100, C/C-1000                 - PAЗДEЛ "ЛЮДИ";
             OФ-5,   C/C-100, A/M-100         - PAЗДEЛ "ABTOTPAHCПOPT C Л.C.";
             OФ-5,   C/C-120, A/M-100, ПPЦ-20 - PAЗДEЛ "ABTOTPAHCПOPT C Л.C.";
             ABTOШИHЫ-100                     - PAЗДEЛ "ABTOШИHЫ".
           */
//        else if(req.VP == 25) {
//            for(int i = 0; i < PK; i++) {
//                trainList.append(strGood);
//            }
//        }
    }
    for(int i = 0; i < PK; i++) {
        trainList[i].chop(2);
    }
    return trainList;
}
