/*
Класс, используемый в качестве структуры данных для хранения информации из базы данных
о файле участков
*/
#ifndef SECTION_H
#define SECTION_H
#include <QMap>

struct Section
{
public:
    Section() {}
    int stationNumber1;             //номер начальной станции участка
    int stationNumber2;             //номер конечной станции участка
    float distance;                 //длина участка (на данный момент критерий поиска оптимального маршрута)
    int ps;                         //пропускная способность
    QMap<int, int> m_passingPossibilities;   //пропускные возможности
    bool limited;                   //признак лимитированного участка
    int speed;                      //скорость движения по участку [км/сутки]
    float time;                     //время движения по участку [суток] - используется в boost::dijkstraShortestPath
    bool operator ==(const Section &s) const;
    bool operator < (const Section &s) const;
    operator QString() const;
};

#endif // SECTION_H
