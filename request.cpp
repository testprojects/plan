#include "request.h"
#include "mydb.h"
#include "pvr.h"
#include "mytime.h"
#include "programsettings.h"
#include <QDebug>
#include <QList>


bool Request::canLoad()
{
    //погрузка потока осуществляется ДО отправления потока
    //соответственно готовность к отправлению потока должна рассчитываться
    //исходя из времени начала погрузки и времени, необходимого для погрузки всех поездов этого потока


    if((VP != 23)&&(VP != 24)&&(VP!=25)) {
        qDebug() << "Погрузка рассчитывается только для 23, 24, 25 вида перевозки";
        return true;
    }
    int _VP = ProgramSettings::instance()->goodsTypes.value(KG);  //определяем, соответствует ли код груза виду перевозок
    if(VP != _VP) {
        qDebug() << QString("Код груза в заявке: %1 задан неправильно");
        return false;
    }

    //используем копию TZ
    int _TZ = TZ;
    if(_TZ < 5) _TZ = 3;
    //используем копию PK
    int _PK = PK;
    if(_PK == 0) _PK = 1;

    station s1 = MyDB::instance()->stationByNumber(SP);
    station s2 = MyDB::instance()->stationByNumber(SV);
    pvr p1 = MyDB::instance()->pvrByStationNumber(SP);
    pvr p2 = MyDB::instance()->pvrByStationNumber(SV);

    //определяем время начала погрузки исходя из времени готовности к отправлению и
    //предыдущего времени погрузки потока на этой станции (последнего поезда)
    //интервала погрузки между поездами
    //вида перевозки 23,24,25

    MyTime departureTime = MyTime(DG, CG, 0);           //время отправления первого поезда потока
    MyTime loadTime;                                    //время, необходимое на погрузку потока
    MyTime startLoadFirstTrain;                         //время начала погрузки первого поезда
    MyTime finishLoadLastTrain;                         //время окончания погрузки последнего поезда
    MyTime delayTrain;                                  //задержка между погрузкой поездов
    MyTime delayStream = MyTime(0, 0, 0);               //задержка между погрузкой потоков
    MyTime trainLoad;                                   //время, необходимое для погрузки одного поезда
    trainLoad = MyTime(0, 1, 0);

    //--------------------------------------------------------------------------------------------------------------
    //расчёт интервала между отправлениями поездов
    //текущего потока в зависимости от груза
    int k;                                              //погрузочная способность (в зависимости от вида перевозок)
    switch(VP) {
    case 23: {
        //для 23 вида перевозок погрузочная способность станции учитывается ТОЛЬКО тогда,
        //когда станция погрузки не принадлежит ПВР
        if(p1.number != 0)
            k = p1.ps;      //здесь используется погрузочная способность района
        else
            k = s1.loadingCapacity23;
        if(24%k > 0)
            delayTrain = MyTime(0, 24/k + 1, 0);
        else
            delayTrain = MyTime(0, 24/k, 0);
        break;
    }
    case 24: {
        switch(KG)
        {
        case 4: k = s1.loadingCapacity24_BP; break;
        case 5: k = s1.loadingCapacity24_GSM; break;
        default: k = s1.loadingCapacity24_PR; break;
        }
        if(24%k > 0)
            delayTrain = MyTime(0, 24/k + 1, 0);
        else
            delayTrain = MyTime(0, 24/k, 0);
        break;
    }
    case 25: {
        k = s1.loadingCapacity25;
        if(24%k > 0)
            delayTrain = MyTime(0, 24/k + 1, 0);
        else
            delayTrain = MyTime(0, 24/k, 0);
        break;
    }
    }
    //---------------------------------------------------------------------------------------------------------------

    //---------------------------------------------------------------------------------------------------------------
    //расчёт интервала между отправлениями потоков
    //в зависимости от темпа перевозок
    delayStream = MyTime(0, 0, 0);
    if(VP == 23)
        delayStream = MyTime::timeFromHours(24/_TZ);
    //---------------------------------------------------------------------------------------------------------------

    loadTime = delayTrain*(_PK - 1) + trainLoad * _PK;        //время, необходимое на погрузку всех поездов потока
    startLoadFirstTrain = departureTime - loadTime;
    finishLoadLastTrain = departureTime;

    //если время начала погрузки < 0 часов первого дня, погрузка невозможна
    //---------------------------------------------------------------------------------------------------------------
    if(startLoadFirstTrain.toHours() < 0) {
        qDebug() << QString("Нельзя погрузить поток: %1 так рано (расчитанное время начала погрузки: %2)")
                    .arg(*this)
                    .arg(startLoadFirstTrain);
        return false;
    }
    //---------------------------------------------------------------------------------------------------------------

    //считаем левую и правую границу возможности погрузки
    //---------------------------------------------------------------------------------------------------------------
    QList<std::pair<MyTime, MyTime> > listOfTrainsLoad;
    listOfTrainsLoad = MyDB::instance()->getBusy(SP, KG);
    if(listOfTrainsLoad.isEmpty()) {
        MyDB::instance()->loadAtStation(SP, KG, NP, KP, startLoadFirstTrain, finishLoadLastTrain);
        for(int i = 0; i < _PK; i++) {
            std::pair<MyTime, MyTime> times;
            times.first = startLoadFirstTrain + delayTrain * i;
            times.second = startLoadFirstTrain + delayTrain * i + trainLoad;
            m_trainsLoadingTime.append(times);
        }
        return true;
    }
    MyTime leftBorder;
    MyTime rightBorder;

    bool b_betweenLoads;//наша погрузка находится между другими погрузками, а не до или после них

    //сможем ли погрузиться ДО первого элемента
    if(startLoadFirstTrain <= listOfTrainsLoad.first().first) {
        leftBorder = MyTime(0, 0, 0);
        rightBorder = listOfTrainsLoad.first().first;
        b_betweenLoads = false;
    }
    //сможем ли погрузиться ПОСЛЕ последнего элемента
    else if(startLoadFirstTrain >= listOfTrainsLoad.last().second) {
        leftBorder = listOfTrainsLoad.last().second;
        rightBorder = MyTime(60, 0, 0);
        b_betweenLoads = false;
    }
    if(listOfTrainsLoad.count() < 2)
        return false;

    //сможем ли погрузитсья между погрузками
    for(int i = 0; i < listOfTrainsLoad.count() - 1; i++) {
        if((startLoadFirstTrain >= listOfTrainsLoad.at(i).second) && (finishLoadLastTrain <= listOfTrainsLoad.at(i+1).first)) {
            leftBorder = listOfTrainsLoad.at(i).second;
            rightBorder = listOfTrainsLoad.at(i+1).first;
            b_betweenLoads = true;
            break;
        }
    }
    if(leftBorder == rightBorder)
        return false;
    if(leftBorder < MyTime(0, 0, 0))
        return false;

    if((leftBorder <= startLoadFirstTrain) && (rightBorder >= finishLoadLastTrain))
    {
        //погрузка потока вписывается между левой и правой границей (интервал свободной погрузки в БД)
        if(VP == 23) {
            if(((rightBorder - finishLoadLastTrain) < delayStream) && (b_betweenLoads == false))
                return false;
            if(((startLoadFirstTrain - leftBorder) < delayStream) && (b_betweenLoads == false))
                return false;
        }
        MyDB::instance()->loadAtStation(SP, KG, NP, KP, startLoadFirstTrain, finishLoadLastTrain);
        for(int i = 0; i < _PK; i++) {
            std::pair<MyTime, MyTime> times;
            times.first = startLoadFirstTrain + delayTrain * i;
            times.second = startLoadFirstTrain + delayTrain * i + trainLoad;
            m_trainsLoadingTime.append(times);
        }
        return true;
    }
    //---------------------------------------------------------------------------------------------------------------

    return false;
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
