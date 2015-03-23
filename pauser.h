#ifndef PAUSER_H
#define PAUSER_H
#include <QEventLoop>

class QTcpSocket;
#include "../myClient/packet.h"

class Pauser : public QEventLoop
{
    Q_OBJECT
public:
    explicit Pauser(QTcpSocket *socket);
    void setBlockSize(quint32 size);

public slots:
    void checkIfDataRecieved();

private:
    QTcpSocket *m_socket;
    quint32 m_blockSize;
};

#endif // PAUSER_H
