#ifndef PAUSER_H
#define PAUSER_H
#include <QEventLoop>
class QTcpSocket;

class Pauser : public QEventLoop
{
    Q_OBJECT
public:
    explicit Pauser(QTcpSocket *socket, quint32 blockSize);

public slots:
    void checkIfDataRecieved();

private:
    QTcpSocket *m_socket;
    quint32 m_blockSize;
};

#endif // PAUSER_H
