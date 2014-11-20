#include "request.h"
#include "mydb.h"
#include "pvr.h"
#include <QDebug>

bool Request::canLoad()
{
    if(!TZ) {
        qDebug() << "Деление на 0";
        return true;
    }

    station s1 = MyDB::instance()->stationByNumber(SP);
    station s2 = MyDB::instance()->stationByNumber(SV);
    pvr p1 = MyDB::instance()->pvrByStationNumber(SP);
    pvr p2 = MyDB::instance()->pvrByStationNumber(SV);
    int days_load = PK / TZ; //количество дней, необходимых на погрузку
    int i;

    //[1]погрузка
    //[2]если станция принадлежит ПВР
    if(p1.number != 0) {
        for (i = 0; i < days_load; i++) {
            if(PK / TZ > p1.pv[i]) {
                qDebug() << QString("Нельзя погрузить поток №%1 на ПВР - %2").arg(NP).arg(p1.name);
                m_loadingPossibility.clear();
                return false;
            }
            else {
                m_loadingPossibility.append(PK / TZ);
            }
        }

        //считаем погрузку оставшихся поездов на последний день
        if(PK % TZ != 0) {
            if(PK % TZ > p1.pv[i + 1]) {
                qDebug() << QString("Нельзя погрузить поток №%1 на ПВР - %2").arg(NP).arg(p1.name);
                m_loadingPossibility.clear();
                return false;
            }
            else {
                m_loadingPossibility.append(PK % TZ);
            }
        }
    }
    //[!2]
    else {
        //[3] если станция не принадлежит ПВР, но вид перевозок - оперативный, смотрим погрузочную способность для участка
        if(VP == 23) {
            for (i = 0; i < days_load; i++) {
                if(PK / TZ > s1.loadingPossibilityForOperativeTraffic) {
                    qDebug() << QString("Нельзя погрузить поток №%1 на станции - %2").arg(NP).arg(s1.name);
                    m_loadingPossibility.clear();
                    return false;
                }
                else {
                    m_loadingPossibility.append(PK / TZ);
                }
            }
            if(PK % TZ != 0) {
                if(PK % TZ > s1.loadingPossibilityForOperativeTraffic) {
                    qDebug() << QString("Нельзя погрузить поток №%1 на станции - %2").arg(NP).arg(s1.name);
                    m_loadingPossibility.clear();
                    return false;
                }
                else {
                    m_loadingPossibility.append(PK % TZ);
                }
            }
            qDebug() << QString("Поток №%1 будет погружен на станции %2, вид перевозок - оперативные").arg(NP).arg(s1.name);
        }
        //[!3]
        else {
            qDebug() << QString("Нельзя погрузить поток №%1, так как станция погрузки не принадлежит ПВР и вид перевозок - не оперативные").arg(NP);
            return false;
        }
    }
    //[!1]


//    //[1]разрузка
//    //[2]если станция принадлежит ПВР
//    if(p1.number != 0) {
//        for (i = 0; i < days_load; i++) {
//            if(PK / TZ > p1.pv[i]) {
//                qDebug() << QString("Нельзя погрузить поток №%1 на ПВР - %2").arg(NP).arg(p1.name);
//                m_loadingPossibility.clear();
//                return false;
//            }
//            else {
//                m_loadingPossibility.append(PK / TZ);
//            }
//        }

//        //считаем погрузку оставшихся поездов на последний день
//        if(PK % TZ != 0) {
//            if(PK % TZ > p1.pv[i + 1]) {
//                qDebug() << QString("Нельзя погрузить поток №%1 на ПВР - %2").arg(NP).arg(p1.name);
//                m_loadingPossibility.clear();
//                return false;
//            }
//            else {
//                m_loadingPossibility.append(PK % TZ);
//            }
//        }
//    }
//    //[!2]
//    else {
//        //[3] если станция не принадлежит ПВР, но вид перевозок - оперативный, смотрим погрузочную способность для участка
//        if(VP == 23) {
//            for (i = 0; i < days_load; i++) {
//                if(PK / TZ > s1.loadingPossibilityForOperativeTraffic) {
//                    qDebug() << QString("Нельзя погрузить поток №%1 на станции - %2").arg(NP).arg(s1.name);
//                    m_loadingPossibility.clear();
//                    return false;
//                }
//                else {
//                    m_loadingPossibility.append(PK / TZ);
//                }
//            }
//            if(PK % TZ != 0) {
//                if(PK % TZ > s1.loadingPossibilityForOperativeTraffic) {
//                    qDebug() << QString("Нельзя погрузить поток №%1 на станции - %2").arg(NP).arg(s1.name);
//                    m_loadingPossibility.clear();
//                    return false;
//                }
//                else {
//                    m_loadingPossibility.append(PK % TZ);
//                }
//            }
//            qDebug() << QString("Поток №%1 будет погружен на станции %2, вид перевозок - оперативные").arg(VP).arg(s1.name);
//        }
//        //[!3]
//        else {
//            qDebug() << QString("Нельзя погрузить поток №%1, так как станция погрузки не принадлежит ПВР и вид перевозок - не оперативные").arg(NP);
//            return false;
//        }
//    }
//    //[!1]

    return true;
}

QString Request::getString() const
{
    QString str;
    str = QString::fromUtf8("Вид перевозок: %1, Код получателя: %2, Поток № %3")
            .arg(VP)
            .arg(KP)
            .arg(NP);
    str += QString::fromUtf8("\nКоличество эшелонов: %1\nТемп заданный: %2").arg(PK).arg(TZ);
    str += QString::fromUtf8("\nВремя готовности: день:%1 час:%2").arg(DG).arg(CG);
    if(!OM.isEmpty()) str += QString::fromUtf8("\nОбязательные станции потока: ");
    foreach (int i, OM) {
        str += MyDB::instance()->stationByNumber(i).name + ", ";
    }
    str.chop(2);
    str += "\n";
    return str;
}






























