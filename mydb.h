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

class StationBusy;


class MyDB //СИНГЛТОН. Все обращения к public-членам идут через статический метод MyDB::instance()->
{
public:
    //-------------------------------ОБЩАЯ--------------------------------------------------------------------------
    static MyDB* instance();
    //создание соединения с БД
    bool createConnection (QString databaseName = "postgres", QString hostName = "localhost", QString userName = "postgres", QString password = "postgres", QString driver = "QPSQL");
    //загружаем данные из БД в память
    void cacheIn();
    QMap<int, QString> roads(QString pathToRoads);//считывает с файла ассоциативный массив (№ дороги - наименование)
    //---------------------------------------------------------------------------------------------------------------

    //--------------------------------СТАНЦИИ------------------------------------------------------------------------
public:
    Station *stationByNumber(int n);
    bool isStationFree(int stNumber, const QMap<int, int> &trainsByDays, int KG);
private:
    Station* DB_stationByNumber(int n);
    QVector<Station*> DB_stations();
    //---------------------------------------------------------------------------------------------------------------

    //--------------------------------УЧАСТКИ------------------------------------------------------------------------
public:
    //дополняет таблицу участков (БД) из текстового файла
    Section* sectionByNumbers(int st1, int st2);//возвращает участок, даже если на входе - неопорные станции
    QString convertSections(QString oldFormatSection);
private:
    void DB_createTableSections();
    void DB_addSectionsFromFile(QString sectionsFilePath = "./UCH.txt");
    Section* DB_sectionByStationsNumbers(int s1, int s2);
    QVector<Section*> DB_sections();
    //---------------------------------------------------------------------------------------------------------------

    //--------------------------------ЗАЯВКИ--------------------------------------------------------------------------
public:
    Request* request(int VP, int KP, int NP);
private:
    void addRequestsFromFile(QString requestsFilePath = "./requests.txt",
                             int format = 0/*0 - формат WZAYV, 1 - формат District*/);//загружает заявки с файла в БД
    QList<Request> requestsFromFile(QString requestsFilePath,
                                    int format = 0/*0 - формат WZAYV, 1 - формат District*/);//читает заявки с файла без записи в БД
    QString convertFromWzayvRequest(QString wzayvFormatRequest);//преобразовывает заявку с WZAYV.EXE в мой формат
    QString convertFromDistrictRequest(QString districtFormatRequest);//преобразует заявку с Жениной проги в мой формат
    Request parseRequest(QString MyFormatRequest);
    void DB_createTableRequests();
    Request DB_request(int VP, int KP, int NP);//загрузить заявку из БД (вид перевозок, код получателя, номер потока)
    QList<Request> DB_requestsBySPRoadNumber(int roadNumber);
    QVector<Request> DB_requests(int VP = 0 /*вид перевозок*/, int KP = 0 /*код получателя*/);
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ПВР----------------------------------------------------------------------------
public:
    void addPVRFromFile(QString requestsFilePath = "./pvr.txt");
    QString convertPVR(QString oldFormatPVR);
    QList<Station> freeStationsInPVR(int stNumber, const QMap<int, int> &trainsByDays, int KG);
private:
    void DB_createTablePVR();
    PVR DB_PVRByNumber(int n);
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ПОТОКИ-------------------------------------------------------------------------
public:
    Stream stream(int VP, int KP, int NP);
private:
    void DB_createTableStreams();
    Stream *DB_getStream(int VP, int KP, int NP);
    QVector<Stream*> DB_getStreams();
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ЗАНЯТОСТЬ СТАНЦИЙ--------------------------------------------------------------
private:
    void DB_createTableStationLoad();
    void DB_cropTableStationLoad();
    void DB_insertStationLoad(int stationNumber, int KG, int VP, int KP, int NP, QMap<int, int> loadDays);
    void DB_removeStationLoad(int VP, int KP, int NP);
    QMap<int, int> DB_getStationLoad(int VP, int KP, int NP, int KG);//для наполнения streams
    QMap<int, int> DB_getStationLoad(int SN, int KG);
    QMap<int, int> DB_getStationLoad(int SN, QString typeKG);//для наполнения stations
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ЗАНЯТОСТЬ ПВР------------------------------------------------------------------
private:
    void DB_createTablePVRLoad();
    void DB_cropTablePVRLoad();
    void DB_insertPVRLoad(int VP, int KP, int NP, int PN, int KG, QMap<int, int> loadDays);
    void DB_removePVRLoad(int VP, int KP, int NP);
    QMap<int, int> DB_getPVRLoad(int VP, int KP, int NP, int PN, int KG);
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ЗАНЯТОСТЬ УЧАСТКОВ-------------------------------------------------------------
private:
    void DB_createTableSectionsLoad();
    void DB_cropTableSectionsLoad();
    QMap<int, int> DB_getSectionsLoad(int VP, int KP, int NP, int S1, int S2);
    QMap<int, int> DB_getSectionsLoad(int S1, int S2);
    void DB_insertSectionLoad(int VP, int KP, int NP, int S1, int S2, QMap<int, int> loads);
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------------ЭШЕЛОНЫ------------------------------------------------------------------
private:
    void DB_createTableEchelones();
    void DB_cropTableEchelones();
    Echelon *DB_echelon(int VP, int KP, int NP, int NE);
    QVector<Echelon> DB_echelones(int VP, int KP, int NP);
    //----------------------------------------------------------------------------------------------------------------

private:
    static MyDB *_self;
    MyDB();
    MyDB(const MyDB&);
    virtual ~MyDB();
    MyDB& operator=(const MyDB&);
    QSqlDatabase db;

private:
    QVector<Station*> m_stations;//станции ж/д сети
    QVector<Section*> m_sections;//участки ж/д сети
    QVector<PVR*> m_pvrs;//ПВР'ы
    QVector<Stream*> m_streams;//высчитанные маршруты
    QVector<Request*> m_requests;//заявки
};

#endif // MYDB_H
