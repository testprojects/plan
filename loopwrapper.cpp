#include "loopwrapper.h"
#include "server.h"
#include <QTcpSocket>

LoopWrapper::LoopWrapper(QObject *parent) :
    QEventLoop(parent)
{
}

void LoopWrapper::acceptedOffset(bool bAccepted)
{
    this->exit((int)bAccepted);
}
