#ifndef FILTERSTREAM_H
#define FILTERSTREAM_H

#include <QVector>

class Stream;
class FilterStream
{
    public:
        explicit FilterStream();
        ~FilterStream();

        QVector<Stream*> filter(Stream**) const;
        void setTypeTransport(int from, int to);
        void setCodeRecipient(int form, int to);
        void setNumberStream(int from, int to);

    private:
        int typeTransportFrom;
        int typeTransportTo;
        int codeRecipientFrom;
        int codeRecipientTo;
        int numberStreamFrom;
        int numberStreamTo;
};

#endif // FILTERSTREAM_H
