#include "request.h"
#include "mydb.h"
#include "pvr.h"
#include "mytime.h"
#include "programsettings.h"
#include "pvrbusy.h"
#include "stationbusy.h"
#include <QDebug>


bool Request::canLoad(QMap<int, int> *p_loadAtDays)
{
    //виды перевозок, которые используются для расчёта погрузки = [21, 22, 23, 24, 25]
    //используем копию TZ, чтобы при TZ == 0 изменить его на TZ = 1 без последствий
    int _TZ = TZ;
    if(_TZ < 5) _TZ = 3;
    int _PK = PK;
    if(_PK == 0) _PK = 1;
    station s1 = MyDB::instance()->stationByNumber(SP);
    station s2 = MyDB::instance()->stationByNumber(SV);
    pvr p1 = MyDB::instance()->pvrByNumber(s1.pvrNumber);
    pvr p2 = MyDB::instance()->pvrByNumber(s2.pvrNumber);

    //проверяем соответствие кода груза виду перевозок
    QList<int> kgs = ProgramSettings::instance()->goodsTypes.keys(VP);
    if(!kgs.contains(KG)) {
        qDebug() << QString("Код груза: %1 не соответствует виду перевозок: %2")
                    .arg(KG)
                    .arg(VP);
        return false;
    }

    //считаем время отправления каждого поезда
    QVector <MyTime> departureTimes;
    QMap <int, int> trainsByDays; //<день, кол-во поездов>
    departureTimes.append(MyTime(DG, CG, 0));
    int delay = 24 / _TZ;        //задержка между отправлениями поездов [ч.]
    int delayBetweenTemp = 0;   //задержка в часах между отправлениями (для 24 ВП) [ч.]
    if(VP == 24) {
        delayBetweenTemp = QString::number(_TZ).left(1).toInt() * 24;
        _TZ = QString::number(_TZ).right(1).toInt();
    }

    for(int i = 1; i < _PK; i++) {
        if((VP == 24)&&(i!=0)&&(i%_TZ==0))
            departureTimes.append(departureTimes.at(i - 1) + MyTime::timeFromHours(delayBetweenTemp));
        else
            departureTimes.append(departureTimes.at(i - 1) + MyTime::timeFromHours(delay));
    }

    for(int i = 0; i < _PK; i++) {
        MyTime t = departureTimes.at(i);
        if(trainsByDays.value(t.days(), -1) == -1)
            trainsByDays.insert(t.days(), 1);
        else {
            int amount = trainsByDays.value(t.days());
            trainsByDays.insert(t.days(), amount + 1);
        }
        qDebug() << QString("Время отправления %1-го поезда: %2")
                    .arg(i)
                    .arg(departureTimes.at(i));
    }




    if(VP == 21) {
        //грузимся в ПВР
        if(p1.number != 0) {
            bool can = true;
            foreach (int key, trainsByDays.keys()) {
                if(p1.pv[key] < trainsByDays.value(key)) {
                    can = false;
                    break;
                }
            }
            if(can) {
                *p_loadAtDays = trainsByDays;
                return true;
            }
        }
        //Если ПВР нет - грузимся на станции
        else {
            foreach (int key, trainsByDays.keys()) {
                if(s1.loadingPossibilities23[key] < trainsByDays.value(key)) {
                    qDebug() << QString("Поток особого учёта %1 не может быть погружены на станции %2: %3 / %4")
                                .arg(NP)
                                .arg(s1.name)
                                .arg(s1.loadingPossibilities23[key])
                                .arg(trainsByDays.value(key));
                    return false;
                }
            }
        }
    }

    else if(VP == 22) {
        foreach (int key, trainsByDays.keys()) {
            if(s1.loadingPossibilities24_PR[key] < trainsByDays.value(key)) {
                qDebug() << QString("Нельзя погрузить поток %1 на станции %2.\nПогрузочная возможность в %3 день = %4, а нужно погрузить"
                                    " %5 поездов")
                            .arg(NP)
                            .arg(s1.name)
                            .arg(key)
                            .arg(s1.loadingPossibilities24_PR[key])
                            .arg(trainsByDays.value(key));
                return false;
            }
        }
    }

    //если вид перевозок = 23, смотрим сможем ли погрузиться в ПВР
    else if(VP == 23) {
        foreach (int key, trainsByDays.keys()) {
            if(p1.pv[key] < trainsByDays.value(key)) {
                qDebug() << QString("Нельзя погрузить поток %1 в %2 ПВР.\nПогрузочная возможность в %3 день = %4, а нужно погрузить"
                                    " %5 поездов")
                            .arg(NP)
                            .arg(p1.name)
                            .arg(key)
                            .arg(p1.pv[key])
                            .arg(trainsByDays.value(key));
                return false;
            }
        }
        *p_loadAtDays = trainsByDays;
        return true;
    }
    //для остальных видов перевозок
    else {
        //выбираем поле БД в зависимости от кода груза
        QString type = ProgramSettings::instance()->goodsTypesDB.value(KG);


        foreach (int key, trainsByDays.keys()) {
            //типы грузов: 23, 24BP, 24GSM, 24PR, 25
            int pvDay;

            if(type == "24BP")
                pvDay = s1.loadingPossibilities24_BP[key];
            else if(type == "24GSM")
                pvDay = s1.loadingPossibilities24_GSM[key];
            else if(type == "24PR")
                pvDay = s1.loadingPossibilities24_PR[key];
            else if(type == "25")
                pvDay = s1.loadingPossibilities25[key];
            else {
                qDebug() << QString("Код груза %1 не подлежит подстчёту")
                            .arg(KG);
                return false;
            }

            if(pvDay < trainsByDays.value(key)) {
                qDebug() << QString("Нельзя погрузить поток %1 в %2 ПВР.\nПогрузочная возможность в %3 день = %4, а нужно погрузить"
                                    " %5 поездов")
                            .arg(NP)
                            .arg(s1.name)
                            .arg(key)
                            .arg(pvDay)
                            .arg(trainsByDays.value(key));
                return false;
            }
        }
        *p_loadAtDays = trainsByDays;
        return true;
    }

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

    return true;
}

void Request::load(const QMap<int, int> &trainsAtDays)
{
    station sp = MyDB::instance()->stationByNumber(SP);
    if(sp.pvrNumber != 0) {
        MyDB::instance()->loadAtPVR(sp.pvrNumber,  KG, VP, KP, NP, trainsAtDays);
        //изменить занятость ПВР
    }
    else {
        MyDB::instance()->loadAtStation(sp.number, KG, VP, KP, NP, trainsAtDays);
        //изменить занятость станции
    }
}

Request::operator QString() const
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
