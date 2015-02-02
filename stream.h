#ifndef STREAM_H
#define STREAM_H
#include <QVector>
#include "section.h"
#include "station.h"
#include "echelon.h"

//#include "request.h"
class Request;
class Graph;
enum LoadType {LOAD_NO = 0, LOAD_STATION, LOAD_PVR};

class Stream
{
public:
    Request* m_sourceRequest;                               //заявка, от которой формируется файл маршрута
    QVector<Section*> m_passedSections;                      //пройденные станции с обновлёнными пропускными возможностями
    QVector<Station*> m_passedStations;                      //пройденные станции
    QVector<Echelon> m_echelones;
    QVector<QMap<int, int> > m_busyPassingPossibilities;      //занятая пропускная возможность на каждый участок по дням
    QMap<int, int> m_busyLoadingPossibilities;              //занятая погрузочная возможность
    LoadType m_loadType;

    MyTime m_departureTime;//время отправления первого эшелона с первой станции маршрута
    MyTime m_arrivalTime;//время прибытия последнего эшелона на последнюю станцию маршрута

public:
    Stream();
    Stream(Request *request);
    void cacheOut();
    bool wasChangedDuringSession();
    QVector<QMap<int, int> > calculatePV(const QVector<Echelon> &echelones);//рассчитывает пропускную возможность, занимаемую для каждого из участков по дням
    static QVector<Section*> fillSections(QVector<Station*> passedStations);                                    //заполняет участки согласно пройденным станциям
    //может ли поток пройти участки маршрута (0 - не может пройти и нельзя сместить; 1 - не может пройти но можно сместить; 2 - может пройти)
    int canPassSections(QVector<Section*> passedSections, QVector<QMap<int, int> > busyPassingPossibilities,
                        MyTime timeOffset = MyTime(0, 0, 0), QVector<Section*> *fuckedUpSections = 0) const;
    void passSections(const QVector<Section*> &passedSections, const QVector<QMap<int, int> > &busyPassingPossibilities,
                      const MyTime timeOffset = MyTime(0, 0, 0));
    //смещение НАДО ЗАДАВАТЬ = ВРЕМЕНИ ГОТОВНОСТИ К ОТПРАВЛЕНИЮ
    bool canBeShifted(int hours, QVector<Section *> *fuckedUpSections);    //может ли спланированная заявка быть сдвинута (принимаются также и отрицательные значения)
    bool canBeShifted(const MyTime &offsetTime, QVector<Section *> *fuckedUpSections);
    void shiftStream(int hours);
    void shiftStream(const MyTime &offsetTime);
    int length();
    QString print(bool b_PSInfo = false, bool b_RouteInfo = true,
                  bool b_BusyPossibilities = false, bool b_echelonsTimes = false);
    QList<float> distancesBetweenStations();
    //та же самая функция, только считает время прохождения станций по маршруту с точностью до минут
    QVector<Echelon> fillEchelonesInMinutes(MyTime departureTime, int VP /*вид перевозок*/,int PK/*колво поездов*/, int TZ /*темп*/
                                 , const QList<float> &distancesBetweenStations, const QList<int> &sectionsSpeed);//функция заполнения эшелонов.

    QList<PS> dividePS(const Request &req);
    QStringList divideNA(const Request &req);
};

#endif // STREAM_H
