#include "request.h"
#include "mydb.h"
#include "pvr.h"
#include "mytime.h"
#include "programsettings.h"
#include "pvrbusy.h"
#include "stationbusy.h"
#include <QDebug>

//может ли быть погружен (0 - нет, 1 - погрузка на станции, 2 - погрузка на ПВР)
int Request::canLoad(QMap<int, int> *p_loadAtDays, int *alternativeStationNumber)
{
    if((VP != 23) && (VP != 24) && (VP != 25)) {
        qDebug() << "Погрузка рассчитывается только для 23/24/25 ВП";
        return 0;
    }
    //используем копию TZ, чтобы при TZ == 0 изменить его на TZ = 1 без последствий
    int _TZ = TZ;
    if(_TZ < 5) _TZ = 3;
    int _PK = PK;
    if(_PK == 0) _PK = 1;
    station s1 = MyDB::instance()->stationByNumber(SP);
    pvr p1 = MyDB::instance()->PVRByNumber(s1.pvrNumber);

    //проверяем соответствие кода груза виду перевозок
    QList<int> kgs = ProgramSettings::instance()->goodsTypes.keys(VP);
    if(!kgs.contains(KG)) {
        qDebug() << QString("Код груза: %1 не соответствует виду перевозок: %2")
                    .arg(KG)
                    .arg(VP);
        return 0;
    }

    //считаем время отправления каждого поезда
    QVector <MyTime> departureTimes;
    QMap <int, int> trainsByDays; //<день, кол-во поездов>
    departureTimes.append(MyTime(DG - 1, CG, 0));
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




    //если вид перевозок = 23, смотрим сможем ли погрузиться на станции
    //если нет, ищем ближайшую свободную станцию, входящую в соотв. ПВР
    //если таких нет - погрузка невозможна. иначе - занимаем станцию
    //смотрим, сможем ли занять ПВР.
    //Если нет - ОШИБКА (занятость ПВР должна соответствовать сумме занятости станций, входящих в него)
    if(VP == 23) {
        QList<station> freePVRStations;
        foreach (int key, trainsByDays.keys()) {
            if(s1.loadingPossibilities23[key] < trainsByDays.value(key)) {
                if(s1.pvrNumber == 0) {
                    qDebug() << QString("Нельзя погрузить поток %1 на %2 станции.\nПогрузочная возможность в %3 день = %4, а нужно погрузить"
                                        " %5 поездов")
                                .arg(NP)
                                .arg(s1.name)
                                .arg(key)
                                .arg(s1.loadingPossibilities23[key])
                                .arg(trainsByDays.value(key));
                    return 0;
                }
                freePVRStations = MyDB::instance()->freeStationsInPVR(s1.number, trainsByDays, KG);
                if(freePVRStations.isEmpty()) {
                    qDebug() << QString("В ПВР с номером %1 нет свободных станций для погрузки потока №%2")
                                .arg(s1.pvrNumber)
                                .arg(NP);
                    return 0;
                }
                else {
                    //выбираем первую попавшуюся станцию из свободных в ПВР
                    *alternativeStationNumber = freePVRStations.first().number;
//                    SP = freePVRStations.first().number;
                }
            }
        }
        //если погрузка происходит на станции без ПВР, итс ок
        if(p1.number == 0) {
            *p_loadAtDays = trainsByDays;
            return 1;
        }
        //иначе на ПВР тоже грузимся
        foreach (int key, trainsByDays.keys()) {
            if(p1.pv[key] < trainsByDays.value(key)) {
                qDebug() << QString("Ошибка: занятость ПВР должна соответствовать сумме занятости станций, входящих в него:\n"
                            "ПВР: %1")
                            .arg(p1.name);
                exit(16);
            }
        }
        *p_loadAtDays = trainsByDays;
        return 2;
    }

    //для остальных видов перевозок ПВР не используется
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
                qDebug() << QString("Кода груза %1 нет в словаре типов груза (реестр)")
                            .arg(KG);
                return 0;
            }

            if(pvDay < trainsByDays.value(key)) {
                qDebug() << QString("Нельзя погрузить поток %1 на %2 станции.\nПогрузочная возможность в %3 день = %4, а нужно погрузить"
                                    " %5 поездов")
                            .arg(NP)
                            .arg(s1.name)
                            .arg(key)
                            .arg(pvDay)
                            .arg(trainsByDays.value(key));
                return 0;
            }
        }
        *p_loadAtDays = trainsByDays;
        return 1;
    }
}

void Request::load(const QMap<int, int> &trainsAtDays)
{
    station sp = MyDB::instance()->stationByNumber(SP);
    if(VP == 23) {
        MyDB::instance()->loadRequestAtPVR(sp.pvrNumber,  KG, VP, KP, NP, trainsAtDays);
        MyDB::instance()->loadRequestAtStation(sp.number, KG, VP, KP, NP, trainsAtDays);
        //изменить занятость ПВР
    }
    else {
        MyDB::instance()->loadRequestAtStation(sp.number, KG, VP, KP, NP, trainsAtDays);
        //изменить занятость станции
    }
}

void Request::unload()
{
    if(VP == 23) {
        MyDB::instance()->unloadRequestAtPVR(VP, KP, NP);
        MyDB::instance()->unloadRequestAtStation(VP, KP, NP);
    }
    else {
        MyDB::instance()->unloadRequestAtStation(VP, KP, NP);
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
