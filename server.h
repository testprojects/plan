#ifndef SERVER_H
#define SERVER_H
#include <QObject>

class QTcpServer;
class QTcpSocket;
class Packet;
class Graph;
class PlanThread;

//! [0]
class Server : public QObject
{
    Q_OBJECT
public:
    Server();
    ~Server();

public slots:
    void sendPacket(Packet &pack);
    void sendMessage(QString msg);

public slots:
    void openSession();
    void listenClient();
    void readMessage();
    void printDisconnected();
    void dispatchMessage(QString msg);
    void cacheOut();

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

public:
    QTcpSocket *m_tcpSocket;
    QTcpServer *m_tcpServer;
    Graph *m_graph;
    PlanThread *planThread;
    QString m_currentMessage;
    quint32 m_blockSize;
};
//! [0]

#endif
