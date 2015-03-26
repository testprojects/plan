#include "pauser.h"
#include "assert.h"
#include "server.h"
#include "../myClient/types.h"
#include <QTcpSocket>
#include <QDebug>

Pauser::Pauser()
{
}

void Pauser::accept(bool bAccept)
{
    this->exit((bool)bAccept);
}
