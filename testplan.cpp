#include "testplan.h"
#include "graph.h"
#include "mydb.h"
#include "programsettings.h"
#include <QtTest/QtTest>

TestPlan::TestPlan(QObject *parent) :
    QObject(parent)
{
    ProgramSettings::instance()->readSettings();
    if(!MyDB::instance()->createConnection("C:\\plan\\docs\\plan_old.db", "localhost", "artem", "1", "QSQLITE")) {
        qDebug() << "connection failed";
    }
    MyDB::instance()->checkTables();
    MyDB::instance()->BASE_deleteStreamsFromDB();
    MyDB::instance()->cacheIn();

    gr = new Graph(MyDB::instance()->stations(), MyDB::instance()->sections());
}

void TestPlan::distanceBetweenStations()
{
    //тест определения длины маршрута:

    //Станции находятся на разных участках
    //1 вариант - (опорная станция, опорная станция)        ЛУГА 1(101072009)    - БОЛОГОЕ-МОСКОВСКОЕ(101050009)
    //2 вариант - (опорная станция, неопорная станция)      ЛУГА 1(101072009)    - ДОБЫВАЛОВО(101055107)
    //3 вариант - (неопорная станция, опорная станция)      ТОЛМАЧЕВО(101072102) - БОЛОГОЕ-МОСКОВСКОЕ(101050009)
    //4 вариант - (неопорная станция, неопорная станция)    ТОЛМАЧЕВО(101072102) - ДОБЫВАЛОВО(101055107)

    //Станции находятся на одном участке
    //5 вариант - (опорная станция, опорная станция)        ДНО(101056701)      - ВАЛДАЙ(101055200)
    //6 вариант - (опорная станция, неопорная станция)      ДНО(101056701)      - ЛЮБНИЦА(101055709)
    //7 вариант - (неопорная станция, опорная станция)      ВОЛОТ(101056504)    - ВАЛДАЙ(101055200)
    //8 вариант - (неопорная станция, неопорная станция)    ВОЛОТ(101056504)    - ЛЮБНИЦА(101055709)

    int stStart1 = 101072009, stEnd1 = 101050009;
    int stStart2 = 101072009, stEnd2 = 101055107;
    int stStart3 = 101072102, stEnd3 = 101050009;
    int stStart4 = 101072102, stEnd4 = 101055107;

    int stStart5 = 101056701, stEnd5 = 101055200;
    int stStart6 = 101056701, stEnd6 = 101055709;
    int stStart7 = 101056504, stEnd7 = 101055200;
    int stStart8 = 101056504, stEnd8 = 101055709;

    const int distance1 = 371; //36+60+74+131+20+50=371 (ЛУГА1, БАТЕЦКАЯ, НОВГОРОД-НА-ВОЛХОВКЕ, ЧУДОВО-МОСК, ОКУЛОВКА, УГЛОВКА, БОЛОГОЕ-МОСК)
    const int distance2 = 353; //36+98+207+50-38=353 (ЛУГА1, БАТЕЦКОЕ, ДНО, ВАЛДАЙ, ДОБЫВАЛОВО)
    const int distance3 = 385; //14+36+60+74+131+20+50=385 (ТОЛМАЧЕВО, ЛУГА1, БАТЕЦКАЯ, НОВГОРОД-НА-ВОЛХОВКЕ, ЧУДОВО-МОСК, ОКУЛОВКА, УГЛОВКА, БОЛОГОЕ-МОСК)
    const int distance4 = 367; //14+36+98+207+50-38=367 (ТОЛМАЧЕВО, ЛУГА1, БАТЕЦКОЕ, ДНО, ВАЛДАЙ, ДОБЫВАЛОВО)

    const int distance5 = 207; //207 (ДНО, ВАЛДАЙ)
    const int distance6 = 171; //207-36=171 (ДНО, ЛЮБНИЦА)
    const int distance7 = 161; //207-46=161 (ВОЛОТ, ВАЛДАЙ)
    const int distance8 = 125; //207-36-46=125 (ВОЛОТ, ЛЮБНИЦА)

    QVector<Station*> route;
    int evaluated_distance;

    //[1]
    gr->optimalPath(stStart1, stEnd1, &route);
    evaluated_distance = gr->distanceBetweenStations(0, route.count() - 1, route);
    route.clear();
    QCOMPARE(distance1, evaluated_distance);
    //[2]
    gr->optimalPath(stStart2, stEnd2, &route);
    evaluated_distance = gr->distanceBetweenStations(0, route.count() - 1, route);
    route.clear();
    QCOMPARE(distance2, evaluated_distance);
    //[3]
    gr->optimalPath(stStart3, stEnd3, &route);
    evaluated_distance = gr->distanceBetweenStations(0, route.count() - 1, route);
    route.clear();
    QCOMPARE(distance3, evaluated_distance);
    //[4]
    gr->optimalPath(stStart4, stEnd4, &route);
    evaluated_distance = gr->distanceBetweenStations(0, route.count() - 1, route);
    route.clear();
    QCOMPARE(distance4, evaluated_distance);
    //[5]
    gr->optimalPath(stStart5, stEnd5, &route);
    evaluated_distance = gr->distanceBetweenStations(0, route.count() - 1, route);
    route.clear();
    QCOMPARE(distance5, evaluated_distance);
    //[6]
    gr->optimalPath(stStart6, stEnd6, &route);
    evaluated_distance = gr->distanceBetweenStations(0, route.count() - 1, route);
    route.clear();
    QCOMPARE(distance6, evaluated_distance);
    //[7]
    gr->optimalPath(stStart7, stEnd7, &route);
    evaluated_distance = gr->distanceBetweenStations(0, route.count() - 1, route);
    route.clear();
    QCOMPARE(distance7, evaluated_distance);
    //[8]
    gr->optimalPath(stStart8, stEnd8, &route);
    evaluated_distance = gr->distanceBetweenStations(0, route.count() - 1, route);
    route.clear();
    QCOMPARE(distance8, evaluated_distance);
}

void TestPlan::canLoad()
{
    Request *req = new Request();
    req->SP = 101582005;    //ЛИСКИ (6 поездов в сутки на погрузку ГСМ)
    req->PK = 25;           //25 поездов
    req->TZ = 25;            //с темпом 5 поездов в сутки и интервалом в двое суток между погрузками пяти поездов
    req->DG = 18;           //день готовности - 18
    req->CG = 21;           //час готовности - 21
    req->VP = 24;           //24 вид перевозок
    req->KG = 5;            //код груза. 24_GSM

    //задержка между погрузками поездов = 24/5 = 4.8 часа
    //[поезд - день погрузки]
    //расчёт вручную времени погрузки поездов
    //delay   = 24/5=4.8[часов]
    //delayBT = 48[часов]
    //[1  - 429   - 17]
    //[2  - 433.8 - 18]
    //[3  - 438.6 - 18]
    //[4  - 443.4 - 18]
    //[5  - 448.2 - 18]

    //[6  - 496.2 - 20]
    //[7  - 501   - 20]
    //[8  - 505.8 - 21]
    //[9  - 510.6 - 21]
    //[10 - 515.4 - 21]

    //[11 - 563.4 - 23]
    //[12 - 568.2 - 23]
    //[13 - 573.0 - 23]
    //[14 - 577.8 - 24]
    //[15 - 582.6 - 24]

    //[16 - 630.6 - 26]
    //[17 - 635.4 - 26]
    //[18 - 640.2 - 26]
    //[19 - 645.0 - 26]
    //[20 - 649.8 - 27]

    //[21 - 697.8 - 29]
    //[22 - 702.6 - 29]
    //[23 - 707.4 - 29]
    //[24 - 712.2 - 29]
    //[25 - 717.0 - 29]
    QMap<int, int> calculatedLoadAtDays;  //занятая погрузочная возможность по дням
    calculatedLoadAtDays.insert(17, 1);
    calculatedLoadAtDays.insert(18, 4);
    calculatedLoadAtDays.insert(20, 2);
    calculatedLoadAtDays.insert(21, 3);
    calculatedLoadAtDays.insert(23, 3);
    calculatedLoadAtDays.insert(24, 2);
    calculatedLoadAtDays.insert(26, 4);
    calculatedLoadAtDays.insert(27, 1);
    calculatedLoadAtDays.insert(29, 5);


    QMap<int, int> loadAtDays;  //занятая погрузочная возможность по дням
    int res = req->canLoad(&loadAtDays);
    delete req;
    QCOMPARE(res, static_cast<int>(LOAD_STATION));
    //сравниваем полученные значения
    foreach (int key, calculatedLoadAtDays.keys()) {
        QCOMPARE(calculatedLoadAtDays.value(key), loadAtDays.value(key));
    }
    //и в обратном порядке
    foreach (int key, loadAtDays.keys()) {
        QCOMPARE(calculatedLoadAtDays.value(key), loadAtDays.value(key));
    }
}

QTEST_MAIN(TestPlan)
