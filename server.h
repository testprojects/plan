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

private slots:
    void openSession();
    void listenClient();
    void readMessage();
    void displayMessage();
    void printDisconnected();
    void sendPacket(Packet &pack);
    void dispatchMessage();

signals:
    void messageReady();

signals:
    void signalPlanStreams(int VP, int KP, int NP_Start, int NP_End, bool SUZ);

private slots:
    void   slotPlanStreams(int VP, int KP, int NP_Start, int NP_End, bool SUZ);

private:
    QTcpServer *m_tcpServer;
    QTcpSocket *m_tcpSocket;
    Graph *m_graph;
    QString m_currentMessage;
};
//! [0]

#endif
