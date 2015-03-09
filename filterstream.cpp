#include "filterstream.h"
#include "mydb.h"
#include "stream.h"

#include <QDebug>

FilterStream::FilterStream()
{

}

FilterStream::~FilterStream()
{

}

QVector<Stream*> FilterStream::filter(Stream **sm)
{
    int smVP;
    int smKP;
    int smNP;
    int i = 0;
    QVector<Stream*> streamsFiltered;

    while (true) {
        if (sm[i] != 0) {
            smVP = sm[i]->m_sourceRequest->VP;
            smKP = sm[i]->m_sourceRequest->KP;
            smNP = sm[i]->m_sourceRequest->NP;
            if (typeTransportFrom <= smVP && smVP <= typeTransportTo)
                if (codeRecipientFrom <= smKP && smKP <= codeRecipientTo)
                    if (numberStreamFrom <= smNP && smNP <= numberStreamTo)
                        streamsFiltered << sm[i];
        }
        else
            break;
        i++;
    }
    qDebug() << "count:" << streamsFiltered.size() << endl;
    return streamsFiltered;
}

void FilterStream::setTypeTransport(int from, int to)
{
    typeTransportFrom = from;
    typeTransportTo = to;
}

void FilterStream::setCodeRecipient(int from, int to)
{
    codeRecipientFrom = from;
    codeRecipientTo = to;
}

void FilterStream::setNumberStream(int from, int to)
{
    numberStreamFrom = from;
    numberStreamTo = to;
}

