#include "loopwrapper.h"
#include "server.h"
#include <QTcpSocket>

LoopWrapper::LoopWrapper(Server *server, Packet *packet):
    m_server(server), m_packet(packet), mainThread(QThread::currentThread())
{
}

void LoopWrapper::run()
{
    qDebug() << "loop thread: " << QThread::currentThread();
    m_server->moveToThread(QThread::currentThread());
    connect(m_server, SIGNAL(signalOffsetAccepted(bool)), m_server, SLOT(slotOffsetAccepted(bool)));
    connect(m_server, SIGNAL(signalOffsetAccepted(bool)), this, SLOT(quit()));
    m_server->sendPacket(*m_packet);
}

void LoopWrapper::quit()
{
    m_server->moveToThread(mainThread);
    QThread::currentThread()->quit();
}
