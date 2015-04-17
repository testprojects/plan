#include "sortfilterstream.h"
#include "mydb.h"
#include "stream.h"
#include "programsettings.h"

#include <QDebug>

SortFilterStream::SortFilterStream()
{

}

SortFilterStream::~SortFilterStream()
{

}

QMap<int, Stream*> SortFilterStream::filter(QVector<Stream*> *streams) const
{
    int smVP = 0;
    int smKP = 0;
    int smNP = 0;
    int numDistrictStationFirst = 0;
    int numDistrictStationLast = 0;
    QMultiMap<int, Stream*> streamsFiltered;

    QVector<Stream*>::const_iterator it;
    for (it = streams->begin(); it != streams->end(); ++it) {
        smVP = (*it)->m_sourceRequest->VP;
        smKP = (*it)->m_sourceRequest->KP;
        smNP = (*it)->m_sourceRequest->NP;
        QVector<Station*> stations = (*it)->m_passedStations;
        numDistrictStationFirst = ProgramSettings::instance()->m_districts.key(stations.first()->VO);
        numDistrictStationLast = ProgramSettings::instance()->m_districts.key(stations.last()->VO);

//        if (typeTransportFrom <= smVP && smVP <= typeTransportTo)
        if (codeRecipientFrom <= smKP && smKP <= codeRecipientTo)
            if (numberStreamFrom <= smNP && smNP <= numberStreamTo) {
                switch (typeGroup) {
                    case 1:
                    {
                        if (numDistrictStationLast != numDistrictStationFirst) {
                            if (militaryDistrict.contains(QString::number(numDistrictStationLast))) {
                                streamsFiltered.insert(numDistrictStationLast,(*it));
                                qDebug() << numDistrictStationLast << stations.last()->VO;
                            }
                        }
                        break;
                    }
                    case 2:
                    {
                        if (numDistrictStationFirst != numDistrictStationLast) {
                            if (militaryDistrict.contains(QString::number(numDistrictStationFirst)))
                                streamsFiltered.insert(numDistrictStationFirst,(*it));
                        }
                        break;
                    }
                    case 3:
                    {
                        if (numDistrictStationFirst != numDistrictStationLast && stations.length() > 2) {
                            for (int i = 1; i < (stations.length() - 1); i++) {
                                int numDistrictStation = ProgramSettings::instance()->m_districts.key(stations[i]->VO);
                                if (militaryDistrict.contains(QString::number(numDistrictStation)))
                                    streamsFiltered.insert(numDistrictStation,(*it));
                            }
                        }
                        break;
                    }
                }
            }
    }
    qDebug() << "count filter:" << streamsFiltered.size() << endl;
    qDebug() << "numbers district:" << streamsFiltered.keys() << endl;
    return streamsFiltered;
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
    typeGroup = type;
    militaryDistrict = districts;
}
