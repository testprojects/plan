#include "pauser.h"
#include "assert.h"
#include <QTcpSocket>
#include <QDebug>

Pauser::Pauser(QTcpSocket *socket, quint32 blockSize) :
    m_socket(socket), m_blockSize(blockSize)
{
    assert(socket);
}

void Pauser::checkIfDataRecieved()
{
    if(m_socket->bytesAvailable() >= m_blockSize)
        quit();
}
