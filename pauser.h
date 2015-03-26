#ifndef PAUSER_H
#define PAUSER_H
#include <QEventLoop>

class QTcpSocket;
#include "../myClient/packet.h"

class Pauser : public QEventLoop
{
    Q_OBJECT
public:
    explicit Pauser();

public slots:
    void accept(bool bAccept);
};

#endif // PAUSER_H
