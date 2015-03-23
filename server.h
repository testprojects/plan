#ifndef SERVER_H
#define SERVER_H
#include <QObject>

class Pauser;
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

public slots:
    void openSession();
    void listenClient();
    void readMessage();
    void displayMessage(QString msg);
    void printDisconnected();
    void dispatchMessage(QString msg);

signals:
    void messageReady(QString);

signals:
    void signalPlanStreams(int VP, int KP, int NP_Start, int NP_End, bool SUZ);
    void signalOffsetAccepted(bool);

private slots:
    void   slotPlanStreams(int VP, int KP, int NP_Start, int NP_End, bool SUZ);
    void   slotOffsetAccepted(bool bAccepted);

public:
    QTcpSocket* getClient() {return m_tcpSocket;}
    Pauser* getPauser() {return m_pauser;}

public:
    QTcpSocket *m_tcpSocket;
    QTcpServer *m_tcpServer;
    Graph *m_graph;
    QString m_currentMessage;
    Pauser *m_pauser;
};
//! [0]

#endif
