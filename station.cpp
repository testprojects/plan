#include "station.h"
#include <QDataStream>

bool Station::operator== (const Station &st2) const {
    return (this->number == st2.number);
}

bool Station::operator!= (const Station &st2) const {
    return (this->number != st2.number);
}

Station::operator QString () const
{
    return QString("%1 (%2)")
            .arg(name)
            .arg(number);
}

QByteArray Station::encode() const
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;

    out << number << type << name << latitude << longitude
        << startNumber << endNumber << distanceTillStart << distanceTillEnd
        << pvrNumber << loadingCapacity23 << loadingCapacity24_BP << loadingCapacity24_GSM
        << loadingCapacity24_PR << loadingCapacity25;
    for(int i = 0; i < 60; i++) out << loadingPossibilities23[i];
    for(int i = 0; i < 60; i++) out << loadingPossibilities24_BP[i];
    for(int i = 0; i < 60; i++) out << loadingPossibilities24_GSM[i];
    for(int i = 0; i < 60; i++) out << loadingPossibilities24_PR[i];
    for(int i = 0; i < 60; i++) out << loadingPossibilities25[i];
    out << roadNumber;

    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    return block;
}
