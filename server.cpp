#include <QtNetwork>
#include <QDebug>

#include <stdlib.h>
#include <iostream>

#include "server.h"
#define PORT 1535

Server::Server()
: m_tcpServer(0), m_currentMessage("empty")
{
    openSession();
    connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(listenClient()));
    connect(this, SIGNAL(messageRecieved()), this, SLOT(dispatchMessage()));
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

void Server::sendMessage(QString message)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;
    out << message;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));

    m_tcpSocket->write(block);
    if(m_tcpSocket->flush())
        qDebug() << "Writed message: " << message;
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
    emit messageRecieved();
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
    if(msg.startsWith("PLAN_STREAM")) {
        msg.remove(0, QString("PLAN_STREAM(").length());
        msg.chop(1);
        qDebug() << "Dispathced message: PLAN_STREAM";
        sendMessage(QString("planning_stream:%1").arg(msg));
    }
    else if(msg.startsWith("LOAD_REQUEST")) {
        msg.remove(0, QString("LOAD_REQUEST(").length());
        msg.chop(1);
        qDebug() << "Dispathced message: LOAD_REQUEST";
        sendMessage(QString("loading_request_from_file:%1").arg(msg));
    }
}

