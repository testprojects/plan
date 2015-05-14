/*
Класс, используемый в качестве структуры данных для хранения информации из базы данных
о файле станций
*/
#ifndef STATION_H
#define STATION_H

#include <QString>
#include <QMap>

struct Station
{
public:
    Station(): number(0) {}
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
    int VO;                                     //военный округ

    int loadingCapacity23;                      //погрузочная способность станции для оперативных перевозок (23 ВП)
    int loadingCapacity24_BP;
    int loadingCapacity24_GSM;
    int loadingCapacity24_PR;
    int loadingCapacity25;

    int loadingPossibilities23[60];             //погрузочная способность станции для оперативных перевозок (23 ВП)
    int loadingPossibilities24_BP[60];
    int loadingPossibilities24_GSM[60];
    int loadingPossibilities24_PR[60];
    int loadingPossibilities25[60];

                                                //занятые станции по погрузке <день, количество поездов>
                                                //если ключа <день> в списке не найдено, возвращается
                                                //значение по умолчанию, равное соответствующей погрузочной способности
    int roadNumber;
    bool operator ==(const Station &) const;
    bool operator !=(const Station &) const;

public:
    operator QString () const;
};

#endif // STATION_H
