#ifndef LOOPWRAPPER_H
#define LOOPWRAPPER_H

#include <QThread>
class Server;
class Packet;
class LoopWrapper : public QThread
{
    Q_OBJECT
    void run();
public:
    LoopWrapper(Server *server, Packet *packet);

signals:
    void signalOffsetAccepted(bool bAccepted);

public slots:
    void quit();

private:
    Server *m_server;
    Packet *m_packet;
    QThread *mainThread;
};

#endif // LOOPWRAPPER_H
