#include "request.h"
#include "mydb.h"
#include "pvr.h"
#include "mytime.h"
#include "programsettings.h"
#include <QDebug>
#include "assert.h"

//может ли быть погружен (0 - нет, 1 - погрузка на станции, 2 - погрузка на ПВР)
int Request::canLoad(QMap<int, int> *p_loadAtDays) const
{
    if((VP != 23) && (VP != 24) && (VP != 25)) {
        qDebug() << "Погрузка рассчитывается только для 23/24/25 ВП";
        return 0;
    }
    //используем копию TZ, чтобы при TZ == 0 изменить его на TZ = 1 без последствий
    int _TZ = TZ;
    if(TZ == 0) _TZ = 1;
    int _PK = PK;
    if(_PK == 0) _PK = 1;
    Station *s1 = MyDB::instance()->stationByNumber(SP);
    PVR *p1 = MyDB::instance()->pvr(s1->pvrNumber);

    //проверяем соответствие кода груза виду перевозок
    QList<int> kgs = ProgramSettings::instance()->m_goodsTypes.keys(VP);
    if(!kgs.contains(KG)) {
        qDebug() << QString("Код груза: %1 не соответствует виду перевозок: %2")
                    .arg(KG)
                    .arg(VP);
        return 0;
    }

    //считаем время отправления каждого поезда
    QVector <MyTime> departureTimes;
    departureTimes.append(MyTime(DG - 1, CG, 0));
    float delay = 24.0 / (float)_TZ;        //задержка между отправлениями поездов [ч.]
    int delayBetweenTemp = 0;   //задержка в часах между отправлениями (для 24 ВП) [ч.]
    if(VP == 24) {
        delayBetweenTemp = QString::number(_TZ).left(1).toInt() * 24;
        _TZ = QString::number(_TZ).right(1).toInt();
        delay = 24.0 / (float)_TZ;
    }
    for(int i = 1; i < _PK; i++) {
        if((VP == 24)&&(i!=0)&&(i%_TZ==0))
            departureTimes.append(departureTimes.at(i - 1) + MyTime::timeFromHours(delayBetweenTemp));
        else
            departureTimes.append(departureTimes.at(i - 1) + MyTime::timeFromHours(delay));
    }

    //считаем сколько поездов в какой день будем грузить
    QMap <int, int> trainsToLoad; //<день, кол-во поездов>
    for(int i = 0; i < _PK; i++) {
        MyTime t = departureTimes.at(i);
        int old = trainsToLoad.value(t.days(), 0);
        trainsToLoad.insert(t.days(), old + 1);
    }

    //смотрим, сколько поездов в какой день свободно для погрузки
    QString load_type = ProgramSettings::instance()->m_goodsTypesDB.value(KG);
    if(load_type == "NO_TYPE") {
        qDebug() << QString("VP = %1, KP = %2, NP = %3. Нет соответствующего кода груза (%4) в типах груза")
                    .arg(VP)
                    .arg(KP)
                    .arg(NP)
                    .arg(KG);
        return 0;
    }
    else if(load_type == "23") {
        if(p1) {
            foreach (int key, trainsToLoad.keys()) {
                if(trainsToLoad.value(key) > p1->loadingPossibilities23[key]) {
                    qDebug() << QString("VP = %1, KP = %2, NP = %3. Нельзя погрузить заявку на ПВР %4")
                                .arg(VP)
                                .arg(KP)
                                .arg(NP)
                                .arg(p1->number);
                    return 0;
                }
                *p_loadAtDays = trainsToLoad;
                return 2;
            }
        }
        else {
            foreach (int key, trainsToLoad.keys()) {
                if(trainsToLoad.value(key) > s1->loadingPossibilities23[key]) {
                    qDebug() << QString("VP = %1, KP = %2, NP = %3. Нельзя погрузить заявку на станции %4")
                                .arg(VP)
                                .arg(KP)
                                .arg(NP)
                                .arg(*s1);
                    return 0;
                }
                *p_loadAtDays = trainsToLoad;
                return 1;
            }
        }
    }
    else if(load_type == "24_GSM") {
        foreach (int key, trainsToLoad.keys()) {
            if(trainsToLoad.value(key) > s1->loadingPossibilities24_GSM[key]) {
                qDebug() << QString("VP = %1, KP = %2, NP = %3. Нельзя погрузить заявку на станци %4")
                            .arg(VP)
                            .arg(KP)
                            .arg(NP)
                            .arg(*s1);
                return 0;
            }
            *p_loadAtDays = trainsToLoad;
            return 1;
        }
    }
    else if(load_type == "24_BP") {
        foreach (int key, trainsToLoad.keys()) {
            if(trainsToLoad.value(key) > s1->loadingPossibilities24_BP[key]) {
                qDebug() << QString("VP = %1, KP = %2, NP = %3. Нельзя погрузить заявку на станци %4")
                            .arg(VP)
                            .arg(KP)
                            .arg(NP)
                            .arg(*s1);
                return 0;
            }
            *p_loadAtDays = trainsToLoad;
            return 1;
        }
    }
    else if(load_type == "24_PR") {
        foreach (int key, trainsToLoad.keys()) {
            if(trainsToLoad.value(key) > s1->loadingPossibilities24_PR[key]) {
                qDebug() << QString("VP = %1, KP = %2, NP = %3. Нельзя погрузить заявку на станци %4")
                            .arg(VP)
                            .arg(KP)
                            .arg(NP)
                            .arg(*s1);
                return 0;
            }
            *p_loadAtDays = trainsToLoad;
            return 1;
        }
    }
    else if(load_type == "25") {
        foreach (int key, trainsToLoad.keys()) {
            if(trainsToLoad.value(key) > s1->loadingPossibilities25[key]) {
                qDebug() << QString("VP = %1, KP = %2, NP = %3. Нельзя погрузить заявку на станци %4")
                            .arg(VP)
                            .arg(KP)
                            .arg(NP)
                            .arg(*s1);
                return 0;
            }
            return 1;
        }
    }
    assert(0);
    return 0;
}

void Request::load(const QMap<int, int> p_loadAtDays)
{
    Station *s1 = MyDB::instance()->stationByNumber(SP);
    PVR *p1 = MyDB::instance()->pvr(s1->pvrNumber);
    QString load_type = ProgramSettings::instance()->m_goodsTypesDB.value(KG);
    if(load_type == "NO_TYPE") {
        qDebug() << QString("VP = %1, KP = %2, NP = %3. Нет соответствующего кода груза (%4) в типах груза")
                    .arg(VP)
                    .arg(KP)
                    .arg(NP)
                    .arg(KG);
        assert(0);
    }
    if(load_type == "23") {
        if(p1) {
            foreach (int key, p_loadAtDays.keys()) {
                p1->loadingPossibilities23[key] -= p_loadAtDays.value(key);
            }
        }
        else {
            foreach (int key, p_loadAtDays.keys()) {
                s1->loadingPossibilities23[key] -= p_loadAtDays.value(key);
            }
        }
    }
    else if(load_type == "24_GSM") {
        foreach (int key, p_loadAtDays.keys()) {
            s1->loadingPossibilities24_GSM[key] -= p_loadAtDays.value(key);
        }
    }
    else if(load_type == "24_BP") {
        foreach (int key, p_loadAtDays.keys()) {
            s1->loadingPossibilities24_BP[key] -= p_loadAtDays.value(key);
        }
    }
    else if(load_type == "24_PR") {
        foreach (int key, p_loadAtDays.keys()) {
            s1->loadingPossibilities24_PR[key] -= p_loadAtDays.value(key);
        }
    }
    else if(load_type == "25") {
        foreach (int key, p_loadAtDays.keys()) {
            s1->loadingPossibilities25[key] -= p_loadAtDays.value(key);
        }
    }
    else {
        assert(0);
    }
}

Request::operator QString() const
{
    QString str;
    Station *stSP = MyDB::instance()->stationByNumber(SP),
            *stSV = MyDB::instance()->stationByNumber(SV);
    str = QString::fromUtf8("Вид перевозок: %1, Код получателя: %2, Поток № %3")
            .arg(VP)
            .arg(KP)
            .arg(NP);
    str += QString::fromUtf8("\nНаименование и количество перевозимого: %1")
            .arg(NA);
    str += QString::fromUtf8("\nКоличество эшелонов: %1\nТемп заданный: %2").arg(PK).arg(TZ);
    str += QString::fromUtf8("\nВремя готовности: день:%1 час:%2").arg(DG - 1).arg(CG);
    str += QString::fromUtf8("\nСтанция погрузки: %1 (%2) \nСтанция выгрузки: %3 (%4)")
            .arg(stSP->name)
            .arg(stSP->number)
            .arg(stSV->name)
            .arg(stSV->number);
    if(!OM.isEmpty()) {
        str += QString::fromUtf8("\nОбязательные станции потока: ");
        foreach (int i, OM) {
            str += *(MyDB::instance()->stationByNumber(i)) + ", ";
        }
        str.chop(2);
    }
    str += "\n";
    return str;
}
