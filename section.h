/*
Класс, используемый в качестве структуры данных для хранения информации из базы данных
о файле участков
*/
#ifndef SECTION_H
#define SECTION_H

struct section
{
public:
    int stationNumber1;             //номер начальной станции участка
    int stationNumber2;             //номер конечной станции участка
    float distance;                 //длина участка (на данный момент критерий поиска оптимального маршрута)
    int ps;                         //пропускная способность
    int passingPossibilities[60];   //пропускные возможности
    bool limited;                   //признак лимитированного участка
    int speed;                      //скорость движения по участку [км/сутки]
    bool operator ==(const section &s) const;
    bool operator < (const section &s) const;
};

#endif // SECTION_H
