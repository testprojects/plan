#include "pauser.h"
#include <QTcpSocket>

Pauser::Pauser()
{
}

void Pauser::accept(bool bAccept)
{
    qDebug() << "Pauser::accept(" << bAccept << ")";
    this->exit(bAccept);
}
