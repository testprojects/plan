#include <QtNetwork>
#include <QDebug>
#include <iostream>

#include "station.h"
#include "mydb.h"
#include "server.h"
#include "../myClient/packet.h"
#include "graph.h"
#include "filterstream.h"
#include "../myClient/types.h"
#include "documentsformer.h"
#include "pauser.h"
#include <QTimer>

#define PORT 1535

Server::Server()
: m_tcpServer(0), m_currentMessage("empty")
{
    MyDB::instance()->checkTables();
    MyDB::instance()->BASE_deleteStreamsFromDB();
    MyDB::instance()->cacheIn();
    m_graph = new Graph(MyDB::instance()->stations(), MyDB::instance()->sections(), this);
    openSession();

    connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(listenClient()));
    connect(this, SIGNAL(messageReady(QString)), this, SLOT(dispatchMessage(QString)));
    connect(this, SIGNAL(signalPlanStreams(int,int,int,int,bool)), SLOT(slotPlanStreams(int,int,int,int,bool)));
}

void Server::listenClient()
{
    m_tcpSocket = m_tcpServer->nextPendingConnection();

    qDebug() << "client connected";
    connect(m_tcpSocket,  SIGNAL(readyRead()), this, SLOT(readMessage()));
    connect(m_tcpSocket,  SIGNAL(disconnected()), this, SLOT(printDisconnected()));
}

void Server::printDisconnected()
{
    delete m_pauser;
    qDebug() << "client disconnected";
}

void Server::sendPacket(Packet &pack)
{
    QByteArray buf = pack.toByteArray();
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << quint32(0);
    out << quint8(pack.type());
    out.device()->seek(0);
    out << quint32(buf.size() + sizeof(quint8));
    ba += buf;
    m_tcpSocket->write(ba);
    m_tcpSocket->flush();
}

void Server::readMessage()
{
    qDebug() << "Server::readMessage()";
    quint32 blockSize;
    QDataStream in(m_tcpSocket);
    in.setVersion(QDataStream::Qt_4_0);

    if (m_tcpSocket->bytesAvailable() < (int)sizeof(quint32)) {
        m_currentMessage = QString("Can't read size of message: %1").arg(m_tcpSocket->errorString());
        return;
    }
    in >> blockSize;
    qDebug() << "Block size    : " << blockSize;

    if(m_tcpSocket->bytesAvailable() < blockSize) {
        QCoreApplication::processEvents();
    }

    in >> m_currentMessage;
    qDebug() << "Readed message: " << m_currentMessage;

    displayMessage(m_currentMessage);
    emit messageReady(m_currentMessage);
}

void Server::displayMessage(QString msg)
{
    qDebug() << "Readed message: " << msg;
}

void Server::openSession()
{
    m_tcpServer = new QTcpServer(this);
    QHostAddress localHost(QHostAddress::LocalHost);
    if (!m_tcpServer->listen(localHost, PORT)) {
        m_currentMessage = QString("Unable to start the server: %1.")
                              .arg(m_tcpServer->errorString());
        return;
    }
    m_currentMessage = QString("Run client now at adress: %1, port: %2")
            .arg(m_tcpServer->serverAddress().toString())
            .arg(m_tcpServer->serverPort());
    qDebug() << m_currentMessage;
}

void Server::dispatchMessage(QString msg)
{
    Commands command = (Commands)msg.left(msg.indexOf(',', 0)).toInt();
    msg.remove(0, msg.indexOf(',', 0) + 1);

    switch (command) {
    case PLAN_SUZ:
    {
        Packet pack("PLAN_STARTED");
        sendPacket(pack);

        int VP, KP, NP_Start, NP_End;
        QStringList list = msg.split(',');
        VP = list[0].toInt();
        KP = list[1].toInt();
        NP_Start = list[2].toInt();
        NP_End = list[3].toInt();
        emit signalPlanStreams(VP, KP, NP_Start, NP_End, true);
        break;
    }
    case PLAN_BUZ:
    {
        Packet pack("PLAN_STARTED");
        sendPacket(pack);

        int VP, KP, NP_Start, NP_End;
        QStringList list = msg.split(',');
        VP = list[0].toInt();
        KP = list[1].toInt();
        NP_Start = list[2].toInt();
        NP_End = list[3].toInt();
        emit signalPlanStreams(VP, KP, NP_Start, NP_End, false);
        break;
    }
    case ACCEPT_OFFSET:
    {
        bool b_accepted = msg.split(',').at(0).toInt();
        emit signalOffsetAccepted(b_accepted);
        break;
    }
    case GET_F2:
    {
        QStringList fields;
        fields = msg.split(',');
        int VP = fields[0].toInt();
        int KP_Start = fields[1].toInt();
        int KP_End = fields[2].toInt();
        int NP_Start = fields[3].toInt();
        int NP_End = fields[4].toInt();
        QString grif = fields[5];

        bool divideByKG = (bool) fields[6].toInt(); //разделять ли по коду груза [0,1]
        bool divideByOKR = (bool) fields[7].toInt(); //разделять ли по округам [0,1]
        QString actionOKR = fields[8];//как разделять по округам ["Прибытие в округ", "Убытие из округа", "Транзит через округ"]
        QStringList okr = fields[9].split(';');//военные округа ["ЗВО", "ВВО", "ЮВО", "СФ", "ЦВО"]

        //разделение по кодам груза должно представляться в след. виде:
        //"код груза - наименование груза"
        //код груза можно найти в Steam::m_sourceRequest->KG;
        //наименование груза в соответствии с кодом QString naimenGruza = ProgrammSettings::instance()->m_goodsNames.value(int KG);

        //разделение по округам
        //в оглавлении документа пишем одно из след. значений (Прибытие в округ / Убитие с округа / Транзит через округ)
        //подразделы именуем след. образом:
        //"Наименование военного округа (Западный военный округ и т.д.)"

        FilterStream *filterStream = new FilterStream();
        filterStream->setTypeTransport(VP, VP);
        filterStream->setCodeRecipient(KP_Start, KP_End);
        filterStream->setNumberStream(NP_Start, NP_End);

        QByteArray ba;
        int streamsCount = MyDB::instance()->streams().size();

        ba = DocumentsFormer::createXmlForm2(filterStream->filter(MyDB::instance()->streams().data(), streamsCount));
        delete filterStream;

        Packet pack(ba, TYPE_XML_F2);
        sendPacket(pack);
        break;
    }
    case LOAD_REQUEST_ZHENYA:
    {
        QString data = msg;
        MyDB::instance()->BASE_loadRequestFromQStringDISTRICT(data);
        Packet pack("REQUESTS_ADDED");
        sendPacket(pack);
    }
    case LOAD_REQUEST_DIKON:
    {
        QString data = msg;
        MyDB::instance()->BASE_loadRequestFromQStringWZAYV(data);
        Packet pack("REQUESTS_ADDED");
        sendPacket(pack);
    }
    default:
        break;
    }
}

void Server::slotPlanStreams(int VP, int KP, int NP_Start, int NP_End, bool SUZ)
{
    QVector<Request*> requests;
#ifndef TEST_MOVE_STREAM
    if(KP == 0)
        requests = MyDB::instance()->requests(VP);
    else
        requests = MyDB::instance()->requests(VP, KP, NP_Start, NP_End);
#else
    Request *req = new Request;
    req->SP = 101072009;    //ЛУГА 1(101072009)
    req->SV = 101050009;    //БОЛОГОЕ-МОСКОВСКОЕ(101050009)
    req->PK = 6;            //6 поездов
    req->TZ = 3;            //с темпом 5 поездов в сутки
    req->DG = 18;           //день готовности - 18
    req->CG = 0;            //час готовности - 0
    req->VP = 23;           //24 вид перевозок
    req->KP = 1;
    req->NP = 1;
    req->KG = 3;            //код груза. 24_GSM
    requests.append(req);

    Section *sec = MyDB::instance()->sectionByNumbers(101072009, 101058302);
    //поток должен сдвинуться
    sec->m_passingPossibilities.insert(16, 2);
    sec->m_passingPossibilities.insert(17, 2);
    sec->m_passingPossibilities.insert(18, 1);
    sec->m_passingPossibilities.insert(19, 1);
    sec->m_passingPossibilities.insert(20, 1);
    sec->m_passingPossibilities.insert(21, 1);
#endif

    QVector<Request*> failedStreams;
    int iter = 0;
    foreach (Request *req, requests) {
        QMap<int, int> loadAtDays;
        LoadType load_type;
        Stream *stream;
        stream = MyDB::instance()->stream(req->VP, req->KP, req->NP);
        if(!stream) {
            switch(req->canLoad(&loadAtDays)) {
            case 0: load_type = LOAD_NO; break;
            case 1: load_type = LOAD_STATION; break;
            case 2: load_type = LOAD_PVR; break;
            default: assert(0);
            }
            if((load_type == LOAD_NO) || (req->VP == 21) || (req->VP == 22) || (!SUZ))
                stream = m_graph->planStream(req, false, false);
            else {
                req->load(loadAtDays);
                stream = m_graph->planStream(req, true, true);
            }
            if(stream) {
                stream->m_loadType = load_type;
                if(load_type)
                    stream->m_busyLoadingPossibilities = loadAtDays;
                MyDB::instance()->addToCache(stream);
            }
            else {
                failedStreams.append(req);
            }
        }
        iter++;
        Packet pack(QString("STREAM_PLANNED(%1/%2)").arg(iter).arg(requests.count()));
        sendPacket(pack);
    }

    if(!failedStreams.isEmpty()) {
        Packet pack(QString("FAILED_STREAMS(%1)").arg(failedStreams.count()));
        sendPacket(pack);
    }
    Packet pack("PLAN_FINISHED");
    sendPacket(pack);

    MyDB::instance()->cacheOut();
}

void Server::slotOffsetAccepted(bool bAccepted)
{
    qDebug() << "accepted = " << bAccepted;
}


