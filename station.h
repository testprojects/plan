/*
Класс, используемый в качестве структуры данных для хранения информации из базы данных
о файле станций
*/
#ifndef STATION_H
#define STATION_H

#include <QString>

struct station
{
public:
    int number;                                 //номер станции
    int type;                                   //тип станции (0-неопределён, 1-узловая, 2-промежуточная опорная, 3-тупиковая, 4-промежуточная неопорная)
    QString name;                               //наименование станции
    double latitude;                            //широта
    double longitude;                           //долгота
    int startNumber;                            //номер станции начала участка (если станция - неопорная, иначе - 0)
    int endNumber;                              //номер станции конца участка (если станция - неопорная, иначе - 0)
    int distanceTillStart;                      //км до начала участка
    int distanceTillEnd;                        //км до конца участка
    int pvrNumber;                              //номер района погрузки
    int loadingPossibilityForOperativeTraffic;  //погрузочная способность станции для оперативных перевозок (23 ВП)
    int roadNumber;
    bool operator ==(const station &) const;
    bool operator !=(const station &) const;
};

#endif // STATION_H
