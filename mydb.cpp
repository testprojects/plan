#include "mydb.h"
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

bool MyDB::createConnection(QString databaseName, QString hostName, QString userName, QString password)
{
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName(hostName);
    db.setDatabaseName(databaseName);
    db.setUserName(userName);
    db.setPassword(password);
    bool ok = db.open();
    return ok;
}

void MyDB::addRequestsFromFile(QString requestsFilePath)
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
        QString newStr = parseRequest(in.readLine());
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

QString MyDB::parseRequest(QString old)
{
    QStringList fields;
    QString newStr;
    //разделяем поля, помещаем последовательно в fields
    while(!old.isEmpty()) {
        int i = old.indexOf(';');
        QString buf = old.left(i);
        if(buf.isEmpty()) buf = "0";
        fields << buf;
        old.remove(0, i + 1);
    }
    //заполняем newStr из полученных полей
    newStr += fields[0].mid(1,2) + ", ";//вид перевозок
    newStr += fields[0].mid(3,3) + ", ";//код получателя
    newStr += QString("15") + ", ";//число начала перевозок
    newStr += QString("12") + ", ";//месяц начала перевозок
    newStr += QString("2014") + ", ";//год начала перевозок
    newStr += fields[1] + ", ";//номер потока
    newStr += QString("\'") + fields[26] + QString("\', ");//наименование и кол-во перевозимого
    newStr += QString("\'") + fields[27] + QString("\', ");//номер штата
    newStr += QString("\'") + fields[28] + QString("\', ");//отправитель
    newStr += QString("\'") + fields[29] + QString("\', ");//получатель
    newStr += QString("\'") + fields[31] + QString("\', ");//месяц готовности
    newStr += QString::number((fields[98].toInt() / 24 + 1)) + ", ";//день готовности
    newStr += QString::number((fields[98].toInt() % 24)) + ", ";//час готовности
    newStr += fields[4].left(1) + ", ";//категория срочности
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
    newStr += fields[33] + ", ";//номер эшелона, с которым следует россыпь
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
    newStr += fields[78] + "";//особенности перевозки

//    int i = 0;
//    foreach (QString tmp, fields) {
//        qDebug() << i++ << ")" << tmp;
//    }
//    qDebug() << newStr;
    return newStr;
}

QString MyDB::parsePVR(QString old)
{
    QString newPVR;
    QStringList fields;
    for(int i = 0; i < 6; i++) {
        QString tmp;
        int ind = old.indexOf(';');
        tmp = old.left(ind);
        fields << tmp;
        old.remove(0, ind + 1);
    }
    fields << old;
    newPVR = fields[0] + ", \'" + fields[1] + "\', " + fields[4] + ", \'{";
    for(int i = 0; i < 60; i++) {
        newPVR += fields[4];
        newPVR += ", ";
    }
    newPVR.chop(2);
    newPVR += "}\'";
    return newPVR;
}

void MyDB::createTableRequests()
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
                                                      "ER int, "
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
                                                      "OP smallint"
                  ")")) {
        qDebug() << "table requests successfully created";
    }
    else {
        qDebug() << "unable to create table: requests. " << query.lastError().text();
    }
}

void MyDB::createTablePVR()
{
    if(db.tables().contains("pvr")) {
        qDebug() << "table pvr already exists";
        return;
    }
    QSqlQuery query(db);
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
        QString newStr = parsePVR(in.readLine());
        if(PVRList.contains(newStr)) continue;
        PVRList.append(newStr);
    }
    QSqlQuery query(db);
    foreach (QString tmp, PVRList) {
        if(query.exec("INSERT INTO pvr VALUES(" + tmp + ")")) {
            qDebug() << "pvr successfully added";
        }
        else {
            qDebug() << "Error, while trying to add pvr: " << query.lastError().text();
        }
    }

}

void MyDB::dropTable(QString tableName)
{
    QSqlQuery query(db);
    if (query.exec("DROP TABLE IF EXISTS " + tableName)) {
        qDebug() << "table " + tableName + " successfully deleted";
    }
    else {
        qDebug() << "unable to delete table " + tableName +". " << query.lastError().text();
    }
}

void MyDB::readDatabase()
{
    QSqlQuery query(QSqlDatabase::database());

    //stations
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    if(!query.exec("SELECT * FROM stations;"))
        qDebug() << query.lastError().text();
    while(query.next()) {
        station st;
        double LAT, LONG;
        QString str;

        str = query.value("PN").toString();
        while(str.endsWith(' ')) {
            if(str.isEmpty()) {
                qDebug() << "station name is empty";
                break;
            }
            str.chop(1);
        }

        QString buf = query.value("SH").toString();
        LAT = buf.left(3).toDouble() + buf.mid(3, 2).toDouble() / 60 + buf.right(2).toDouble() / 3600;
        buf = query.value("DL").toString();
        LONG = buf.left(3).toDouble() + buf.mid(3, 2).toDouble() / 60 + buf.right(2).toDouble() / 3600;


        st.number =  query.value("SK").toInt();
        st.name = str;
        st.latitude = LAT;
        st.longitude = LONG;
        st.type = query.value("ST").toInt();
        st.startNumber = query.value("SB").toInt();
        st.endNumber = query.value("SE").toInt();
        st.distanceTillStart = query.value("LB").toInt();
        st.distanceTillEnd = query.value("LK").toInt();
        st.pvrNumber = query.value("NR").toInt();
        st.loadingPossibilityForOperativeTraffic = query.value("DR").toInt();

        //здесь можно выполнить проверку на правильность структуры
        m_stations.append(st);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    //sections
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    QString strQuery = "SELECT KU, KK, LU, SP, ";
    for(int i = 0; i < 60; i++)
        strQuery += "PV[" + QString::number(i) + "], ";

    strQuery += "VU, LM FROM sections";
    query.exec(strQuery);
    while(query.next()) {
        section sec;

        sec.stationNumber1 = query.value("KU").toInt();
        sec.stationNumber2 = query.value("KK").toInt();
        sec.distance = query.value("LU").toInt();
        sec.ps = query.value("SP").toInt();
        sec.speed = query.value("VU").toInt();
        for(int i = 0; i < 60; i++) {
            sec.passingPossibilities[i] = query.value(i+4).toInt();
        }
        sec.limited = query.value("LM").toBool();

        //здесь можно выполнить проверку на правильность структуры
        m_sections.append(sec);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    //pvrs
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    strQuery = "SELECT NR, IR, PR, ";
    for(int i = 1; i <= 60; i++)
        strQuery += "SP[" + QString::number(i) + "], ";
    strQuery.chop(2);
    strQuery += " FROM pvr";
    if(!query.exec(strQuery))
        qDebug() << query.lastError().text();
    while(query.next()) {
        pvr _pvr;

        int num = query.value("NR").toInt();
        QString strName = query.value("IR").toString();
        while(strName.endsWith(' ')) {
            if(strName.isEmpty()) {
                qDebug() << "pvr name is empty";
                break;
            }
            strName.chop(1);
        }
        int PR = query.value("PR").toInt();
        int SP[60];
        for(int i = 0; i < 60; i++)
            SP[i] = query.value(i + 3).toInt();

        _pvr.name = strName;
        _pvr.number = num;
        _pvr.ps = PR;
        for(int i = 0; i < 60; i++)
            _pvr.pv[i] = SP[i];

        //здесь можно выполнить проверку на правильность структуры
        m_pvrs.append(_pvr);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////
}

station MyDB::stationByNumber(int n)
{
    if(n == 0) {
        qDebug() << "Станции с номером 0 не существует";
        exit(1);
    }
    QSqlQuery query(db);
    QString strQuery = "SELECT PN, SH, DL, ST, SB, SE, LB, LK, NR FROM stations WHERE sk = \'" + QString::number(n) + "\'";
    query.exec(strQuery);
    if(!query.first()) {
        qDebug() << "Не удалось найти станцию с номером " << n << " в БД";
        exit(1);
    }

    station st;
    double LAT, LONG;

    QString buf = query.value("SH").toString();
    LAT = buf.left(3).toDouble() + buf.mid(3, 2).toDouble() / 60 + buf.right(2).toDouble() / 3600;
    buf = query.value("DL").toString();
    LONG = buf.left(3).toDouble() + buf.mid(3, 2).toDouble() / 60 + buf.right(2).toDouble() / 3600;

    st.number = n;
    st.name = query.value("PN").toString();
    st.latitude = LAT;
    st.longitude = LONG;
    st.type = query.value("ST").toInt();
    st.startNumber = query.value("SB").toInt();
    st.endNumber = query.value("SE").toInt();
    st.distanceTillStart = query.value("LB").toInt();
    st.distanceTillEnd = query.value("LK").toInt();
    st.pvrNumber = query.value("NR").toInt();

    return st;
}

section MyDB::sectionByStations(station s1, station s2)
{
    //если обе станции неопорные
    if((s1.type == 4)&&(s2.type == 4)) {
        s2 = stationByNumber(s1.endNumber);
        s1 = stationByNumber(s1.startNumber);
    }

    //если одна из станций - неопорная
    if(s1.type == 4) {
        if(s2.number == s1.startNumber) {
            s1 = stationByNumber(s1.endNumber);
        }
        if(s2.number == s1.endNumber) {
            s1 = stationByNumber(s1.startNumber);
        }
    }
    if(s2.type == 4) {
        if(s1.number == s2.startNumber) {
            s2 = stationByNumber(s2.endNumber);
        }
        if(s1.number == s2.endNumber) {
            s2 = stationByNumber(s2.startNumber);
        }
    }

    QSqlQuery query(db);
    QString strQuery = "SELECT * FROM sections WHERE ku = \'" + QString::number(s1.number) + "\' AND kk = \'" + QString::number(s2.number) + "\'";
    query.exec(strQuery);
    if(!query.first()) {
        strQuery = "SELECT * FROM sections WHERE ku = \'" + QString::number(s2.number) + "\' AND kk = \'" + QString::number(s1.number) + "\'";
        query.exec(strQuery);

        if(!query.first()) {
            qDebug() << "Не удалось найти участок с номерами " << s1.number << " - " << s2.number << " в БД";
            exit(1);
        }
    }

    section s;
    s.distance = query.value("LU").toInt();
    s.limited = query.value("LM").toInt();
    s.ps = query.value("SP").toInt();
    s.stationNumber1 = query.value("KU").toInt();
    s.stationNumber2 = query.value("KK").toInt();
    s.speed = query.value("VU").toInt();
    QString strPV = query.value("PV").toString();
    for(int i = 0; i < 60; i++) {
        s.passingPossibilities[i] = strPV.mid(i*4 + 8, 3).toInt();
    }
    return s;
}

pvr MyDB::pvrByStationNumber(int n)
{
    QSqlQuery query(db);
    station st = stationByNumber(n);
    if(st.pvrNumber == 0)
        return pvr();
    QString strQuery = "SELECT * FROM pvr WHERE nr = " + QString::number(st.pvrNumber) + ";";

    if(!query.exec(strQuery))
        qDebug() << query.lastError().text();
    if(!query.first())
        qDebug() << query.lastError().text();
    pvr p;
    p.number = query.value("nr").toInt();
    p.name = query.value("ir").toString();
    p.ps = query.value("pr").toInt();
    QString ps = query.value("sp").toString();
    for(int i = 0; i < 60; i++)
        p.pv[i] = ps.mid(i*3 + 1, 2).toInt();
    return p;
}

MyDB* MyDB::instance()
{
    if(!_self) {
        _self = new MyDB();
    }
    return _self;
}

MyDB* MyDB::_self = 0;

Request MyDB::requestByStationsName(QString stationLoadName, QString stationUnloadName, QStringList requiredStationsNames)
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

Request MyDB::requestByStationsName(QString stationLoadName, QString stationUnloadName)
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

Request MyDB::request(int VP, int KP, int NP)
{
    Request tmp;
    QSqlQuery query(MyDB::instance()->db);
    QString str = "SELECT * FROM requests WHERE vp = " + QString::number(VP) + " AND kp = " + QString::number(KP) + " AND np = " + QString::number(NP);
    if(!query.exec(str)) {
        qDebug() << "Нет заявки с такими параметрами";
        exit(2);
    }
    query.first();

    //ключ
    tmp.VP = query.value("VP").toInt();
    tmp.KP = query.value("KP").toInt();
    tmp.NP = query.value("NP").toInt();

    //начало перевозок
    tmp.CN = query.value("CN").toInt();
    tmp.GN = query.value("GN").toInt();
    tmp.MN = query.value("MN").toInt();
    //готовность
    tmp.MG = query.value("mo").toString();
    tmp.DG = query.value("deno").toInt();
    tmp.CG = query.value("co").toInt();

    //наименование перевозимого
    tmp.NA = query.value("NA").toString();

    //станции погрузки, разгрузки
    tmp.SP = query.value("SP").toInt();
    tmp.SV = query.value("SV").toInt();
    QString strOM = query.value("OM").toString();
    while(!strOM.isEmpty()) {
        if(strOM.left(9).toInt() != 0)
            tmp.OM.append(strOM.left(9).toInt());
        if(strOM.size() == 9) strOM.remove(0, 9);
        else
            strOM.remove(0, 10);
    }

    //количество поездов
    tmp.PK = query.value("PK").toInt();
    //темп заданный
    tmp.TZ = query.value("TZ").toInt();

    tmp.KS = query.value("KS").toInt();
    tmp.PR = query.value("PR").toInt();
    tmp.KG = query.value("KG").toInt();
    tmp.PG = query.value("PG").toString();
    tmp.OP = query.value("OP").toInt();

    PS ps;
    ps.pass = query.value("C1").toInt();
    ps.luds = query.value("L1").toInt();
    ps.krit = query.value("K1").toInt();
    ps.kuhn = query.value("X1").toInt();
    ps.plat = query.value("P1").toInt();
    ps.polu = query.value("V1").toInt();
    ps.spec = query.value("Y1").toInt();
    ps.ledn = query.value("D1").toInt();
    ps.cist = query.value("R1").toInt();
    ps.total = query.value("CH").toInt();
    tmp.ps = ps;

//    QString SH;//номер штата
//    QString OT;//отправитель
//    QString PY;//получатель

//    QVector<QString> NE;//номера эшелонов
//    int ER;//номер эшелона, с которым следует россыпь
//    PS ps;//подвижной состав
    return tmp;
}

void MyDB::resetPassingPossibility()
{
    QSqlQuery query(MyDB::instance()->db);
    QString str = "UPDATE sections SET PV[0:59] = ARRAY[SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP, SP]";
    if(!query.exec(str)) {
        qDebug() << "Пропускная способность не сброшена: " << query.lastError().text();
    }
}

void MyDB::resetLoadingPossibility()
{
    QSqlQuery query(MyDB::instance()->db);
    QString str = "UPDATE pvr SET SP[1:60] = ARRAY[PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR, PR]";
    if(!query.exec(str)) {
        qDebug() << "Погрузочная способность не сброшена: " << query.lastError().text();
    }
}
