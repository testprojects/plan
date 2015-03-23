#include "pauser.h"
#include "assert.h"
#include "server.h"
#include "../myClient/types.h"
#include <QTcpSocket>
#include <QDebug>

Pauser::Pauser(QTcpSocket *socket) :
    m_socket(socket)
{
    assert(m_socket);
}

void Pauser::checkIfDataRecieved()
{
    if(m_socket->bytesAvailable() >= m_blockSize)
        quit();
}

void Pauser::setBlockSize(quint32 size)
{
    m_blockSize = size;
}
