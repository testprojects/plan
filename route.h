#ifndef ROUTE_H
#define ROUTE_H
#include <QVector>
#include "section.h"
#include "station.h"
#include "echelon.h"

//#include "request.h"
class Request;
class Graph;

class Route
{
private:
//    QVector<int> m_loadingPossibility;                    //занятая погрузочная возможность по дням

public:
    Request* m_sourceRequest;                               //заявка, от которой формируется файл маршрута
    Graph* m_graph;                                         //граф
    QVector<section> m_passedSections;                      //пройденные станции с обновлёнными пропускными возможностями
    QVector<station> m_passedStations;                      //пройденные станции
    QVector<echelon> m_echelones;
    QVector<QVector<int> > m_busyPassingPossibilities;      //занятая пропускная возможность на каждый участок по дням
    int m_temp;                                             //темп рассчётный
    bool m_planned;                                         //спланирован ли поток мысли
    MyTime m_departureTime;
    MyTime m_arrivalTime;

public:
    Route();
    Route(Request *request, Graph *gr);
    QVector<QVector<int> > calculatePV(const QVector<echelon> &echelones);                                     //рассчитывает пропускную возможность, необходимую для каждого из участков по дням
    void fillSections();                                    //заполняет участки согласно пройденным станциям
    bool canBePlanned(bool bWriteInBase = false);           //считает может ли поток быть спланирован (записать погрузочную и пропускную возможности в базу, если он может быть спланирован?)
    bool canPassSections(const QVector<section> &passedSections, const QVector<QVector<int> > &busyPassingPossibilities, MyTime timeOffset = MyTime(0, 0, 0), QVector<section> *fuckedUpSections = 0);
    bool canBeShifted(int days, int hours, int minutes);    //может ли спланированная заявка быть сдвинута (принимаются также и отрицательные значения)
    bool canBeShifted(const MyTime &offsetTime);
    int length();
    void setPlanned(bool planned = true) {m_planned = planned;}
    bool planned() {return m_planned;}
    QString print();
};

#endif // ROUTE_H
