#ifndef SORTFILTERSTREAM_H
#define SORTFILTERSTREAM_H

#include <QVector>
#include <QMap>
#include <QStringList>

class Stream;
class SortFilterStream
{
    public:
        explicit SortFilterStream();
        ~SortFilterStream();

        QMap<int, QVector<Stream*> > filter(QVector<Stream*> *data);
        void setTypeTransport(int value);
        void setCodeRecipientRange(int from, int to);
        void setNumberStreamRange(int from, int to);
        void setGroupDistricts(int type, const QStringList &districts);
        void setGroupCodeCargo(bool isGrouped) { groupCodeCargo = isGrouped; }

    private:
        int typeTransport;
        int codeRecipientFrom;
        int codeRecipientTo;
        int numberStreamFrom;
        int numberStreamTo;
        int typeGroup;
        bool groupDistricts;
        bool groupCodeCargo;
        QStringList militaryDistrict;
        QMap<int, QVector<Stream*> > streamsFiltered;

        void sortByCodeCargo();
};

#endif // SORTFILTERSTREAM_H
