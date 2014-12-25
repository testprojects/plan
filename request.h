#ifndef REQUEST_H
#define REQUEST_H
#include <QString>
#include <QStringList>
#include <QVector>
#include "ps.h"
//#define request struct Request
enum _loading_type {e23, eBP, eGSM, ePR, e25};

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

    _loading_type loading_type; //какой из вариантов погрузки будем использовать

    bool canLoad();//может ли быть погружен поток на заданной станции
    QString getString() const;

    bool operator ==(Request req) { return ((this->VP == req.VP)&&(this->KP == req.KP)&&(this->NP == req.NP));}
};

#endif // REQUEST_H
