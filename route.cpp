#include "route.h"
#include "request.h"
#include "mydb.h"
#include "graph.h"
#include <QDebug>

Route::Route(): m_planned(false), m_departureTime(MyTime(0,0,0)), m_failed(false) {}

Route::Route(Request* request, Graph *gr): m_sourceRequest(request), m_graph(gr), m_planned(false), m_departureTime(MyTime(request->DG, request->CG, 0)), m_failed(false) {}

QVector< QVector<int> > Route::calculatePV(const QVector <echelon> &echelones)
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
        //1 < j < количество пройденных поездом станций
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

void Route::fillSections()
{
    if(!m_passedSections.isEmpty()) m_passedSections.clear();

    for(int i = 0; i < m_passedStations.count(); i++) {
        if(m_passedStations[i] == m_passedStations.last()) return;
        section s;
        s = MyDB::instance()->sectionByStations(m_passedStations[i], m_passedStations[i+1]);
        m_passedSections.append(s);
    }
 }

bool Route::canBePlanned(bool bWriteInBase)
{
    if(m_sourceRequest->canLoad() && canPassSections(m_passedSections, m_busyPassingPossibilities))
        return true;
    return false;
}

bool Route::canPassSections(const QVector<section> &passedSections, const QVector< QVector<int> > &busyPassingPossibilities, MyTime timeOffset, QVector<section> *fuckedUpSections)
{
    MyTime offsettedStartTime = m_departureTime + timeOffset;       //сдвинутое время начала перевозок
    MyTime offsettedFinishTime = m_arrivalTime + timeOffset;     //сдвинутое время окончания перевозок
    if(offsettedStartTime.toMinutes() < 0) return false;
    if(offsettedFinishTime.toMinutes() >= 60 * 24 * 60) return false;

    bool can = true;
    for(int i = 0; i < passedSections.count(); i++) {
        int k = m_departureTime.days();  //итератор по всем дням
        for(int j = offsettedStartTime.days(); j < offsettedFinishTime.days(); j++) {
            if(!(passedSections[i].passingPossibilities[j] >= busyPassingPossibilities[i][k]))
            {
                qDebug() << "пропускная возможность участка " << MyDB::instance()->stationByNumber(passedSections[i].stationNumber1).name << " - " << MyDB::instance()->stationByNumber(passedSections[i].stationNumber2).name << " на " << j+1 << " день = "
                            << passedSections[i].passingPossibilities[j] << " , а количество проходящих в этот день поездов на этой станции в заявке с номером потока " << m_sourceRequest->NP
                               << " = " << busyPassingPossibilities[i][k];
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

bool Route::canBeShifted(int days, int hours, int minutes)
{
    //чтобы проверить, может ли поток быть сдвинут на days дней вперёд или назад, необходимо, чтобы выполнились следующие условия:
    //1)от времени прибытия на каждую станцию отнять время, идущее в параметре функции
    //2)перерасчитать пропусную возможность по дням для данного маршрута
    //3)посмотреть, сможет ли пройти маршрут по участкам с новым временем прибытия (функция bool Route::canPassSections(NULL))
    if(m_failed) return false;

    QVector< QVector<int> > tmpBusyPassingPossibilities;    //перерасчитанная занятость участков
    QVector< echelon > tmpEchelones = m_echelones;          //делаем копию эшелонов, т.к. будем их менять

    //ЗДЕСЬ НУЖНО ПЕРЕСЧИТАТЬ ЭШЕЛОНЫ НА НОВОЕ, СМЕЩЁННОЕ ВРЕМЯ

    //

    tmpBusyPassingPossibilities = calculatePV(tmpEchelones);
    return canPassSections(m_passedSections, tmpBusyPassingPossibilities, MyTime(days, hours, minutes), NULL);
//    if(planned()) {
//        if(canPassSections(m_passedSections, tmpBusyPassingPossibilities, MyTime(days, hours, minutes), NULL)) {
//            return true;
//        }
//        else return false;
//    }
//    else {
//        qDebug() << "Нельзя проверить, может ли быть поток сдвинут, если он не спланирован";
//        return false;
//    }
}

bool Route::canBeShifted(const MyTime &offsetTime)
{
    return canBeShifted(offsetTime.days(), offsetTime.hours(), offsetTime.minutes());
}

int Route::length()
{
    if(!m_passedSections.isEmpty())
        return m_graph->distanceBetweenStations(0, m_passedStations.count() - 1, m_passedStations);
    else
        return -1;
}

QString Route::print()
{
    if(m_failed) {
        return QString::fromUtf8("Поток №%1 не спланирован. Причина: %2").arg(m_sourceRequest->NP).arg(m_failString);
    }
    QString str;
    str += m_sourceRequest->getString();
    str += QString::fromUtf8("\nВремя отправление первого эшелона потока: %1").arg(m_departureTime.getString());
    str += QString::fromUtf8("\nВремя прибытия последнего эшелона потока: %1").arg(m_arrivalTime.getString());
    str += QString::fromUtf8("\nМаршрут потока: ");
    foreach (station tmpSt, m_passedStations) {
        str += tmpSt.name + "  -  ";
    }
    str.chop(5);
    str += QString::fromUtf8("\nДлина маршрута = %1 км").arg(m_graph->distanceTillStation(m_passedStations.count() - 1, m_passedStations));

    str += QString::fromUtf8("\nЗанятость погрузки по дням на %1 ПВР:").arg(MyDB::instance()->pvrByStationNumber(m_sourceRequest->SP).name);
    int i = 0;
    foreach (int j, m_sourceRequest->m_loadingPossibility) {
        str += QString::fromUtf8("\n %1 день: %2/%3").arg(i + 1 + m_sourceRequest->DG).arg(j).arg(MyDB::instance()->pvrByStationNumber(m_sourceRequest->SP).pv[i]);
        i++;
    }

    str += QString::fromUtf8("\nЗанятость разгрузки по дням на %1 ПВР:").arg(MyDB::instance()->pvrByStationNumber(m_sourceRequest->SV).name);
    i = 0;
    foreach (int j, m_sourceRequest->m_unloadingPossibility) {
        str += QString::fromUtf8("\n %1 день: %2/%3").arg(i+1).arg(j).arg(MyDB::instance()->pvrByStationNumber(m_sourceRequest->SV).pv[i]);
        i++;
    }

    str += QString::fromUtf8("\nЗанятость участков по дням:");
    for(int j = 0; j < m_busyPassingPossibilities.count(); j++) {
        str += "\n" + MyDB::instance()->stationByNumber(m_passedSections[j].stationNumber1).name + " - " + MyDB::instance()->stationByNumber(m_passedSections[j].stationNumber2).name + ":\n";
        for(int k = 0; k < m_busyPassingPossibilities[j].count(); k++) {
//            if(pv[j][k] != 0)
                str += QString::fromUtf8("%1 день: %2/%3, ").arg(k+1).arg(m_busyPassingPossibilities[j][k]).arg(m_passedSections[j].passingPossibilities[k]);
        }
        str.chop(2);
    }
    str += "\n\n";
    return str;
}

void Route::setFailed(QString errorString)
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
