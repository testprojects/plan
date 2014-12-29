#include "request.h"
#include "mydb.h"
#include "pvr.h"
#include "mytime.h"
#include <QDebug>


bool Request::canLoad()
{
//    //используем копию TZ, чтобы при TZ == 0 изменить его на TZ = 1 без последствий
//    int _TZ = TZ;
//    if(_TZ < 5) _TZ = 3;
//    station s1 = MyDB::instance()->stationByNumber(SP);
//    station s2 = MyDB::instance()->stationByNumber(SV);
//    pvr p1 = MyDB::instance()->pvrByStationNumber(SP);
//    pvr p2 = MyDB::instance()->pvrByStationNumber(SV);

//    //определяем по какому алгоритму будем грузить, исходя из кода груза
//    switch(KG) {
//    case 23: loading_type = e23; break;
//    case 4: loading_type = eBP; break;
//    case 5: loading_type = eGSM; break;
//    case 601: loading_type = ePR; break;
//    case 602: loading_type = ePR; break;
//    case 603: loading_type = ePR; break;
//    case 604: loading_type = ePR; break;
//    case 605: loading_type = ePR; break;
//    case 606: loading_type = ePR; break;
//    case 607: loading_type = ePR; break;
//    case 608: loading_type = ePR; break;
//    case 609: loading_type = ePR; break;
//    case 610: loading_type = ePR; break;
//    case 611: loading_type = ePR; break;
//    case 612: loading_type = ePR; break;
//    case 613: loading_type = ePR; break;
//    case 614: loading_type = ePR; break;
//    case 615: loading_type = ePR; break;
//    case 616: loading_type = ePR; break;
//    case 617: loading_type = ePR; break;
//    case 618: loading_type = ePR; break;
//    case 619: loading_type = ePR; break;
//    case 620: loading_type = ePR; break;
//    case 70+77: loading_type = e25; break;
//    default: loading_type = -1;
//    }
//    assert(loading_type != -1);

//    QMap<int,int> trainsLoaded;                         //<день, поездов грузится>
//    MyTime departureTime = MyTime(DG, CG, 0);

//    int k;              //погрузочная способность (в зависимости от когда груза)

//    //считаем задержку между погрузкой поездов
//    MyTime delay;
//    switch(loading_type) {
//    case e23: {
//        k = p1.ps;      //здесь используется погрузочная способность района
//        if(24%k > 0)
//            delay = MyTime(0, 24/k + 1, 0);
//        else
//            delay = MyTime(0, 24/k, 0);
//        break;
//    }
//    case eBP: {
//        k = s1.loadingCapacity24_BP;
//        if(24%k > 0)
//            delay = MyTime(0, 24/k + 1, 0);
//        else
//            delay = MyTime(0, 24/k, 0);
//        break;
//    }
//    case eGSM: {
//        k = s1.loadingCapacity24_GSM;
//        if(24%k > 0)
//            delay = MyTime(0, 24/k + 1, 0);
//        else
//            delay = MyTime(0, 24/k, 0);
//        break;
//    }
//    case ePR: {
//        k = s1.loadingCapacity24_PR;
//        if(24%k > 0)
//            delay = MyTime(0, 24/k + 1, 0);
//        else
//            delay = MyTime(0, 24/k, 0);
//        break;
//    }
//    case e25: {
//        k = s1.loadingCapacity25;
//        if(24%k > 0)
//            delay = MyTime(0, 24/k + 1, 0);
//        else
//            delay = MyTime(0, 24/k, 0);
//        break;
//    }
//    }


//    //[1]погрузка
//    //[2]если станция принадлежит ПВР
//    if(p1.number != 0) {
//        for (i = 0; i < days_load; i++) {
//            if(PK / _TZ > p1.pv[i]) {
//                qDebug() << QString("Нельзя погрузить поток №%1 на ПВР - %2").arg(NP).arg(p1.name);
//                m_loadingPossibility.clear();
//                return false;
//            }
//            else {
//                m_loadingPossibility.append(PK / _TZ);
//            }
//        }

//        //считаем погрузку оставшихся поездов на последний день
//        if(PK % _TZ != 0) {
//            if(PK % _TZ > p1.pv[i + 1]) {
//                qDebug() << QString("Нельзя погрузить поток №%1 на ПВР - %2").arg(NP).arg(p1.name);
//                m_loadingPossibility.clear();
//                return false;
//            }
//            else {
//                m_loadingPossibility.append(PK % _TZ);
//            }
//        }
//    }
//    //[!2]
//    else {
//        //[3] если станция не принадлежит ПВР, но вид перевозок - оперативный, смотрим погрузочную способность станции
//        if(VP == 23) {
//            for (i = 0; i < days_load; i++) {
//                if(PK / _TZ > s1.loadingPossibilityForOperativeTraffic) {
//                    qDebug() << QString("Нельзя погрузить поток №%1 на станции - %2").arg(NP).arg(s1.name);
//                    m_loadingPossibility.clear();
//                    return false;
//                }
//                else {
//                    m_loadingPossibility.append(PK / _TZ);
//                }
//            }
//            if(PK % _TZ != 0) {
//                if(PK % _TZ > s1.loadingPossibilityForOperativeTraffic) {
//                    qDebug() << QString("Нельзя погрузить поток №%1 на станции - %2").arg(NP).arg(s1.name);
//                    m_loadingPossibility.clear();
//                    return false;
//                }
//                else {
//                    m_loadingPossibility.append(PK % _TZ);
//                }
//            }
//            qDebug() << QString("Поток №%1 будет погружен на станции %2, вид перевозок - оперативные").arg(NP).arg(s1.name);
//        }
//        //[!3]
//        else {
//            qDebug() << QString("Нельзя погрузить поток №%1, так как станция погрузки не принадлежит ПВР и вид перевозок - не оперативные").arg(NP);
//            return false;
//        }
//    }
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
    station stSP = MyDB::instance()->stationByNumber(SP),
            stSV = MyDB::instance()->stationByNumber(SV);
    str = QString::fromUtf8("Вид перевозок: %1, Код получателя: %2, Поток № %3")
            .arg(VP)
            .arg(KP)
            .arg(NP);
    str += QString::fromUtf8("\nНаименование и количество перевозимого: %1")
            .arg(NA);
    str += QString::fromUtf8("\nКоличество эшелонов: %1\nТемп заданный: %2").arg(PK).arg(TZ);
    str += QString::fromUtf8("\nВремя готовности: день:%1 час:%2").arg(DG).arg(CG);
    str += QString::fromUtf8("\nСтанция погрузки: %1 (%2) \nСтанция выгрузки: %3 (%4)")
            .arg(stSP.name)
            .arg(stSP.number)
            .arg(stSV.name)
            .arg(stSV.number);
    if(!OM.isEmpty()) {
        str += QString::fromUtf8("\nОбязательные станции потока: ");
        foreach (int i, OM) {
            str += MyDB::instance()->stationByNumber(i).name + ", ";
        }
        str.chop(2);
    }
    str += "\n";
    return str;
}






























