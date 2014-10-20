#ifndef REQUEST_H
#define REQUEST_H
#include <QString>
#include <QVector>
#include "ps.h"
//#define request struct Request

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
    QVector<int> OM;//обязательные станции маршрута
    int SV;//станция выгрузки
    QVector<QString> NE;//номера эшелонов
    int ER;//номер эшелона, с которым следует россыпь
    PS ps;//подвижной состав
    int PK;//количество поездов
    int TZ;//темп заданный
    int PR;//признак россыпи
    int KG;//код груза
    QString PG;//код принадлежности груза
    int OP;//особенности перевозки

    QVector<int> m_loadingPossibility;
    QVector<int> m_unloadingPossibility;
    bool canLoad();//может ли быть погружен поток на заданной станции
    QString getString() const;
};

#endif // REQUEST_H
