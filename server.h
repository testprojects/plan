#ifndef SERVER_H
#define SERVER_H
#include <QObject>

class QTcpServer;
class QTcpSocket;
class Packet;

//! [0]
class Server : public QObject
{
    Q_OBJECT

public:
    Server();

private slots:
    void openSession();
    void listenClient();
    void readMessage();
    void displayMessage();
    void printDisconnected();
    void sendPacket(const Packet &pack);
    void dispatchMessage();

signals:
    void messageReady();

private:
    QTcpServer *m_tcpServer;
    QTcpSocket *m_tcpSocket;
    QString m_currentMessage;
};
//! [0]

#endif
