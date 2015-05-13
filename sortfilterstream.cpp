#include "sortfilterstream.h"
#include "mydb.h"
#include "stream.h"
#include "programsettings.h"

#include <QDebug>

SortFilterStream::SortFilterStream()
{
    groupDistricts = false;
    groupCodeCargo = false;
}

SortFilterStream::~SortFilterStream()
{

}

QMap<int, QVector<Stream*> > SortFilterStream::filter(QVector<Stream*> *streams)
{
    int smVP = 0;
    int smKP = 0;
    int smNP = 0;
    int numDistrictStationFirst = 0;
    int numDistrictStationLast = 0;

    int key = 0;
    QVector<Stream*>::const_iterator it;
    for (it = streams->constBegin(); it != streams->constEnd(); ++it) {
        smVP = (*it)->m_sourceRequest->VP;
        smKP = (*it)->m_sourceRequest->KP;
        smNP = (*it)->m_sourceRequest->NP;
        QVector<Station*> stations = (*it)->m_passedStations;
        numDistrictStationFirst = ProgramSettings::instance()->m_districts.value(stations.first()->VO);
        numDistrictStationLast = ProgramSettings::instance()->m_districts.value(stations.last()->VO);

        if (typeTransport == smVP && codeRecipientFrom <= smKP && smKP <= codeRecipientTo &&
                numberStreamFrom <= smNP && smNP <= numberStreamTo) {
            if (groupDistricts) {
                switch (typeGroup) {    // type: 1 - in, 2 - out, 3 - transit
                    case 1:
                    {
                        if (numDistrictStationLast != numDistrictStationFirst) {
                            if (militaryDistrict.contains(QString::number(numDistrictStationLast))) {
                                qDebug() << "#" << "F:" << stations.first()->VO << numDistrictStationFirst << "L:" << stations.last()->VO << numDistrictStationLast << "VP:" << smVP << "KP:" << smKP << "NP:" << smNP;
                                key = numDistrictStationLast;
                            }
                        }
                        break;
                    }
                    case 2:
                    {
                        if (numDistrictStationFirst != numDistrictStationLast) {
                            if (militaryDistrict.contains(QString::number(numDistrictStationFirst))) {
                                key = numDistrictStationFirst;
                            }
                        }
                        break;
                    }
                    case 3:
                    {
                        if (numDistrictStationFirst != numDistrictStationLast && stations.length() > 2) {
                            for (int i = 1; i < (stations.length() - 1); i++) {
                                int numDistrictStation = ProgramSettings::instance()->m_districts.key(stations[i]->VO);
                                if (militaryDistrict.contains(QString::number(numDistrictStation)))
                                    key = numDistrictStation;
                            }
                        }
                        break;
                    }
                }
                if (key == 0)
                    continue;

                QVector<Stream*> streams = streamsFiltered[key];
                streams.append(*it);
                streamsFiltered[key] = streams;
            }
            else {
                QVector<Stream*> streams = streamsFiltered[key];
                streams.append(*it);
                streamsFiltered[key] = streams;
            }
            key = 0;
        }
    }
    if (groupCodeCargo)
        sortByCodeCargo();

    qDebug() << "count filter:" << streamsFiltered.size();
    qDebug() << "numbers district:" << streamsFiltered.keys();
    return streamsFiltered;
}

void SortFilterStream::setTypeTransport(int value)
{
    typeTransport = value;
}

void SortFilterStream::setCodeRecipientRange(int from, int to)
{
    codeRecipientFrom = from;
    codeRecipientTo = to;
}

void SortFilterStream::setNumberStreamRange(int from, int to)
{
    numberStreamFrom = from;
    numberStreamTo = to;
}

void SortFilterStream::setGroupDistricts(int type, const QStringList &districts)
{
    groupDistricts = true;
    typeGroup = type;
    militaryDistrict = districts;
}

void SortFilterStream::sortByCodeCargo()
{
    QMap<int, QVector<Stream*> >::const_iterator it;
    for (it = streamsFiltered.constBegin(); it != streamsFiltered.constEnd(); ++it) {
        QList<Stream*> streamsList((*it).toList());
        QList<int> codeCargList;

        for (int i = 0; i < streamsList.size(); i++)
            codeCargList << streamsList[i]->m_sourceRequest->KG;

        int i, j;
        for (int n = 0; n < codeCargList.size() - 1; n++)
            for (i = codeCargList.size() - 1, j = i - 1; i > 0; i--, j--) {
                if (codeCargList[i] < codeCargList[j]) {
                    streamsList.move(i, j);
                    codeCargList.move(i, j);
                }
            }
        streamsFiltered[it.key()] = streamsList.toVector();
    }
}
