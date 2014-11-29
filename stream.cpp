#include "stream.h"
#include "request.h"
#include "mydb.h"
#include "graph.h"
#include <QDebug>

Stream::Stream(): m_planned(false), m_departureTime(MyTime(0,0,0)), m_failed(false) {}

Stream::Stream(Request* request, Graph *gr): m_sourceRequest(request), m_graph(gr), m_planned(false), m_departureTime(MyTime(request->DG, request->CG, 0)), m_failed(false) {}

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
//                if((k <= t.days())&&(k >= t_prev.days())) //заполняет реально (раскомментировать при необходимости)
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

bool Stream::canBePlanned()
{
    if(m_sourceRequest->canLoad() && canPassSections(m_passedSections, m_busyPassingPossibilities))
        return true;
    return false;
}

bool Stream::canPassSections(const QList<section> &passedSections, const QVector< QVector<int> > &busyPassingPossibilities, MyTime timeOffset, QList<section> *fuckedUpSections)
{
    MyTime offsettedStartTime = m_departureTime + timeOffset;       //сдвинутое время начала перевозок
    MyTime offsettedFinishTime = m_arrivalTime + timeOffset;     //сдвинутое время окончания перевозок

    if(offsettedStartTime.toMinutes() < 0) return false;
    if(offsettedFinishTime.toMinutes() >= 60 * 24 * 60) return false;

    qDebug() << QString::fromUtf8("Смещённое время отправления: день:%1 час:%2").arg(offsettedStartTime.days()).arg(offsettedStartTime.hours());
    qDebug() << QString::fromUtf8("Смещённое время прибыия: день:%1 час:%2").arg(offsettedFinishTime.days()).arg(offsettedFinishTime.hours());

    bool can = true;
    for(int i = 0; i < passedSections.count(); i++) {
        int k = offsettedStartTime.days();  //итератор по всем дням
        for(int j = offsettedStartTime.days(); j < offsettedFinishTime.days(); j++) {
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
                can = false;
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

    MyTime requestDepartureTime = MyTime(m_sourceRequest->DG, m_sourceRequest->CG, 0);
    MyTime offset(days, hours, minutes);

    QVector< QVector<int> > tmpBusyPassingPossibilities;    //перерасчитанная занятость участков
    QList<float> distances = distancesBetweenStations();
    QList<int> sectionSpeeds;
    foreach (section sec, m_passedSections) {
        sectionSpeeds.append(sec.speed);
    }
    QList< echelon > tmpEchelones = fillEchelones(requestDepartureTime + offset, m_sourceRequest->VP, m_sourceRequest->PK, m_sourceRequest->TZ, distances, sectionSpeeds);          //делаем копию эшелонов, т.к. будем их менять

    tmpBusyPassingPossibilities = calculatePV(tmpEchelones);
    return canPassSections(m_passedSections, tmpBusyPassingPossibilities, MyTime(days, hours, minutes), fuckedUpSections);
}

bool Stream::canBeShifted(const MyTime &offsetTime, QList<section> *fuckedUpSections = NULL)
{
    return canBeShifted(offsetTime.days(), offsetTime.hours(), offsetTime.minutes(), fuckedUpSections);
}

void Stream::shiftStream(int days, int hours)
{
    MyTime requestDepartureTime = MyTime(m_sourceRequest->DG, m_sourceRequest->CG, 0);
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

QString Stream::print(bool b_PSInfo = false, bool b_RouteInfo = true,
                      bool b_BusyPossibilities = false, bool b_echelonsTimes = false)
{
    if(m_failed) {
        return QString::fromUtf8("Поток №%1 не спланирован. Причина: %2").arg(m_sourceRequest->NP).arg(m_failString);
    }

    QString str;
    str += m_sourceRequest->getString();

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

    //Время проследования эшелонов по станциям маршрута
    if(b_echelonsTimes) {
        str += QString::fromUtf8("\nВремя проследования эшелонов по станциям маршрута: ");
        foreach (echelon ech, m_echelones) {
            str += QString::fromUtf8("\n%1 эшелон: время отправления: %2, время прибытия: %3\n")
                .arg(ech.number)
                .arg(ech.timeDeparture)
                .arg(ech.timeArrival);
            int i = 0;
            foreach (MyTime time, ech.timesArrivalToStations) {
                station st = m_passedStations.at(i);
                str += QString::fromUtf8("%1 - %2")
                        .arg(time)
                        .arg(st);
                str += "\n";
                i++;
            }
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

QList<echelon> Stream::fillEchelones(const MyTime departureTime, int VP, int PK, int TZ, const QList<float> distancesBetweenStations, const QList<int> sectionsSpeed)
{
    QList<echelon> echs;
    QVector <int> sectionSpeedVector = sectionsSpeed.toVector();
    QList<PS> ps_list = dividePS(*m_sourceRequest);
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
        ech.timeDeparture = departureTime + MyTime::timeFromMinutes(i * delay);
        //время прибытия на первую станцию = времени отправления
        ech.timesArrivalToStations.append(ech.timeDeparture);


        int j = 0;
        MyTime  elapsedTime = ech.timeDeparture; //время прибытия к (j+1) станции
        foreach (float dist, distancesBetweenStations) {
            //расчёт времени въезда каждого эшелона на очередную станцию маршрута
            //должен осуществляться относительно времени доезда на прошлую станцию
            double minutes = (dist * 24.0 * 60.0) / sectionSpeedVector[j]; //часов от прошлой до текущей станции
            //если число часов содержит дробную часть, откидываем её и добавляем целый час
            if(minutes > int(minutes))
                elapsedTime = elapsedTime + MyTime::timeFromMinutes(minutes + 1);
            else
                elapsedTime = elapsedTime + MyTime::timeFromMinutes(minutes);
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
        echs.append(ech);
    }
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
        qDebug() << "Косяк при распределении ПС по поездам: Количество поездов = 0";
        return psList;
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
