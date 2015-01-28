#ifndef MYDB_H
#define MYDB_H
#include <QSqlDatabase>
#include <QList>
#include "station.h"
#include "section.h"
#include "pvr.h"
#include "request.h"
#include "stream.h"
#include "mytime.h"

class MyDB //СИНГЛТОН. Все обращения к public-членам идут через статический метод MyDB::instance()->
{
    friend class Stream;
public:
    //-------------------------------ОБЩАЯ--------------------------------------------------------------------------
    static MyDB* instance();
    //создание соединения с БД
    bool createConnection (QString databaseName = "postgres", QString hostName = "localhost", QString userName = "postgres", QString password = "postgres", QString driver = "QPSQL");
    void checkTables();
    //загружаем данные из БД в память
    void cacheIn();
    void cacheOut();
    //---------------------------------------------------------------------------------------------------------------

    //----------------------------PUBLIC-МЕТОДЫ----------------------------------------------------------------------
public:
    Station* stationByNumber(int n);
    Section* sectionByNumbers(int st1, int st2);//возвращает участок, даже если на входе - неопорные станции
    Request* request(int VP, int KP, int NP);
    QVector<Request*> requests(int VP, int KP = 0);//при указании KP = 0, ищет заявки для всех получателей с заданным VP
    PVR* pvr(int PN);
    Stream* stream(int VP, int KP, int NP);
    QVector<Station*> freeStationsInPVR(int stNumber, const QMap<int, int> &trainsByDays, int KG);
    QVector<Station*> stations() {return m_stations;}
    QVector<Section*> sections() {return m_sections;}
    QVector<PVR*> pvrs() {return m_pvrs;}
    QVector<Request*> requests() {return m_requests;}
    QVector<Stream*> streams() {return m_streams;}
    void addToCache(Stream *s);
    //--------------------------------BASE---------------------------------------------------------------------------
public:
    void BASE_deleteStreamsFromDB(int VP = 0, int KP = 0, int NP = 0);
    void BASE_deleteRequestsFromDB(int VP = 0, int KP = 0, int NP = 0);
    void BASE_loadRequestsFromFileWZAYV(QString strPathToFile);
    void BASE_loadRequestsFromFileDISTRICT(QString strPathToFile);
    void BASE_deleteStationsFromDB(int startStationNumber, int endStationNumber = startStationNumber);
    void BASE_deleteSectionFromDB(int stationNumber1, int stationNumber2);
    void BASE_addStationToDB(Station *st);
    void BASE_addSectionToDB(Section *sec);
    void BASE_addRequestToDB(Request *req);
    void BASE_addStationsFromFile(QString strPathToFile);
    void BASE_addSectionsFromFile(QString strPathToFile);
    void BASE_updateStationInDB(Station *st);
    void BASE_updateSectionInDB(Section *sec);
    void BASE_updateRequestInDB(Request *req);
    QMap<int, int> BASE_stationLoad(int stationNumber);
    QMap<int, int> BASE_stationLoadByKG(int stationNumber, int KG);
    QMap<int, int> BASE_stationLoadByStream(int VP, int KP, int NP);
    QMap<int, int> BASE_PVRLoad(int pvrNumber);
    QMap<int, int> BASE_PVRLoadByStream(int VP, int KP, int NP);
    QVector<QMap<int, int> > BASE_sectionsLoadByStream(int VP, int KP, int NP);
    QMap<int, int> BASE_sectionLoad(int stationNuber1, int stationNumber2);
    QVector<Echelon> BASE_echelonsOfStream(int VP, int KP, int NP);
    QVector<Station> BASE_stationsInSection(int stationNumber1, int stationNumber2);
    QVector<Station> BASE_stationsInPVR(int pvrNumber);
    QVector<Section> BASE_sectionsByStationNumber(int stationNumber);
    PVR BASE_PVRByStationNumber(int stationNumber);
    int BASE_roadByStationNumber(int stationNumber);
    QVector<Station> BASE_stationsInRoad(int roadNumber);
    //---------------------------------------------------------------------------------------------------------------

    //--------------------------------СТАНЦИИ------------------------------------------------------------------------
private:
    bool isStationFree(int stNumber, const QMap<int, int> &trainsByDays, int KG);
    Station* DB_getStationByNumber(int n);
    QVector<Station*> DB_getStations();
    //---------------------------------------------------------------------------------------------------------------

    //--------------------------------УЧАСТКИ------------------------------------------------------------------------
private:
    QString convertSections(QString oldFormatSection);
    void DB_createTableSections();
    void DB_addSectionsFromFile(QString sectionsFilePath = "./UCH.txt");
    Section* DB_getSectionByStationsNumbers(int s1, int s2);
    QVector<Section*> DB_getSections();
    //---------------------------------------------------------------------------------------------------------------

    //--------------------------------ЗАЯВКИ--------------------------------------------------------------------------
private:
    void addRequestsFromFile(QString requestsFilePath = "./requests.txt",
                             int format = 0/*0 - формат WZAYV, 1 - формат District*/);//загружает заявки с файла в БД
    QList<Request> requestsFromFile(QString requestsFilePath,
                                    int format = 0/*0 - формат WZAYV, 1 - формат District*/);//читает заявки с файла без записи в БД
    QString convertFromWzayvRequest(QString wzayvFormatRequest);//преобразовывает заявку с WZAYV.EXE в мой формат
    QString convertFromDistrictRequest(QString districtFormatRequest);//преобразует заявку с Жениной проги в мой формат
    Request parseRequest(QString MyFormatRequest);
    void DB_createTableRequests();
    Request* DB_getRequest(int VP, int KP, int NP);
    QVector<Request*> DB_getRequests();
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ПВР----------------------------------------------------------------------------
private:
    void addPVRFromFile(QString requestsFilePath = "./pvr.txt");
    QString convertPVR(QString oldFormatPVR);
    void DB_createTablePVRs();
    PVR* DB_getPVR(int n);
    QVector<PVR*> DB_getPVRs();
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ПОТОКИ-------------------------------------------------------------------------
private:
    void DB_createTableStreams();
    Stream *DB_getStream(int VP, int KP, int NP);
    QVector<Stream*> DB_getStreams(int VP = 0, int KP = 0, int NP = 0);
    void DB_clearStream(int VP, int KP, int NP);
    void DB_updateStream(int VP, int KP, int NP, int LT, const QVector<Station*> &passedStations);
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ЗАНЯТОСТЬ СТАНЦИЙ--------------------------------------------------------------
private:
    void DB_createTableStationsLoad();
    void DB_cropTableStationsLoad();
    void DB_updateStationsLoad(int VP, int KP, int NP, int SN, int KG, QMap<int, int> loadDays);
    void DB_removeStationsLoad(int VP, int KP, int NP);
    QMap<int, int> DB_getStationsLoad(int VP, int KP, int NP, int KG);//для наполнения streams
    QMap<int, int> DB_getStationsLoad(int VP, int KP, int NP);//суммирует занятость со всеми кодами груза
    QMap<int, int> DB_getStationsLoad(int SN, int KG);
    QMap<int, int> DB_getStationsLoad(int SN, QString typeKG);//для наполнения stations
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ЗАНЯТОСТЬ ПВР------------------------------------------------------------------
private:
    void DB_createTablePVRLoad();
    void DB_cropTablePVRLoad();
    void DB_updatePVRLoad(int VP, int KP, int NP, int PN, QMap<int, int> loadDays);
    void DB_removePVRLoad(int VP, int KP, int NP);
    QMap<int, int> DB_getPVRLoad(int VP, int KP, int NP, int PN);
    QMap<int, int> DB_getPVRLoad(int PN);
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------ЗАНЯТОСТЬ УЧАСТКОВ-------------------------------------------------------------
private:
    void DB_createTableSectionsLoad();
    void DB_cropTableSectionsLoad();
    QMap<int, int> DB_getSectionsLoad(int VP, int KP, int NP, int S1, int S2);
    QMap<int, int> DB_getSectionsLoad(int S1, int S2);
    void DB_updateSectionLoad(int VP, int KP, int NP, int S1, int S2, QMap<int, int> loads);
    //----------------------------------------------------------------------------------------------------------------

    //---------------------------------------ЭШЕЛОНЫ------------------------------------------------------------------
    //эшелоны без указателей, т.к. внутренняя структура класса Echelon не может однозначно определить к какому потоку
    //он относится и поэтому иметь общий архив эшелонов не имеет смысла, т.к. они необходимы только для загрузки
    //объектов Stream из БД
private:
    void DB_createTableEchelones();
    void DB_cropTableEchelones();
    Echelon DB_getEchelon(int VP, int KP, int NP, int NE);
    QVector<Echelon> DB_getEchelones(int VP, int KP, int NP);
    void DB_updateEchelones(int VP, int KP, int NP, int NE, QString NA, PS ps, QMap<int, int> hoursArrival);
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
