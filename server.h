#ifndef SERVER_H
#define SERVER_H
#include <QObject>

class QTcpServer;
class QTcpSocket;
class QNetworkSession;
class QAction;

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
    void sendMessage(QString message);
    void dispatchMessage();

signals:
    void messageRecieved();

private:
    QTcpServer *m_tcpServer;
    QTcpSocket *m_tcpSocket;
    QString m_currentMessage;
};
//! [0]

#endif
