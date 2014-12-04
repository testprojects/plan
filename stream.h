#ifndef STREAM_H
#define STREAM_H
#include <QVector>
#include "section.h"
#include "station.h"
#include "echelon.h"

//#include "request.h"
class Request;
class Graph;

class Stream
{
private:
//    QVector<int> m_loadingPossibility;                    //занятая погрузочная возможность по дням

public:
    Request* m_sourceRequest;                               //заявка, от которой формируется файл маршрута
    Graph* m_graph;                                         //граф
    QList<section> m_passedSections;                      //пройденные станции с обновлёнными пропускными возможностями
    QList<station> m_passedStations;                      //пройденные станции
    QList<echelon> m_echelones;
    QVector<QVector<int> > m_busyPassingPossibilities;      //занятая пропускная возможность на каждый участок по дням
    //
    int m_temp;                                             //темп рассчётный
    bool m_planned;                                         //спланирован ли поток
    bool m_failed;
    QString m_failString;
    MyTime m_departureTime;
    MyTime m_arrivalTime;

public:
    Stream();
    Stream(Request *request, Graph *gr);
    QVector<QVector<int> > calculatePV(const QList<echelon> &echelones);//рассчитывает пропускную возможность, занимаемую для каждого из участков по дням
    void fillSections();                                    //заполняет участки согласно пройденным станциям
    bool canBePlanned();           //считает может ли поток быть спланирован (записать погрузочную и пропускную возможности в базу, если он может быть спланирован?)
    bool canPassSections(const QList<section> &passedSections, const QVector<QVector<int> > &busyPassingPossibilities, MyTime timeOffset = MyTime(0, 0, 0), QList<section> *fuckedUpSections = 0);
    //смещение НАДО ЗАДАВАТЬ = ВРЕМЕНИ ГОТОВНОСТИ К ОТПРАВЛЕНИЮ
    bool canBeShifted(int days, int hours, int minutes, QList<section> *fuckedUpSections);    //может ли спланированная заявка быть сдвинута (принимаются также и отрицательные значения)
    bool canBeShifted(const MyTime &offsetTime, QList<section> *fuckedUpSections);
    void shiftStream(int days, int hours);
    void shiftStream(const MyTime &offsetTime);
    int length();
    void setPlanned(bool planned = true) {m_planned = planned;}
    bool planned() {return m_planned;}
    QString print(bool b_PSInfo = false, bool b_RouteInfo = true,
                  bool b_BusyPossibilities = false, bool b_echelonsTimes = false);
    void setFailed(QString errorString);
    QList<float> distancesBetweenStations();
    QList<echelon> fillEchelones(MyTime departureTime, int VP /*вид перевозок*/,int PK/*колво поездов*/, int TZ /*темп*/
                                 , const QList<float> &distancesBetweenStations, const QList<int> &sectionsSpeed);//функция заполнения эшелонов.
    //та же самая функция, только считает время прохождения станций по маршруту с точностью до минут
    QList<echelon> fillEchelonesInMinutes(MyTime departureTime, int VP /*вид перевозок*/,int PK/*колво поездов*/, int TZ /*темп*/
                                 , const QList<float> &distancesBetweenStations, const QList<int> &sectionsSpeed);//функция заполнения эшелонов.

    QList<PS> dividePS(const Request &req);
};

#endif // STREAM_H
