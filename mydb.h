#ifndef MYDB_H
#define MYDB_H
#include <QSqlDatabase>
#include "station.h"
#include "section.h"
#include "pvr.h"
#include "request.h"
#include "pvr.h"
#include "stream.h"


class MyDB
{

public:
    QSqlDatabase db;
    static MyDB* instance();
    //создание соединения с БД
    bool createConnection (QString databaseName = "postgres", QString hostName = "localhost", QString userName = "postgres", QString password = "postgres");

    //дополняет таблицу участков (БД) из текстового файла
    void addSectionsFromFile(QString sectionsFilePath = "./UCH.txt");
    void createSectionsTable();
    QString parseSections(QString oldFormatSection);

    //дополняет таблицу заявок (БД) из внешнего текстового файла, выгруженного для ПЭВМ с программы WZAY
    void addRequestsFromFile(QString requestsFilePath = "./requests.txt");
    void createRequestsTable();
    QString parseRequest(QString oldFormatRequest);

    //таблица ПВР'ов
    void addPVRFromFile(QString requestsFilePath = "./pvr.txt");
    void createPVRTable();
    QString parsePVR(QString oldFormatPVR);

    //загружаем данные из БД в память
    void readDatabase();
    void closeDatabase(MyDB &db, bool save = true);

    //сбросить пропускную возможность участков в дефолт
    void resetPassingPossibility();
    void resetLoadingPossibility();

    //
    station stationByNumber(int n);
    section sectionByStations(station s1, station s2); //возвращает участок, даже если на входе неопорные станции!
    pvr pvrByStationNumber(int n);
    Request requestByStationsName(QString stationLoadName, QString stationUnloadName, QStringList requiredStationsNames);//сформировать заявку по имени станций погрузки и выгрузки и обязательным станциям маршрута
    Request requestByStationsName(QString stationLoadName, QString stationUnloadName);//сформировать заявку по имени станций погрузки и выгрузки
    Request requestByStationNumber(int stationLoadNumber, int stationUnloadNumber);//сформировать заявку по номерам станций погрузки и выгрузки
    Request request(int VP, int KP, int NP);//загрузить заявку из БД (вид перевозок, код получателя, номер потока)
    QList<Request> requestsBySPRoadNumber(int roadNumber);
    QVector<Request> requests(int VP = 0 /*вид перевозок*/, int KP = 0 /*код получателя*/);
    QMap<int, QString> roads(QString pathToRoads);

private:
    static MyDB *_self;
    MyDB();
    MyDB(const MyDB&);
    virtual ~MyDB();
    MyDB& operator=(const MyDB&);

private:
    QList<station> m_stations;//станции ж/д сети
    QList<section> m_sections;//участки ж/д сети
    QList<pvr> m_pvrs;//ПВР'ы
    QList<Stream> m_routes;//высчитанные маршруты

public:
    QList<station>* stations() {return &m_stations;}
    QList<section>* sections() {return &m_sections;}
    QList<pvr>* pvrs() {return &m_pvrs;}
    QList<Stream>* routes() {return &m_routes;}

};

#endif // MYDB_H
