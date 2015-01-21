#ifndef REQUEST_H
#define REQUEST_H
#include <QString>
#include <QStringList>
#include <QVector>
#include "ps.h"

class Request
{
public:
    int VP;//вид перевозок
    int KP;//код получателя
    int CN;//число начала перезовок
    int MN;//месяц начала перевозок
    int GN;//год начала перевозок
    int NP;//номер потока
    QString NA;//наименование и количество перевозимого
    QString SH;//номер штата
    QString OT;//отправитель
    QString PY;//получатель
    QString MG;//месяц готовности
    int DG;//день готовности
    int CG;//час готовности
    int KS;//категория срочности
    int SP;//станция погрузки
    QList<int> OM;//обязательные станции маршрута
    int SV;//станция выгрузки
    QStringList NE;//номера эшелонов
    int ER;//номер эшелона, с которым следует россыпь
    PS ps;//подвижной состав
    int PK;//количество поездов
    int TZ;//темп заданный
    int PR;//признак россыпи
    int KG;//код груза
    QString PG;//код принадлежности груза
    int OP;//особенности перевозки
    int PL;//признак планирования по ж/д
    int BE;//вес перевозимого
//    int RA;//код района погрузки (ПВР)
//    int RV;//код района выгрузки (ПВР)

    //может ли быть погружен (0 - нет, 1 - погрузка на станции, 2 - погрузка на ПВР)
    //p_loadAtDays - <день погрузки, количество поездов> - карта, куда будет записана занятость, если нужна
    //anotherStationNumber - альтернативный номер станции (если 23ВП и заявка погружена на другой станции ПВРа)
    //её нужно сохранить в БД
    int canLoad(QMap<int, int> *p_loadAtDays = NULL, int* alternativeStationNumber = NULL);
    operator QString() const;

    bool operator ==(Request req) { return ((this->VP == req.VP)&&(this->KP == req.KP)&&(this->NP == req.NP));}
};

#endif // REQUEST_H
