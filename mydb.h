#ifndef MYDB_H
#define MYDB_H
#include <QSqlDatabase>
#include <QList>
#include "station.h"
#include "section.h"
#include "pvr.h"
#include "request.h"
#include "pvr.h"
#include "stream.h"
#include "mytime.h"

class stationBusy;


class MyDB //СИНГЛТОН. Все обращения к методам идут через статический метод MyDB::instance()->
{
public:
    //-------------------------------ОБЩАЯ--------------------------------------------------------------------------
    QSqlDatabase db;
    static MyDB* instance();
    //создание соединения с БД
    bool createConnection (QString databaseName = "postgres", QString hostName = "localhost", QString userName = "postgres", QString password = "postgres");
    //загружаем данные из БД в память
    void readDatabase();
    QMap<int, QString> roads(QString pathToRoads);//считывает с файла ассоциативный массив (№ дороги - наименование)
    //---------------------------------------------------------------------------------------------------------------

    //--------------------------------СТАНЦИИ------------------------------------------------------------------------
    station stationByNumber(int n);
    //---------------------------------------------------------------------------------------------------------------

    //--------------------------------УЧАСТКИ------------------------------------------------------------------------
    //дополняет таблицу участков (БД) из текстового файла
    void createTableSections();
    void addSectionsFromFile(QString sectionsFilePath = "./UCH.txt");
    QString convertSections(QString oldFormatSection);
    void resetPassingPossibility();//сброс пропускных возможностей всех участков в БД
    void resetPassingPossibility(int station1, int station2);//сброс пропускных возможностей конкретного участка
    section sectionByStations(station s1, station s2); //возвращает участок, даже если на входе неопорные станции!

    //---------------------------------------------------------------------------------------------------------------

    //--------------------------------ЗАЯВКИ--------------------------------------------------------------------------
    void createTableRequests();
    void addRequestsFromFile(QString requestsFilePath = "./requests.txt",
                             int format = 0/*0 - формат WZAYV, 1 - формат District*/);//загружает заявки с файла в БД
    QList<Request> requestsFromFile(QString requestsFilePath,
                                    int format = 0/*0 - формат WZAYV, 1 - формат District*/);//читает заявки с файла без записи в БД
    QString convertFromWzayvRequest(QString wzayvFormatRequest);//преобразовывает заявку с WZAYV.EXE в мой формат
    QString convertFromDistrictRequest(QString districtFormatRequest);//преобразует заявку с Жениной проги в мой формат
    Request parseRequest(QString MyFormatRequest);

    Request requestByStations(QString stationLoadName, QString stationUnloadName, QStringList requiredStationsNames);//сформировать заявку по имени станций погрузки и выгрузки и обязательным станциям маршрута
    Request requestByStations(QString stationLoadName, QString stationUnloadName);//загрузить заявку по имени станций погрузки и выгрузки
    Request request(int VP, int KP, int NP);//загрузить заявку из БД (вид перевозок, код получателя, номер потока)
    QList<Request> requestsBySPRoadNumber(int roadNumber);
    QVector<Request> requests(int VP = 0 /*вид перевозок*/, int KP = 0 /*код получателя*/);
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ПВР----------------------------------------------------------------------------
    void createTablePVR();
    void addPVRFromFile(QString requestsFilePath = "./pvr.txt");
    QString convertPVR(QString oldFormatPVR);
    pvr pvrByStationNumber(int n);
    void resetLoadingPossibility();//сброс погрузочной возможности ПВР
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ПОТОКИ-------------------------------------------------------------------------
    void createTableStreams();
    Stream stream(const Request& req);
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ЗАНЯТОСТЬ СТАНЦИЙ--------------------------------------------------------------
    void createTableStationsBusy();
    void loadAtStation(int stationNumber, int KG, int NP, int KP, MyTime startLoadFirstTrain, MyTime finishLoadLastTrain);
    QList<std::pair<MyTime, MyTime> > getBusy(int stationNumber, int KG);
    //----------------------------------------------------------------------------------------------------------------

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
