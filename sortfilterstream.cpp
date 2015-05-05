#include "sortfilterstream.h"
#include "mydb.h"
#include "stream.h"
#include "programsettings.h"

#include <QDebug>

SortFilterStream::SortFilterStream()
{
    groupDistricts = false;
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
    for (it = streams->constBegin(); it != streams->constEnd(); ++it) {
        smVP = (*it)->m_sourceRequest->VP;
        smKP = (*it)->m_sourceRequest->KP;
        smNP = (*it)->m_sourceRequest->NP;
        QVector<Station*> stations = (*it)->m_passedStations;
        numDistrictStationFirst = ProgramSettings::instance()->m_districts.value(stations.first()->VO);
        numDistrictStationLast = ProgramSettings::instance()->m_districts.value(stations.last()->VO);

        qDebug() << "F:" << stations.first()->VO << numDistrictStationFirst << "L:" << stations.last()->VO << numDistrictStationLast << "VP:" << smVP << "KP:" << smKP << "NP:" << smNP;
        //        if (typeTransportFrom <= smVP && smVP <= typeTransportTo)
        if (codeRecipientFrom <= smKP && smKP <= codeRecipientTo)
            if (numberStreamFrom <= smNP && smNP <= numberStreamTo) {
                if (groupDistricts) {
                    switch (typeGroup) {
                        case 1:
                        {
                            if (numDistrictStationLast != numDistrictStationFirst) {
                                if (militaryDistrict.contains(QString::number(numDistrictStationLast))) {
                                    qDebug() << "#" << "F:" << stations.first()->VO << numDistrictStationFirst << "L:" << stations.last()->VO << numDistrictStationLast << "VP:" << smVP << "KP:" << smKP << "NP:" << smNP;
                                    streamsFiltered.insert(numDistrictStationLast,(*it));
                                }
                            }
                            break;
                        }
                        case 2:
                        {
                            if (numDistrictStationFirst != numDistrictStationLast) {
                                if (militaryDistrict.contains(QString::number(numDistrictStationFirst))) {
                                    streamsFiltered.insert(numDistrictStationFirst,(*it));
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
                                        streamsFiltered.insert(numDistrictStation,(*it));
                                }
                            }
                            break;
                        }
                    }
                }
                else
                    streamsFiltered.insert(0, (*it));
            }
    }
    qDebug() << "count filter:" << streamsFiltered.size();
    qDebug() << "numbers district:" << streamsFiltered.keys();
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
    groupDistricts = true;
    typeGroup = type;
    militaryDistrict = districts;
}
