#ifndef SERVER_H
#define SERVER_H
#include <QObject>

class QTcpServer;
class QTcpSocket;
class Packet;
class Graph;

//! [0]
class Server : public QObject
{
    Q_OBJECT
public:
    Server();

public slots:
    void sendPacket(Packet &pack);

private slots:
    void openSession();
    void listenClient();
    void readMessage();
    void displayMessage();
    void printDisconnected();
    void dispatchMessage();

signals:
    void messageReady();

signals:
    void signalPlanStreams(int VP, int KP, int NP_Start, int NP_End, bool SUZ);
    void signalOffsetAccepted(bool);

private slots:
    void   slotPlanStreams(int VP, int KP, int NP_Start, int NP_End, bool SUZ);

public:
    QTcpSocket* getClient() {return m_tcpSocket;}
private:
    QTcpSocket *m_tcpSocket;
    QTcpServer *m_tcpServer;
    Graph *m_graph;
    QString m_currentMessage;
};
//! [0]

#endif
