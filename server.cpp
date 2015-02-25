#include <QtNetwork>
#include <QDebug>

#include <stdlib.h>
#include <iostream>

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

void Server::sendPacket(const Packet &pack)
{
    QByteArray ba = pack.toByteArray();
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
    if(msg.startsWith(QString("%1").arg(PLAN_SUZ))) {
        Packet pack(QString("planning stream:%1").arg(msg));
        sendPacket(pack);
    }
    else if(msg.startsWith("LOAD_REQUEST")) {
        msg.remove(0, QString("LOAD_REQUEST(").length());
        msg.chop(1);
        Packet pack(QString("loading request from file:%1").arg(msg));
        sendPacket(pack);
    }
}

