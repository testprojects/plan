#ifndef SORTFILTERSTREAM_H
#define SORTFILTERSTREAM_H

#include <QVector>
#include <QStringList>

class Stream;
class SortFilterStream
{
    public:
        explicit SortFilterStream();
        ~SortFilterStream();

        QMap<int, Stream*> filter(QVector<Stream*> *data) const;
        void setCodeRecipientRange(int from, int to);
        void setNumberStreamRange(int from, int to);
        void setGroupDistricts(int type, const QStringList &districts);

    private:
        int typeTransportFrom;
        int typeTransportTo;
        int codeRecipientFrom;
        int codeRecipientTo;
        int numberStreamFrom;
        int numberStreamTo;
        int typeGroup;
        QStringList militaryDistrict;
};

#endif // SORTFILTERSTREAM_H
