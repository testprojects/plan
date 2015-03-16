#ifndef LOOPWRAPPER_H
#define LOOPWRAPPER_H

#include <QEventLoop>
class Server;
class LoopWrapper : public QEventLoop
{
    Q_OBJECT
public:
    explicit LoopWrapper(QObject *parent = 0);

signals:

public slots:
    void acceptedOffset(bool bAccepted);

private:
};

#endif // LOOPWRAPPER_H
