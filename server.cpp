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
    connect(this, SIGNAL(messageReady()), this, SLOT(dispatchMessage()));
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
    qDebug() << "client disconnected";
}

void Server::sendPacket(Packet &pack)
{
    QByteArray buf = pack.toByteArray();
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << quint16(0);
    out << quint8(pack.type());
    out.device()->seek(0);
    out << quint16(buf.size() + ba.size() - sizeof(quint16));
    ba += buf;
    m_tcpSocket->write(ba);
    m_tcpSocket->flush();
}

void Server::readMessage()
{
    quint16 blockSize;
    QDataStream in(m_tcpSocket);
    in.setVersion(QDataStream::Qt_4_0);

    if (m_tcpSocket->bytesAvailable() < (int)sizeof(quint16)) {
        m_currentMessage = QString("Can't read size of message: %1").arg(m_tcpSocket->errorString());
        return;
    }
    in >> blockSize;

    if (m_tcpSocket->bytesAvailable() < blockSize) {
        m_currentMessage = QString("Can't read message: %1").arg(m_tcpSocket->errorString());
        return;
    }
    in >> m_currentMessage;

    displayMessage();
    emit messageReady();
}

void Server::displayMessage()
{
    qDebug() << "Readed message: " << m_currentMessage;
}

void Server::openSession()
{
    m_tcpServer = new QTcpServer(this);
    QHostAddress localHost(QHostAddress::LocalHost);
    if (!m_tcpServer->listen(localHost, PORT)) {
        m_currentMessage = QString("Unable to start the server: %1.")
                              .arg(m_tcpServer->errorString());
        QTimer::singleShot(0, this, SLOT(displayMessage()));
        return;
    }
    m_currentMessage = QString("Run client now at adress: %1, port: %2")
            .arg(m_tcpServer->serverAddress().toString())
            .arg(m_tcpServer->serverPort());
    QTimer::singleShot(0, this, SLOT(displayMessage()));
}

void Server::dispatchMessage()
{
    QString msg = m_currentMessage;
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
        int VP_Start = fields[0].toInt();
        int VP_End = fields[1].toInt();
        int KP_Start = fields[2].toInt();
        int KP_End = fields[3].toInt();
        int NP_Start = fields[4].toInt();
        int NP_End = fields[5].toInt();
//        QString grif = fields[6];

        FilterStream *filterStream = new FilterStream();
        filterStream->setTypeTransport(22, 24);
        filterStream->setCodeRecipient(15, 22);
        filterStream->setNumberStream(100, 140);
        QByteArray ba;
        qDebug() << "total:" << MyDB::instance()->streams().size();
        ba = DocumentsFormer::createForm2(filterStream->filter(MyDB::instance()->streams().data()));
        delete filterStream;

        Packet pack(ba, TYPE_XML_F2);
        sendPacket(pack);
        break;
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

    QVector<Stream*> streams;
    int iter = 0;
    foreach (Request *req, requests) {
        Stream *stream = m_graph->planStream(req, SUZ, SUZ);
        if(stream)
            streams.append(stream);
        else
            failedStreams.append(req);
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
}

