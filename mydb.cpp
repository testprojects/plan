#include "mydb.h"
#include "mytime.h"
#include "programsettings.h"
#include "graph.h"
#include <QtSql>
#include <QString>
#include <QtDebug>


MyDB::MyDB()
{
}

MyDB::~MyDB()
{
    if(_self)
        delete _self;
}

//-----------------------------------ОБЩИЕ МЕТОДЫ------------------------------------------------------

MyDB* MyDB::instance()
{
    if(!_self) {
        _self = new MyDB();
    }
    return _self;
}

MyDB* MyDB::_self = 0;

bool MyDB::createConnection(QString databaseName, QString hostName, QString userName, QString password, QString driver)
{
    db = QSqlDatabase::addDatabase(driver);
    db.setHostName(hostName);
    db.setDatabaseName(databaseName);
    db.setUserName(userName);
    db.setPassword(password);
    bool ok = db.open();
    return ok;
}

void MyDB::cacheIn()
{
    //загружаем данные об объектах из БД в ОП
    m_stations = DB_getStations();
    m_sections = DB_getSections();
    m_pvrs = DB_getPVRs();
    m_requests = DB_getRequests();
    m_streams =  DB_getStreams();
}

void MyDB::cacheOut()
{

}

QMap<int, QString> MyDB::roads(QString pathToRoads)
{
    QFile roadsFile(pathToRoads);
    QMap<int, QString> roadsMap;
    if(!roadsFile.open(QFile::ReadOnly)) {
        qDebug() << "Unable to find roads file in path: " << pathToRoads;
        return roadsMap;
    }
    QTextStream in(&roadsFile);
    while(!in.atEnd()) {
        QString buf = in.readLine();
        int number = buf.left(buf.indexOf(';')).toInt();
        buf = buf.remove(0, buf.indexOf(';') + 1);
        QString name = buf;
        roadsMap.insert(number, buf);
    }
    return roadsMap;
}

//---------------------------------------------------------------------------------------------------------


//----------------------------------------------СТАНЦИИ----------------------------------------------------
Station* MyDB::stationByNumber(int n)
{
    if(n == 0) {
        qDebug() << "Станции с номером 0 не существует";
        exit(1);
    }
    foreach (Station* st, m_stations) {
        if(st->number == n)
            return st;
    }
    return NULL;
}

Station* MyDB::DB_getStationByNumber(int n)
{
    if(n == 0) {
        qDebug() << "Станции с номером 0 не существует";
        exit(1);
    }
    QSqlQuery query(db);
    QString strQuery = "SELECT * FROM stations WHERE sk = \'" + QString::number(n) + "\'";
    query.exec(strQuery);
    if(!query.first()) {
        qDebug() << "Не удалось найти станцию с номером " << n << " в БД";
        exit(1);
    }

    Station *st = new Station();
    double LAT, LONG;

    QString buf = query.value("SH").toString();
    LAT = buf.left(3).toDouble() + buf.mid(3, 2).toDouble() / 60 + buf.right(2).toDouble() / 3600;
    buf = query.value("DL").toString();
    LONG = buf.left(3).toDouble() + buf.mid(3, 2).toDouble() / 60 + buf.right(2).toDouble() / 3600;

    st->number = n;
    st->name = query.value("PN").toString();
    st->latitude = LAT;
    st->longitude = LONG;
    st->type = query.value("ST").toInt();
    st->startNumber = query.value("SB").toInt();
    st->endNumber = query.value("SE").toInt();
    st->distanceTillStart = query.value("LB").toInt();
    st->distanceTillEnd = query.value("LK").toInt();
    st->pvrNumber = query.value("NR").toInt();
    st->roadNumber = query.value("SD").toInt();
    //ПОГРУЗОЧНЫЕ СПОСОБНОСТИ СТАНЦИИ
    //23
    st->loadingCapacity23 = query.value("DR").toInt();
    //24 - БП
    int bp = query.value("BO").toInt();
    if(bp == 0)
        st->loadingCapacity24_BP = 2;
    else
        st->loadingCapacity24_BP = bp;
    //24 - ГСМ
    int gsm = query.value("GO").toInt();
    if(gsm == 0)
        st->loadingCapacity24_GSM = 4;
    else
        st->loadingCapacity24_GSM = gsm;
    //24 - ПРОЧИЕ ГРУЗЫ
    int pr = query.value("PO").toInt();
    if(pr == 0)
        st->loadingCapacity24_PR = 2;
    else
        st->loadingCapacity24_PR = pr;
    //25
    int mob = query.value("MO").toInt();
    if(mob == 0)
        st->loadingCapacity25 = 2;
    else
        st->loadingCapacity25 = mob;
    //---------------------------------

    //ПОГРУЗОЧНЫЕ ВОЗМОЖНОСТИ СТАНЦИИ
    for(int i = 0; i < 60; i++) {
        st->loadingPossibilities23[i] = st.loadingCapacity23;
        st->loadingPossibilities24_BP[i] = st.loadingCapacity24_BP;
        st->loadingPossibilities24_GSM[i] = st.loadingCapacity24_GSM;
        st->loadingPossibilities24_PR[i] = st.loadingCapacity24_PR;
        st->loadingPossibilities25[i] = st.loadingCapacity25;
    }

    QMap<int, int> busy23 = DB_getStationsLoad(st->number, "23");
    QMap<int, int> busy24_BP = DB_getStationsLoad(st->number, "24_BP");
    QMap<int, int> busy24_GSM = DB_getStationsLoad(st->number, "24_GSM");
    QMap<int, int> busy24_PR = DB_getStationsLoad(st->number, "24_PR");
    QMap<int, int> busy25 = DB_getStationsLoad(st->number, "25");

    for(int i = 0; i < 60; i++) {
        st->loadingPossibilities23[day] -= busy23.value(day, 0);
        st->loadingPossibilities24_BP[day] -= busy24_BP(day, 0);
        st->loadingPossibilities24_GSM[day] -= busy24_GSM(day, 0);
        st->loadingPossibilities24_PR[day] -= busy24_PR(day, 0);
        st->loadingPossibilities25[day] -= busy25(day, 0);
    }

    return st;
}

QVector<Station*> MyDB::DB_getStations()
{
    QVector<Station*> sts;
    QSqlQuery query(QSqlDatabase::database());
    query.exec("SELECT * FROM stations");
    while(query.next()) {
        int sk = query.value("SK").toInt();
        Station *st = DB_getStationByNumber(sk);
        assert(st);
        sts.append(st);
    }
    return sts;
}

bool MyDB::isStationFree(int stNumber, const QMap<int, int> &trainsByDays, int KG)
{
    if(stNumber == 0)
        return false;
    //ищем все записи с заданным номером станции и с кодом груза в пределах типа груза
    QString type = ProgramSettings::instance()->goodsTypesDB.value(KG);
    QList<int> KGs = ProgramSettings::instance()->goodsTypesDB.keys(type);
    QString strQuery = QString("SELECT * FROM stationload WHERE SN = %1 AND KG IN (").arg(stNumber);
    if(KGs.isEmpty()) {
        qDebug() << QString("Код груза: %1. Не найдено соответствующего типа груза в реестре").arg(KG);
        exit(15);
    }
    foreach (int n, KGs) {
        strQuery += QString("%1, ").arg(n);
    }
    strQuery.chop(2);
    strQuery += ")";

    //выбираем записи
    QSqlQuery query(QSqlDatabase::database());
    query.exec(strQuery);

    //выбираем дни, в пределах которых нам нужно смотреть занятость
    QList<int> days = trainsByDays.keys();

    //суммируем данные по загруженности на заданные дни
    QMap<int, int> busyAtDays;
    foreach (int n, days) {
        busyAtDays.insert(n, 0);
    }

    while(query.next()) {
        foreach (int n, days) {
            int trains = busyAtDays.value(n);
            busyAtDays.insert(n, trains + query.value(QString("PV_%1").arg(n)).toInt());
        }
    }

    //если хотя бы один элемент busyAtDays > хотя бы одного соответствующего элемента trainsByDays
    //значит на этой станции нельзя погрузить нужное количество поездов с таким типом груза
    foreach (int n, days) {
        if(busyAtDays.value(n) > trainsByDays.value(n))
            return false;
    }
    return true;
}

//----------------------------------------------------------------------------------------------------

//--------------------------------------------УЧАСТКИ-------------------------------------------------
Section* MyDB::sectionByNumbers(int st1, int st2)
{
    Station *s1, *s2;
    s1 = stationByNumber(st1);
    s2 = stationByNumber(st2);
    assert(s1);
    assert(s2);
    //если обе станции неопорные
    if((s1->type == 4)&&(s2->type == 4)) {
        s2 = stationByNumber(s1->endNumber);
        s1 = stationByNumber(s1->startNumber);
    }

    //если одна из станций - неопорная
    if(s1->type == 4) {
        if(s2->number == s1->startNumber) {
            s1 = stationByNumber(s1->endNumber);
        }
        if(s2->number == s1->endNumber) {
            s1 = stationByNumber(s1->startNumber);
        }
    }
    if(s2->type == 4) {
        if(s1->number == s2->startNumber) {
            s2 = stationByNumber(s2->endNumber);
        }
        if(s1->number == s2->endNumber) {
            s2 = stationByNumber(s2->startNumber);
        }
    }

    foreach (Section* sec, m_sections) {
        if((sec->stationNumber1 == s1->number) && (sec->stationNumber2 == s2->number))
            return sec;
    }
    return NULL;
}

void MyDB::DB_createTableSections()
{
    if(db.tables().contains("sections")) {
        qDebug() << "table sections already exists";
        return;
    }
    QSqlQuery query(db);
    if(query.exec("CREATE TABLE IF NOT EXISTS sections(    ku int NOT NULL,    "
                                                          "kk int NOT NULL,   "
                                                          "tu smallint,            "
                                                          "ro smallint,            "
                                                          "sp smallint,"
                                                          "lu smallint,"
                                                          "vu smallint,"
                                                          "so smallint,"
                                                          "sd smallint,"
                                                          "ys smallint,"
                                                          "ti smallint,"
                                                          "vt smallint,"
                                                          "mv smallint,"
                                                          "lp smallint,"
                                                          "lm smallint,"
                                                          "pv smallint[]"
                  ")")) {
        qDebug() << "table sections successfully created";
    }
    else {
        qDebug() << "unable to create table sections " << query.lastError().text();
    }
}

void MyDB::DB_addSectionsFromFile(QString sectionsFilePath)
{
    QFile sectionsFile(sectionsFilePath);
    if(!sectionsFile.open(QFile::ReadOnly)) {
        qDebug() << "Unable to find sections file in path: " << sectionsFilePath;
        return;
    }
    QTextStream in(&sectionsFile);
    QStringList sectionsList;
    while(!in.atEnd()) {
        //парсим строку формата заявок WZAYV в строку с форматом, согласно нашей таблицы заявок
        QString newStr = convertSections(in.readLine());
        sectionsList.append(newStr);
    }
    QSqlQuery query(db);
    foreach (QString tmp, sectionsList) {
        if(query.exec("INSERT INTO sections VALUES(" + tmp + ")")) {
        }
        else {
            qDebug() << "Error, while trying to add sections: " << query.lastError().text();
        }
    }
}

QString MyDB::convertSections(QString oldFormatSection)
{
    QString newStr = oldFormatSection;
    int pvStartPos = 0;
    for(int i = 0; i < 15 /*15 полей перед массивом ПВ*/; i++) {
        pvStartPos = newStr.indexOf(',', pvStartPos + 1);
    }
    newStr = newStr.insert(pvStartPos + 1, "ARRAY[");
    newStr.append("]");
    return newStr;
}

Section* MyDB::DB_getSectionByStationsNumbers(int s1, int s2)
{
    Section *s;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery(QString("SELECT * FROM sections WHERE ku = %1 AND kk = %2")
                     .arg(s1)
                     .arg(s2));
    if(query.exec(strQuery)) {
        s = new Section();
        s->distance = query.value("LU").toInt();
        s->limited = query.value("LM").toInt();
        s->ps = query.value("SP").toInt();
        s->stationNumber1 = query.value("KU").toInt();
        s->stationNumber2 = query.value("KK").toInt();
        s->speed = query.value("VU").toInt();
        s->time = (float)s.distance / (float)s.speed * 24.0 * 60.0;
        QString strPV = query.value("PV").toString();
        strPV.remove(0, 1);
        strPV.chop(1);
        for(int i = 0; i < 60; i++) {
            int ind = strPV.indexOf(',');
            if(ind == 0) s.passingPossibilities[i] = strPV.toInt();
            else {
                s->passingPossibilities[i] = strPV.left(ind).toInt();
                strPV.remove(0, ind + 1);
            }
        }
        //учитываем занятую пропускную возможность
        QMap<int, int> load = DB_getSectionsLoad(s->stationNumber1, s->stationNumber2);
        for(int i = 0; i < 60; i++) {
            s->passingPossibilities[i] -= load.value(i, 0);
        }
        return s;
    }
}

QVector<Section*> MyDB::DB_getSections()
{
    QVector<Section*> secs;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM sections");
    if(query.exec(strQuery)) {
        int st1 = query.value("ku").toInt();
        int st2 = query.value("kk").toInt();
        Section* s = DB_getSectionByStationsNumbers(st1, st2);
        assert(s);
        secs.append(s);
    }
    return secs;
}

//-----------------------------------------------------------------------------------------------

//------------------------------------------ЗАЯВКИ-----------------------------------------------
Request* MyDB::request(int VP, int KP, int NP)
{
    foreach (Request* req, m_requests) {
        if((req->VP == VP) && (req->KP == KP) && (req->NP == NP))
            return req;
    }
    return NULL;
}

QVector<Request*> MyDB::requests(int VP, int KP)
{
    QVector<Request*> reqs;
    foreach (Request* req, m_requests) {
        if(KP != 0) {
            if((req->VP == VP) && (req->KP == KP))
                reqs.append(req);
        }
        else {
            if(req->VP == VP)
                reqs.append(req);
        }
    }
}

void MyDB::DB_createTableRequests()
{
    if(db.tables().contains("requests")) {
        qDebug() << "table requests already exists";
        return;
    }
    QSqlQuery query(db);
    if(query.exec("CREATE TABLE IF NOT EXISTS requests(VP smallint NOT NULL, "
                                                      "KP smallint NOT NULL, "
                                                      "CN smallint, "
                                                      "MN smallint, "
                                                      "GN smallint, "
                                                      "NP smallint NOT NULL, "
                                                      "NA character varying, "
                                                      "SH character varying, "
                                                      "OT character varying, "
                                                      "PY character varying, "
                                                      "MO character varying, "
                                                      "deno smallint, "
                                                      "co smallint, "
                                                      "KS smallint, "
                                                      "SP int, "
                                                      "OM character varying, "
                                                      "SV int, "
                                                      "NE character varying, "
                                                      "ER character varying, "
                                                      "C1 int, "
                                                      "C2 int, "
                                                      "L1 int, "
                                                      "L2 int, "
                                                      "K1 int, "
                                                      "K2 int, "
                                                      "X1 int, "
                                                      "X2 int, "
                                                      "P1 int, "
                                                      "P2 int, "
                                                      "V1 int, "
                                                      "V2 int, "
                                                      "Y1 int, "
                                                      "Y2 int, "
                                                      "D1 int, "
                                                      "D2 int, "
                                                      "R1 int, "
                                                      "R2 int, "
                                                      "CH int, "
                                                      "PK smallint, "
                                                      "TZ smallint, "
                                                      "PR smallint, "
                                                      "KG smallint, "
                                                      "PG character varying, "
                                                      "OP smallint, "
                                                      "PL int"
                                                      "BE int" //вес перевозимого [т.]
                  ")")) {
        qDebug() << "table requests successfully created";
    }
    else {
        qDebug() << "unable to create table: requests. " << query.lastError().text();
    }
}

void MyDB::addRequestsFromFile(QString requestsFilePath, int format)
{
    QFile requestsFile(requestsFilePath);
    if(!requestsFile.open(QFile::ReadOnly)) {
        qDebug() << "Unable to find requests file in path: " << requestsFilePath;
        return;
    }
    QTextStream in(&requestsFile);
    QStringList requestsList;
    while(!in.atEnd()) {
        //парсим строку формата заявок WZAYV в строку с форматом, согласно нашей таблицы заявок
        QString newStr;
        if(format == 0)
            newStr = convertFromWzayvRequest(in.readLine());
        else if(format == 1)
            newStr = convertFromDistrictRequest(in.readLine());
        else {
        qDebug() << "Unknown format of request";
        exit(1);
        }
        requestsList.append(newStr);
    }
    QSqlQuery query(db);
    foreach (QString tmp, requestsList) {
        if(query.exec("INSERT INTO requests VALUES(" + tmp + ")")) {
        }
        else {
            qDebug() << "Error, while trying to add requests: " << query.lastError().text();
        }
    }
}

QList<Request> MyDB::requestsFromFile(QString requestsFilePath, int format)
{
    QList<Request> reqs;
    QFile requestsFile(requestsFilePath);
    if(!requestsFile.open(QFile::ReadOnly)) {
        qDebug() << "Unable to find requests file in path: " << requestsFilePath;
        return reqs;
    }
    QTextStream in(&requestsFile);
    while(!in.atEnd()) {
        //парсим строку формата заявок WZAYV в строку с форматом, согласно нашей таблицы заявок
        QString inStr = in.readLine(), outStr;

        //разделяем поля, помещаем последовательно в fields
        if(format == 0) outStr = convertFromWzayvRequest(inStr);
            else if(format == 1) outStr = convertFromDistrictRequest(inStr);
        Request r = parseRequest(outStr);
        reqs.append(r);
    }
    return reqs;
}

QString MyDB::convertFromWzayvRequest(QString wzayvFormatRequest)
{
    QStringList fields;
    QString newStr;
    //разделяем поля, помещаем последовательно в fields
    fields = wzayvFormatRequest.split(';');
    //заполняем newStr из полученных полей
    newStr += fields[0].mid(2,2) + ", ";//вид перевозок
    newStr += fields[0].mid(4,3) + ", ";//код получателя
    newStr += QString("15") + ", ";//число начала перевозок
    newStr += QString("12") + ", ";//месяц начала перевозок
    newStr += QString("2014") + ", ";//год начала перевозок
    newStr += fields[1] + ", ";//номер потока
    newStr += QString("\'") + fields[26] + QString("\', ");//наименование и кол-во перевозимого
    newStr += QString("\'") + fields[27] + QString("\', ");//номер штата
    newStr += QString("\'") + fields[28] + QString("\', ");//отправитель
    newStr += QString("\'") + fields[29] + QString("\', ");//получатель
    newStr += QString("\'") + fields[31] + QString("\', ");//месяц готовности
    newStr += QString::number((fields[8].toInt() / 24 + 1)) + ", ";//день готовности
    newStr += QString::number((fields[8].toInt() % 24)) + ", ";//час готовности
    newStr += fields[4].mid(2, 1) + ", ";//категория срочности
    newStr += fields[5] + ", ";//станция погрузки
    //обязательные станции маршрута [82-90]---------------------
    QString OM;
    for(int i = 82; i <= 90; i++) {
        if(fields[i] != "0")
            OM += fields[i] + ",";
    }
    OM.chop(1);
    newStr += QString("\'") + OM + QString("\', ");
    //----------------------------------------------------------
    newStr += fields[11] + ", ";//станция выгрузки
    newStr += QString("\'") + fields[32] + QString("\', ");//номера эшелонов
    newStr += "\'" + fields[33] + "\'" + ", ";//номер эшелона, с которым следует россыпь
    //подвижной состав------------------------------------------
    newStr += fields[36] + ", ";//пассажирские ч
    newStr += fields[38] + ", ";//пассажирские з
    newStr += fields[40] + ", ";//людские ч
    newStr += fields[42] + ", ";//людские з
    newStr += fields[44] + ", ";//крытые ч
    newStr += fields[46] + ", ";//крытые з
    newStr += fields[48] + ", ";//кухни ч
    newStr += fields[50] + ", ";//кухни з
    newStr += fields[52] + ", ";//платформы ч
    newStr += fields[54] + ", ";//платформы з
    newStr += fields[56] + ", ";//полувагоны ч
    newStr += fields[58] + ", ";//полувагоны з
    newStr += fields[60] + ", ";//спецвагоны ч
    newStr += fields[62] + ", ";//спецвагоны з
    newStr += fields[64] + ", ";//ледники ч
    newStr += fields[66] + ", ";//ледники з
    newStr += fields[68] + ", ";//цистерны ч
    newStr += fields[70] + ", ";//цитсерны з
    newStr += fields[72] + ", ";//всего
    //----------------------------------------------------------
    newStr += fields[91] + ", ";//количество поездов
    newStr += fields[74] + ", ";//темп заданный
    newStr += fields[2] + ", ";//признак россыпи
    newStr += fields[25] + ", ";//код груза
    newStr += "\'" + fields[34] + "\', ";//код принадлежности груза
    newStr += fields[78] + ", ";//особенности перевозки
    newStr += fields[80] + ", ";//признак планирования по ж/д
    newStr += fields[99] + "";//вес перевозимого

    int i = 0;
    foreach (QString tmp, fields) {
        qDebug() << i++ << ")" << tmp;
    }
    qDebug() << newStr;
    return newStr;
}

QString MyDB::convertFromDistrictRequest(QString districtFormatRequest)
{
    QStringList fields;
    QString newStr;
    //разделяем поля, помещаем последовательно в fields
    districtFormatRequest = districtFormatRequest.toUtf8();
    fields = districtFormatRequest.split(';');
    fields.removeLast();
    //добавляем признак планирования
    fields.append("0");
    //добавляем недостающие знаменатели
    for(int i = 20; i < 37; i+=2)
        fields.insert(i, "0");
    //заполняем newStr из полученных полей
    //всё идёт один к одному до ПС
    newStr = fields.join(';');
    newStr.append(';');
    newStr.append("0;");//вес перевозмиого

    int i = 0;
    foreach (QString tmp, fields) {
        qDebug() << ++i << ")" << tmp;
    }
    qDebug() << newStr;
    return newStr;
}

Request MyDB::parseRequest(QString MyFormatRequest)
{
    Request r;
    QStringList list = MyFormatRequest.split(';');
    //убираем последний элемент, т.к. метод QString::split()
    //учитывает последнюю ; в качестве разделителя перед следующим элементом
    list.removeLast();

    int k = 0;
    r.VP = list.at(k++).toInt();
    r.KP = list.at(k++).toInt();
    r.CN = list.at(k++).toInt();
    r.MN = list.at(k++).toInt();
    r.GN = list.at(k++).toInt();
    r.NP = list.at(k++).toInt();
    r.NA = list.at(k++);
    r.SH = list.at(k++);
    r.OT = list.at(k++);
    r.PY = list.at(k++);
    r.MG = list.at(k++);
    r.DG = list.at(k++).toInt();
    r.CG = list.at(k++).toInt();
    r.KS = list.at(k++).toInt();
    r.SP = list.at(k++).toInt();
    QStringList strOM = list.at(k++).split(',');
    if(!((strOM.count() == 1) && (strOM.first().isEmpty()))) {
        foreach (QString str, strOM) {
            r.OM.append(str.toInt());
        }
    }
    r.SV = list.at(k++).toInt();
    //
    QStringList listNE = list.at(k++).split(',');
    r.NE = listNE;
    r.ER = list.at(k++).toInt();

    PS ps;
    ps.pass = list.at(k++).toInt();
    ps.pass+= list.at(k++).toInt();
    ps.luds = list.at(k++).toInt();
    ps.luds+= list.at(k++).toInt();
    ps.krit = list.at(k++).toInt();
    ps.krit+= list.at(k++).toInt();
    ps.kuhn = list.at(k++).toInt();
    ps.kuhn+= list.at(k++).toInt();
    ps.plat = list.at(k++).toInt();
    ps.plat+= list.at(k++).toInt();
    ps.polu = list.at(k++).toInt();
    ps.polu+= list.at(k++).toInt();
    ps.spec = list.at(k++).toInt();
    ps.spec+= list.at(k++).toInt();
    ps.ledn = list.at(k++).toInt();
    ps.ledn+= list.at(k++).toInt();
    ps.cist = list.at(k++).toInt();
    ps.cist+= list.at(k++).toInt();
    ps.total = list.at(k++).toInt();
    r.ps = ps;

    r.PK = list.at(k++).toInt();
    r.TZ = list.at(k++).toInt();
    r.PR = list.at(k++).toInt();
    r.KG = list.at(k++).toInt();
    r.PG = list.at(k++);
    r.OP = list.at(k++).toInt();
    r.PL = list.at(k++).toInt();
    r.BE = list.at(k++).toInt();

    return r;
}

Request MyDB::requestByStations(QString stationLoadName, QString stationUnloadName, QStringList requiredStationsNames)
{
    Request tmp;
    QSqlQuery query(MyDB::instance()->db);
    QString str = "SELECT sk FROM stations WHERE pn = \'" + stationLoadName + "\'";
    query.exec(str);
    tmp.SP = query.value(0).toInt();

    str = "SELECT sk FROM stations WHERE pn = \'" + stationUnloadName + "\'";
    query.exec(str);
    tmp.SV = query.value(0).toInt();

    foreach (QString strStation, requiredStationsNames) {
        QString strQuery = "SELECT sk FROM stations WHERE pn = \'" + strStation + "\'";
        query.exec(strQuery);
        int n_st = query.value(0).toInt();
        tmp.OM.append(n_st);
    }

    return tmp;
}

Request MyDB::requestByStations(QString stationLoadName, QString stationUnloadName)
{
    Request tmp;
    QSqlQuery query(MyDB::instance()->db);
    QString str = "SELECT sk FROM stations WHERE pn = \'" + stationLoadName + "\'";
    query.exec(str);
    query.first();
    tmp.SP = query.value(0).toInt();
    query.finish();

    str = "SELECT sk FROM stations WHERE pn = \'" + stationUnloadName + "\'";
    query.exec(str);
    query.first();
    tmp.SV = query.value(0).toInt();

    return tmp;
}

Request* MyDB::DB_getRequest(int VP, int KP, int NP)
{
    Request *req;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM requests WHERE VP = %1 AND KP = %2 AND NP = %3")
            .arg(VP)
            .arg(KP)
            .arg(NP);
    if(query.exec(strQuery)) {
        req = new Request();
        //ключ
        req->VP = query.value("VP").toInt();
        req->KP = query.value("KP").toInt();
        req->NP = query.value("NP").toInt();

        //начало перевозок
        req->CN = query.value("CN").toInt();
        req->GN = query.value("GN").toInt();
        req->MN = query.value("MN").toInt();
        //готовность
        req->MG = query.value("mo").toString();
        req->DG = query.value("deno").toInt();
        req->CG = query.value("co").toInt();

        //наименование перевозимого
        req->NA = query.value("NA").toString();

        //станции погрузки, разгрузки
        req->SP = query.value("SP").toInt();
        req->SV = query.value("SV").toInt();
        QString strOM = query.value("OM").toString();
        while(!strOM.isEmpty()) {
            if(strOM.left(9).toInt() != 0)
                req->OM.append(strOM.left(9).toInt());
            if(strOM.size() == 9) strOM.remove(0, 9);
            else
                strOM.remove(0, 10);
        }

        //количество поездов
        req->PK = query.value("PK").toInt();
        //темп заданный
        req->TZ = query.value("TZ").toInt();

        req->KS = query.value("KS").toInt();
        req->PR = query.value("PR").toInt();
        req->KG = query.value("KG").toInt();
        req->PG = query.value("PG").toString();
        req->OP = query.value("OP").toInt();
        //признак планирования
        req->PL = query.value("PL").toInt();
        //вес перевозимого
        req->BE = query.value("BE").toInt();

        //подвижной состав
        PS ps;
        ps.pass += query.value("C1").toInt();
        ps.pass += query.value("C2").toInt();

        ps.luds += query.value("L1").toInt();
        ps.luds += query.value("L2").toInt();

        ps.krit += query.value("K1").toInt();
        ps.krit += query.value("K2").toInt();

        ps.kuhn += query.value("X1").toInt();
        ps.kuhn += query.value("X2").toInt();

        ps.plat += query.value("P1").toInt();
        ps.plat += query.value("P2").toInt();

        ps.polu += query.value("V1").toInt();
        ps.polu += query.value("V2").toInt();

        ps.spec += query.value("Y1").toInt();
        ps.spec += query.value("Y2").toInt();

        ps.ledn += query.value("D1").toInt();
        ps.ledn += query.value("D2").toInt();

        ps.cist += query.value("R1").toInt();
        ps.cist += query.value("R2").toInt();

        ps.total += query.value("CH").toInt();

        int sum = ps.pass + ps.luds + ps.krit + ps.kuhn + ps.plat + ps.polu + ps.spec + ps.ledn + ps.cist;

        if(sum != ps.total)
            qDebug() << QString::fromUtf8("%1: Сумма подвижного состава (%2) не сходится с указанной в заявке (%3)")
                        .arg((QString)req)
                        .arg(sum)
                        .arg(ps.total);

        req->ps = ps;

        //    QString SH;//номер штата
        //    QString OT;//отправитель
        //    QString PY;//получатель

        //номера эшелонов
        QString strNE = query.value("NE").toString();
        req->NE = strNE.split(',');
        //    int ER;//номер эшелона, с которым следует россыпь
    }
    return req;
}

QVector<Request> MyDB::DB_getRequests()
{
    QVector<Request*> reqs;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = "SELECT * FROM requests";
    query.exec(strQuery);
    while(query.next()) {
        int vp = query.value("VP");
        int kp = query.value("KP");
        int np = query.value("NP");
        Request *req = DB_getRequest(vp, kp, np);
        assert(req);
        reqs.append(req);
    }
    return reqs;
}
//-------------------------------------------------------------------------------------------------------------

//--------------------------------------------------ПВР--------------------------------------------------------
PVR* MyDB::pvr(int PN)
{
    foreach (PVR* p, m_pvrs) {
        if(p->number == PN)
            return p;
    }
    return NULL;
}

void MyDB::DB_createTablePVR()
{
    if(QSqlDatabase::database().tables().contains("pvr")) {
        qDebug() << "table pvr already exists";
        return;
    }
    QSqlQuery query(QSqlDatabase::database());
    if(query.exec("CREATE TABLE IF NOT EXISTS pvr(NR smallint NOT NULL PRIMARY KEY, "
                  "IR character varying, "
                  "PR smallint, "
                  "SP smallint[60]"
                  ")")) {
        qDebug() << "table pvr successfully created";
    }
    else {
        qDebug() << "unable to create table pvr. " << query.lastError().text();
    }
}

void MyDB::addPVRFromFile(QString PVRFilePath)
{
    QFile PVRFile(PVRFilePath);
    if(!PVRFile.open(QFile::ReadOnly)) {
        qDebug() << "Unable to find pvr file in path: " << PVRFilePath;
        return;
    }
    QTextStream in(&PVRFile);
    QStringList PVRList;
    while(!in.atEnd()) {
        QString newStr = convertPVR(in.readLine());
        if(PVRList.contains(newStr)) continue;
        PVRList.append(newStr);
    }
    QSqlQuery query(QSqlDatabase::database());
    foreach (QString tmp, PVRList) {
        if(query.exec("INSERT INTO pvr VALUES(" + tmp + ")")) {
            qDebug() << "pvr successfully added";
        }
        else {
            qDebug() << "Error, while trying to add pvr: " << query.lastError().text();
        }
    }

}

QString MyDB::convertPVR(QString oldFormatPVR)
{
    QString newPVR;
    QStringList fields;
    for(int i = 0; i < 6; i++) {
        QString tmp;
        int ind = oldFormatPVR.indexOf(';');
        tmp = oldFormatPVR.left(ind);
        fields << tmp;
        oldFormatPVR.remove(0, ind + 1);
    }
    fields << oldFormatPVR;
    newPVR = fields[0] + ", \'" + fields[1] + "\', " + fields[4] + ", \'{";
    for(int i = 0; i < 60; i++) {
        newPVR += fields[4];
        newPVR += ", ";
    }
    newPVR.chop(2);
    newPVR += "}\'";
    return newPVR;
}

PVR *MyDB::DB_getPVR(int n)
{
    QSqlQuery query(QSqlDatabase::database());
    if(n == 0)
        return PVR();
    QString strQuery = "SELECT * FROM pvr WHERE nr = " + QString::number(n) + ";";

    if(!query.exec(strQuery))
        qDebug() << query.lastError().text();
    if(!query.first())
        qDebug() << query.lastError().text();
    PVR *p = new PVR();
    p->number = query.value("nr").toInt();
    p->name = query.value("ir").toString();
    p->ps = query.value("pr").toInt();
    //ПОГРУЗОЧНАЯ ВОЗМОЖНОСТь
    QMap<int, int> load = DB_getPVRLoad(n);
    for(int i = 0; i < 60; i++)
        p->pv[i] = p->ps - load.value(i, 0);

    return p;
}

QVector<PVR*> MyDB::DB_getPVRs()
{
    QVector<PVR*> pvrs;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM pvrs");
    if(query.exec(strQuery)) {
        int pn = query.value("PN");
        pvr *p = DB_getPVR(pn);
        assert(p);
        pvrs.append(p);
    }
    return pvrs;
}

QList<Station> MyDB::freeStationsInPVR(int stNumber, const QMap<int, int> &trainsByDays, int KG)
{
    QList<Station> freeStationsList;
    Station st = MyDB::instance()->stationByNumber(stNumber);
    if (st.pvrNumber == 0) {
        qDebug() << QString("Станция %1 не принадлежит ПВР").arg(st);
        return QList<Station>();
    }
    PVR p = MyDB::instance()->DB_getPVR(st.pvrNumber);

    //формируем список станций, входящих в ПВР
    QList<Station> pvrStations;
    foreach (Station st, m_stations) {
        if(st.pvrNumber == p.number)
            pvrStations.append(st);
    }

    //оставляем только свободные
    foreach (Station st, pvrStations) {
        if((MyDB::instance()->isStationFree(st.number, trainsByDays, KG)))
            freeStationsList.append(st);
    }

    //
    return freeStationsList;
}

void MyDB::resetLoadingPossibility()
{
    QSqlQuery query(QSqlDatabase::database());
    QString str = "UPDATE pvr SET SP = ARRAY[PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR]";
    if(!query.exec(str)) {
        qDebug() << "Погрузочная способность не сброшена: " << query.lastError().text();
    }
}
//--------------------------------------------------------------------------------------------------------

//--------------------------------------ПОТОКИ------------------------------------------------------------
void MyDB::DB_createTableStreams()
{
    if(QSqlDatabase::database().tables().contains("streams")) {
        qDebug() << "table streams already exists";
        return;
    }

    QString strStations;
    for(int i = 0; i < 180; i++)
        strStations += QString("S_%1, ").arg(i);
    QSqlQuery query(QSqlDatabase::database());
    if(query.exec(QString("CREATE TABLE IF NOT EXISTS streams(
                  "VP integer, "
                  "KP integer, "
                  "NP integer, "
                  "%1,         "
                  "PRIMARY KEY (VP, KP, NP)"
                  ")")
                  .arg(strStations)
                  )) {
        qDebug() << "table streams successfully created";
    }
    else {
        qDebug() << "unable to create table streams. " << query.lastError().text();
    }
}

Stream* MyDB::DB_getStream(int VP, int KP, int NP)
{
    Stream *s;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM streams WHERE VP = %1 AND KP = %2 AND NP = %3")
            .arg(VP)
            .arg(KP)
            .arg(NP);
    if(query.exec(strQuery)) {
        s = new Stream();
        //соответствующая заявка
        s->m_sourceRequest = request(VP, KP, NP);
        assert(s->m_sourceRequest);
        //станции маршрута
        QVector<Station*> passedStations;
        for(int i = 0; i < 180; i++) {
            stNum = query.value(QString("S_%1").arg(i)).toInt();
            if(stNum == 0)
                break;
            Station *st = stationByNumber(stNum);
            assert(st);
            passedStations.append(st);
        }
        s->m_passedStations = passedStations;
        //участки маршрута
        s->m_passedSections = Stream::fillSections(s->m_passedStations);
        //эшелоны
        s->m_echelones = DB_getEchelones(VP, KP, NP);
        //времена отправления и прибытия
        s->m_departureTime = s->m_echelones.first()->timesArrivalToStations.first();
        s->m_arrivalTime = s->m_echelones.last()->timesArrivalToStations.last();
        //занятая пропускная возможность
        QVector<QMap<int, int> > busyPassingPossibility;
        foreach (Section *sec, s->m_passedSections) {
            QMap<int, int> busys = DB_getSectionsLoad(VP, KP, NP, sec->stationNumber1, sec->stationNumber2);
            busyPassingPossibility.append(busys);
        }
        s->m_busyPassingPossibilities = busyPassingPossibility;
        //занятая погрузочная возможность
        s->m_busyLoadingPossibilities = DB_getStationsLoad(s->m_sourceRequest->SP);
    }
    return s;
}

QVector<Stream*> MyDB::DB_getStreams()
{
    QVector<Stream*> _streams;
    QSqlQuery query(QSqlDatabase::database());
    if(query.exec("SELECT * FROM streams")) {
        while(query.next()) {
            int VP = query.value("VP").toInt(), KP = query.value("KP").toInt(), NP = query.value("NP").toInt();
            Stream* s = DB_getStream(VP, KP, NP);
            assert(s);
            _streams.append(s);
        }
    }
    return _streams;
}

//--------------------------------------------------------------------------------------------------------

//---------------------------------ЗАНЯТОСТЬ СТАНЦИЙ--------------------------------------------------------------
void MyDB::DB_createTableStationsLoad()
{
    if(QSqlDatabase::database().tables().contains("stationsload")) {
        qDebug() << "table stationsload already exists";
        return;
    }
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery;
    QString strArray;

    for(int i = 0; i < 60; i++)
        strArray += QString("PV_%1 integer, ").arg(i);
    strArray.chop(2);
    strQuery = QString("CREATE TABLE IF NOT EXISTS stationsload("
            "SN integer, "
            "KG integer, "
            "VP integer, "
            "KP integer, "
            "NP integer, "
            "%1, "
            "PRIMARY KEY (VP, KP, NP)"
            ")").arg(strArray);

    if(query.exec(strQuery)) {
        qDebug() << "table stationsload successfully created";
    }
    else {
        qDebug() << "unable to create table stationsload. " << query.lastError().text();
        qDebug() << QString("Query: \"%1\"").arg(strQuery);
    }
}

void MyDB::DB_cropTableStationsLoad()
{
    QSqlQuery query(QSqlDatabase::database());
    query.exec("DELETE FROM stationsload");
}

void MyDB::DB_insertStationsLoad(int stationNumber, int KG, int VP, int KP, int NP,
                         QMap<int, int> loadDays)
{
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery;
    strQuery = QString("INSERT into stationsload VALUES (%1, %2, %3, %4, %5")
            .arg(stationNumber)
            .arg(KG)
            .arg(VP)
            .arg(KP)
            .arg(NP);
    for(int i = 0; i < 60; i++) {
        strQuery += ", ";
        int k = loadDays.value(i, 0);
        strQuery += QString::number(k);
    }
    strQuery += ")";
    if(!query.exec(strQuery)) {
        qDebug() << query.lastError().text();
    }
}

void MyDB::DB_removeStationsLoad(int VP, int KP, int NP)
{
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery;
    strQuery = QString("DELETE FROM stationsload WHERE VP = %1 AND KP = %2 AND NP = %3")
            .arg(VP)
            .arg(KP)
            .arg(NP);
    if(!query.exec(strQuery)) {
        qDebug() << query.lastError().text();
    }
}

QMap<int, int> DB_getStationsLoad(int VP, int KP, int NP, int KG)
{
    QString goodTypeDB = ProgramSettings::instance()->goodsTypesDB.value(KG);
    QList<int> KGs = ProgramSettings::instance()->goodsTypesDB.keys(goodTypeDB);

    QMap<int, int> busys;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM stationsload WHERE VP = %1 AND KP = %2 AND NP = %3 AND KG = %4")
            .arg(VP)
            .arg(KP)
            .arg(NP)
            .arg(KG);
    query.exec(strQuery);
    int i = 0;
    while(query.next()) {
        //если код груза удовлетворяет критериям, добавляем
        int _KG = query.value("KG").toInt();
        if(KGs.contains(_KG)) {
            int k = query.value(QString("PV_%1]").arg(i)).toInt();
            if(k != 0)
                busys.insert(i, k);
        }
        i++;
    }
    return busys;
}

QMap<int, int> MyDB::DB_getStationsLoad(int SN, int KG)
{
    QMap<int, int> busys;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM stationsload WHERE SN = %1 AND KG = %2")
            .arg(SN)
            .arg(KG);
    query.exec(strQuery);
    int i = 0;
    while(query.next()) {
        for(int i = 0; i < 60; i++) {
            int k = query.value(QString("PV_%1]").arg(i)).toInt();
            if(k != 0)
                busys.insert(i, k);
        }
    }
    return busys;
}

QMap<int, int> MyDB::DB_getStationsLoad(int SN, QString typeKG)
{
    QMap<int, int> busys;
    QList<int> KGs = ProgramSettings::goodsTypesDB.keys(typeKG);
    foreach (int kg, KGs) {
        QMap<int, int> innerBusy = DB_getStationsLoad(SN, kg);
        foreach (int day, innerBusy.keys()) {
            int old = busys.value(day);
            busys.insert(day, old + innerBusy.value(day, 0));
        }
    }
    return busys;
}

//----------------------------------------------------------------------------------------------------------------

//---------------------------------ЗАНЯТОСТЬ ПВР------------------------------------------------------------------
void MyDB::DB_createTablePVRLoad()
{
    if(QSqlDatabase::database().tables().contains("pvrload")) {
        qDebug() << "table pvrload already exists";
        return;
    }
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery;
    QString strArray;

    for(int i = 0; i < 60; i++)
        strArray += QString("PV_%1 integer, ").arg(i);
    strArray.chop(2);
    strQuery = QString("CREATE TABLE IF NOT EXISTS pvrload("
            "VP integer, "
            "KP integer, "
            "NP integer, "
            "PN integer, "
            "%1, "
            "PRIMARY KEY (VP, KP, NP)"
            ")").arg(strArray);

    if(query.exec(strQuery)) {
        qDebug() << "table pvrload successfully created";
    }
    else {
        qDebug() << "unable to create table pvrload. " << query.lastError().text();
        qDebug() << QString("Query: \"%1\"").arg(strQuery);
    }
}

void MyDB::DB_cropTablePVRLoad()
{
    QSqlQuery query(QSqlDatabase::database());
    query.exec("DELETE FROM pvrload");
}

void MyDB::DB_insertPVRLoad(int VP, int KP, int NP, int PN, QMap<int, int> loadDays)
{
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery;
    strQuery = QString("INSERT into pvrload VALUES (%1, %2, %3, %4, %5")
            .arg(PN)
            .arg(VP)
            .arg(KP)
            .arg(NP);
    for(int i = 0; i < 60; i++) {
        strQuery += ", ";
        int k = loadDays.value(i, 0);
        strQuery += QString::number(k);
    }
    strQuery += ")";
    query.exec(strQuery);
}

void MyDB::DB_removePVRLoad(int VP, int KP, int NP)
{
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery;
    strQuery = QString("DELETE FROM pvrload WHERE VP = %1 AND KP = %2 AND NP = %3")
            .arg(VP)
            .arg(KP)
            .arg(NP);
    if(!query.exec(strQuery)) {
        qDebug() << query.lastError().text();
    }
}

QMap<int, int> MyDB::DB_getPVRLoad(int VP, int KP, int NP, int PN)
{
    QMap<int, int> busys;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM pvrload WHERE VP = %1 AND KP = %2 AND NP = %3 AND PN = %4")
            .arg(VP)
            .arg(KP)
            .arg(NP)
            .arg(PN);
    query.exec(strQuery);
    for(int i = 0; i < 60; i++) {
        int k = query.value(QString("PV_%1").arg(i)).toInt();
        if(k != 0)
            busys.insert(i, k);
    }
    return busys;
}

QMap<int, int> MyDB::DB_getPVRLoad(int PN)
{
    QMap<int, int> busys;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM pvrload WHERE NP = %1")
            .arg(NP);
    query.exec(strQuery);
    for(int i = 0; i < 60; i++) {
        int k = query.value(QString("PV_%1").arg(i)).toInt();
        if(k != 0)
            busys.insert(i, k);
    }
    return busys;
}

//----------------------------------------------------------------------------------------------------------------

//---------------------------------ЗАНЯТОСТЬ УЧАСТКОВ-------------------------------------------------------------
void MyDB::DB_createTableSectionsLoad()
{
    if(QSqlDatabase::database().tables().contains("sectionsload")) {
        qDebug() << "table sectionsload already exists";
        return;
    }
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery;
    QString strBusy;
    for(int i = 0; i < 60; i++) {
        strBusy += QString("PV_%1, ").arg(i);
    }
    strBusy.chop(2);

    strQuery = QString("CREATE TABLE IF NOT EXISTS sectionsload("
            "VP integer, "
            "KP integer, "
            "NP integer, "
            "S1 integer, "
            "S2 integer, "
            "%1,         "
            "PRIMARY KEY (VP, KP, NP, S1, S2)"
            ")").arg(strBusy);

    if(query.exec(strQuery)) {
        qDebug() << "table sectionsload successfully created";
    }
    else {
        qDebug() << "unable to create table sectionsload. " << query.lastError().text();
    }
}

void MyDB::DB_cropTableSectionsLoad()
{
    QSqlQuery query(QSqlDatabase::database());
    query.exec("DELETE * FROM sectionsload");
}

QMap<int, int> MyDB::DB_getSectionsLoad(int VP, int KP, int NP, int S1, int S2)
{
    QMap<int, int> loads;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM sectionsload WHERE VP = %1 AND KP = %2 AND NP = %3 AND S1 = %4 AND S2 = %5")
            .arg(VP)
            .arg(KP)
            .arg(NP)
            .arg(S1)
            .arg(S2);
    query.exec(strQuery);
    for(int i = 0; i < 60; i ++) {
        int load = query.value(QString("PV_%1").arg(i)).toInt();
        if(load != 0) {
            loads.insert(i, load);
        }
    }
    return loads;
}

QMap<int, int> MyDB::DB_getSectionsLoad(int S1, int S2)
{
    QMap<int, int> loads;
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM sectionsload WHERE S1 = %1 AND S2 = %2")
            .arg(S1)
            .arg(S2);
    query.exec(strQuery);
    for(int i = 0; i < 60; i ++) {
        int load = query.value(QString("PV_%1").arg(i)).toInt();
        if(load != 0) {
            loads.insert(i, load);
        }
    }
    return loads;
}


void MyDB::DB_insertSectionLoad(int VP, int KP, int NP, int S1, int S2, QMap<int, int> loads)
{
    QMap<int, int> loads;
    QSqlQuery query(QSqlDatabase::database());
    QString strLoads;
    for(int i = 0; i < 60; i++) {
        int load = loads.value(i, 0);
        strLoads += QString("%1, ").arg(load);
    }
    strLoads.chop(2);

    QString strQuery = QString("INSERT OR REPLACE into sectionsload VALUES(%1, %2, %3, %4, %5, %6)")
            .arg(VP)
            .arg(KP)
            .arg(NP)
            .arg(S1)
            .arg(S2)
            .arg(strLoads);
    query.exec(strQuery);
}

//----------------------------------------------------------------------------------------------------------------


//------------------------------------ЭШЕЛОНЫ---------------------------------------------------------------------
Echelon* MyDB::echelon(int VP, int KP, int NP, int NE)
{
    foreach (Echelon* e, m_echelones) {

    }
}

void MyDB::DB_createTableEchelones()
{
    if(QSqlDatabase::database().tables().contains("echelones")) {
        qDebug() << "table echelones already exists";
        return;
    }
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery;
    QString strTimes;
    for(int i = 0; i < 180; i++)
        strTimes += QString("T_%1 INTEGER, ").arg(i);
    strTimes.chop(2);
    strQuery = QString("CREATE TABLE IF NOT EXISTS echelones("
            "VP integer, "//вид перевозок
            "KP integer, "//код получателя
            "NP integer, "//номер потока
            "NE integer, "//номер эшелона
            "NA text,    "//наименование и количество перевозимого
            "C1 integer, "//пасс
            "L1 integer, "//людс
            "K1 integer, "//крыт
            "X1 integer, "//кухн
            "P1 integer, "//плат
            "V1 integer, "//полу
            "Y1 integer, "//спец
            "D1 integer, "//ледн
            "R1 integer, "//цист
            "CH integer, "//всего
            "%1,         "
            "PRIMARY KEY (VP, KP, NP, NE)"
            ")").arg(strTimes);

    if(query.exec(strQuery)) {
        qDebug() << "table echelones successfully created";
    }
    else {
        qDebug() << "unable to create table echelones. " << query.lastError().text();
        qDebug() << QString("Query: \"%1\"").arg(strQuery);
    }
}

void MyDB::DB_cropTableEchelones()
{
    QSqlQuery query(QSqlDatabase::database());
    query.exec("DELETE FROM echelones");
}

Echelon MyDB::DB_getEchelon(int VP, int KP, int NP, int NE)
{
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM echelones WHERE VP = %1, KP = %2, NP = %3, NE = %4")
            .arg(VP)
            .arg(KP)
            .arg(NP)
            .arg(NE);

    if(!query.exec(strQuery))
        qDebug() << query.lastError().text();
    if(!query.first())
        qDebug() << query.lastError().text();

    Echelon e;
    e.NA = query.value("NA").toString();
    e.number = query.value("NE").toInt();
    //подвижной состав----------------------------------
    e.ps.pass = query.value("C1").toInt();
    e.ps.luds = query.value("L1").toInt();
    e.ps.krit = query.value("K1").toInt();
    e.ps.kuhn = query.value("X1").toInt();
    e.ps.plat = query.value("P1").toInt();
    e.ps.polu = query.value("V1").toInt();
    e.ps.spec = query.value("Y1").toInt();
    e.ps.ledn = query.value("D1").toInt();
    e.ps.cist = query.value("R1").toInt();
    e.ps.total = query.value("CH").toInt();
    //--------------------------------------------------
    //времена проследования-----------------------------
    QVector<MyTime> times;
    int cur_hours, prev_hours = 0;
    for(int i = 0; i < 180; i++) {
        cur_hours = query.value(QString("T_%1").arg(i)).toInt();
        if(cur_hours < prev_hours)
            break;
        times.append(MyTime::timeFromHours(cur_hours));
        prev_hours = cur_hours;
    }
    e.timesArrivalToStations = times;
    //---------------------------------------------------
    return e;
}

QVector<Echelon> MyDB::DB_getEchelones(int VP, int KP, int NP)
{
    QSqlQuery query(QSqlDatabase::database());
    QString strQuery = QString("SELECT * FROM echelones WHERE VP = %1, KP = %2, NP = %3")
            .arg(VP)
            .arg(KP)
            .arg(NP);

    query.exec(strQuery);
    QVector<Echelon> echs;
    while(query.next()) {
        echs.append(DB_getEchelon(VP, KP, NP, query.value("NE").toInt()));
    }
    return echs;
}
//----------------------------------------------------------------------------------------------------------------
