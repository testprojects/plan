#include <QtNetwork>
#include <QDebug>
#include <iostream>

#include "station.h"
#include "mydb.h"
#include "server.h"
#include "../myClient/packet.h"
#define PORT 1535

Server::Server()
: m_tcpServer(0), m_currentMessage("empty")
{
    openSession();
    connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(listenClient()));
    connect(this, SIGNAL(messageReady()), this, SLOT(dispatchMessage()));
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
    m_tcpSocket->write(ba);
    m_tcpSocket->write(buf);
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
    if(msg.startsWith(QString("%1").arg(PLAN_SUZ))) {
        Station st = *MyDB::instance()->stationByNumber(101472318);
        Packet pack(st);
        sendPacket(pack);
    }
    else if(msg.startsWith(QString("%1").arg(PLAN_BUZ))) {
        Packet pack(QString("planning stream (buz): %1").arg(msg));
        sendPacket(pack);
    }
}

