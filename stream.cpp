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
                if((k <= t.days())&&(k >= t_prev.days())) //здесь происходит выборка дня из времени прибытия
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
        int k = m_departureTime.days();  //итератор по всем дням
        for(int j = offsettedStartTime.days(); j < offsettedFinishTime.days(); j++) {
            if(passedSections[i].passingPossibilities[j] < busyPassingPossibilities[i][k])
            {
                QString strSection = MyDB::instance()->stationByNumber(passedSections[i].stationNumber1).name + " - " + MyDB::instance()->stationByNumber(passedSections[i].stationNumber2).name;
                qDebug() << QString::fromUtf8("пропускная возможность участка %1 на %2 день = %3 , а количество проходящих в этот день поездов по участку в потоке %4 = %5")
                            .arg(strSection)
                            .arg(j+1)
                            .arg(passedSections[i].passingPossibilities[j])
                            .arg(m_sourceRequest->NP)
                            .arg(busyPassingPossibilities[i][k]);
                if(fuckedUpSections != 0) {
                    if(!fuckedUpSections->contains(passedSections[i]))
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

bool Stream::canBeShifted(int days, int hours, int minutes)
{
    //чтобы проверить, может ли поток быть сдвинут на days дней вперёд или назад, необходимо, чтобы выполнились следующие условия:
    //1)от времени прибытия на каждую станцию отнять время, идущее в параметре функции
    //2)перерасчитать пропусную возможность по дням для данного маршрута
    //3)посмотреть, сможет ли пройти маршрут по участкам с новым временем прибытия (функция bool Route::canPassSections(NULL))
    if(m_failed) return false;

    MyTime requestDepartureTime = MyTime(m_sourceRequest->DG, m_sourceRequest->CG, 0);
    MyTime offset(days, hours, minutes);

    QVector< QVector<int> > tmpBusyPassingPossibilities;    //перерасчитанная занятость участков
    QList<float> distances = distancesTillStations();
    QList<int> sectionSpeeds;
    foreach (section sec, m_passedSections) {
        sectionSpeeds.append(sec.speed);
    }
    QList< echelon > tmpEchelones = m_graph->fillEchelones(requestDepartureTime + offset, m_sourceRequest->PK, m_sourceRequest->TZ, distances, sectionSpeeds);          //делаем копию эшелонов, т.к. будем их менять

    tmpBusyPassingPossibilities = calculatePV(tmpEchelones);
    return canPassSections(m_passedSections, tmpBusyPassingPossibilities, MyTime(days, hours, minutes), NULL);
}

bool Stream::canBeShifted(const MyTime &offsetTime)
{
    return canBeShifted(offsetTime.days(), offsetTime.hours(), offsetTime.minutes());
}

void Stream::shiftStream(int days, int hours)
{
    MyTime requestDepartureTime = MyTime(m_sourceRequest->DG, m_sourceRequest->CG, 0);
    MyTime offset(days, hours, 0);

    QList<float> distances = distancesTillStations();
    QList<int> sectionSpeeds;
    foreach (section sec, m_passedSections) {
        sectionSpeeds.append(sec.speed);
    }
    m_echelones = m_graph->fillEchelones(requestDepartureTime + offset, m_sourceRequest->PK, m_sourceRequest->TZ, distances, sectionSpeeds);          //делаем копию эшелонов, т.к. будем их менять
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

QString Stream::print()
{
    if(m_failed) {
        return QString::fromUtf8("Поток №%1 не спланирован. Причина: %2").arg(m_sourceRequest->NP).arg(m_failString);
    }
    QString str;
    str += m_sourceRequest->getString();
    str += QString::fromUtf8("\nВремя отправление первого эшелона потока: %1").arg(m_echelones.first().timeDeparture.getString());
    str += QString::fromUtf8("\nВремя прибытия первого эшелона потока: %1").arg(m_echelones.first().timeArrival.getString());
    str += QString::fromUtf8("\nВремя отправления последнего эшелона потока: %1").arg(m_echelones.last().timeDeparture.getString());
    str += QString::fromUtf8("\nВремя прибытия последнего эшелона потока: %1").arg(m_echelones.last().timeArrival.getString());
    str += QString::fromUtf8("\nМаршрут потока: ");
    foreach (station tmpSt, m_passedStations) {
        str += tmpSt.name + "  -  ";
    }
    str.chop(5);
    str += QString::fromUtf8("\nДлина маршрута = %1 км").arg(m_graph->distanceTillStation(m_passedStations.count() - 1, m_passedStations));

//    str += QString::fromUtf8("\nЗанятость погрузки по дням на %1 ПВР:").arg(MyDB::instance()->pvrByStationNumber(m_sourceRequest->SP).name);
//    int i = 0;
//    foreach (int j, m_sourceRequest->m_loadingPossibility) {
//        str += QString::fromUtf8("\n %1 день: %2/%3").arg(i + m_sourceRequest->DG).arg(j).arg(MyDB::instance()->pvrByStationNumber(m_sourceRequest->SP).pv[i]);
//        i++;
//    }

//    str += QString::fromUtf8("\nЗанятость разгрузки по дням на %1 ПВР:").arg(MyDB::instance()->pvrByStationNumber(m_sourceRequest->SV).name);
//    i = 0;
//    foreach (int j, m_sourceRequest->m_unloadingPossibility) {
//        str += QString::fromUtf8("\n %1 день: %2/%3").arg(i).arg(j).arg(MyDB::instance()->pvrByStationNumber(m_sourceRequest->SV).pv[i]);
//        i++;
//    }

    str += QString::fromUtf8("\nЗанятость участков по дням:");
    for(int j = 0; j < m_passedSections.count(); j++) {
        str += "\n" + MyDB::instance()->stationByNumber(m_passedSections[j].stationNumber1).name + " - " + MyDB::instance()->stationByNumber(m_passedSections[j].stationNumber2).name + ":\n";
        for(int k = 0; k < m_busyPassingPossibilities[j].count(); k++) {
            if(m_busyPassingPossibilities[j][k] != 0)
                str += QString::fromUtf8("%1 день: %2/%3, ").arg(k).arg(m_busyPassingPossibilities[j][k]).arg(m_passedSections[j].passingPossibilities[k]);
        }
        str.chop(2);
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
//    m_sourceRequest = NULL;
//    m_graph = NULL;
    m_passedStations.clear();
    m_passedSections.clear();
    m_echelones.clear();
    m_departureTime = MyTime(0, 0, 0);
    m_arrivalTime = MyTime(0, 0, 0);
}

QList<float> Stream::distancesTillStations()
{
    QList<float> dists;
    if(m_passedStations.isEmpty()) {
        qDebug() << "Route::distancesTillStations: no stations";
        return dists;
    }

    for(int i = 1; i < m_passedStations.count(); i++) {
        float dist = m_graph->distanceTillStation(i, m_passedStations);
        dists.append(dist);
    }
    return dists;
}
