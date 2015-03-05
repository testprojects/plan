#ifndef FILTERSTREAM_H
#define FILTERSTREAM_H

#include <QVector>

class Stream;
class FilterStream
{
    public:
        explicit FilterStream();
        ~FilterStream();

        void filter();
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
        QVector<Stream*> streamsFiltered;
};

#endif // FILTERSTREAM_H
