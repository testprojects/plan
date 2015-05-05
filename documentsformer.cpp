#include "documentsformer.h"
#include "mydb.h"
#include <QXmlStreamWriter>
#include <QDebug>

#include "station.h"


#include <QFile>

DocumentsFormer::DocumentsFormer()
{
}

QByteArray DocumentsFormer::createXmlForm2(const QMap<int, Stream*> data)
{
    QByteArray output;
    QXmlStreamWriter xmlWriter(&output);
    xmlWriter.setAutoFormatting(true);
    // заполняем xml

    /* Writes a document start with the XML version number. */
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("document");

    int district = 0;       // 0 => not exist VO
    QMap<int, Stream*>::const_iterator it = data.constBegin();
    while (it != data.constEnd()) {
        // create header group
        if (it.key() != 0 && district != it.key()) {
            district = it.key();
            xmlWriter.writeStartElement("district");
            xmlWriter.writeCharacters(QString::number(district));
            xmlWriter.writeEndElement();
        }

        Echelon *echelons = (*it)->m_echelones.data();
        int numEchelons = (*it)->m_echelones.size();
        Station **stations = (*it)->m_passedStations.data();
        int numStations = (*it)->m_passedStations.size();
        QStringList numberEchelons = QStringList();
        QStringList timeStations = QStringList();
        QList<int> numMandatoryStation = (*it)->m_sourceRequest->OM;

        // get list num. echelons
        int j = 0;
        while (j < (*it)->m_echelones.size()) {
            numberEchelons << QString::number(echelons[j].number);
            j++;
        }

        // get list station route and time passing
        j = 0;
        int roadNumber;
        while (j < numStations) {
            if (j == 0 || j == (numStations - 1) || numMandatoryStation.contains(stations[j]->number)) {
                MyTime time = echelons[0].timesArrivalToStations[j];
                timeStations << QString("%1 T %2 %3").arg(stations[j]->name, QString::number(time.days()), QString::number(time.hours()));
                roadNumber = stations[j]->roadNumber;
            }
            else if (roadNumber != stations[j]->roadNumber) {
                roadNumber = stations[j]->roadNumber;

                MyTime time = echelons[0].timesArrivalToStations[j - 1];
                timeStations << QString("%1 T %2 %3").arg(stations[j - 1]->name, QString::number(time.days()), QString::number(time.hours()));
                time = echelons[0].timesArrivalToStations[j];
                timeStations << QString("%1 T %2 %3").arg(stations[j]->name, QString::number(time.days()), QString::number(time.hours()));
            }
            j++;
        }

        xmlWriter.writeStartElement("stream");
        xmlWriter.writeStartElement("codeRecipient");
        xmlWriter.writeCharacters(QString::number((*it)->m_sourceRequest->KP));
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("numberStream");
        xmlWriter.writeCharacters(QString::number((*it)->m_sourceRequest->NP));
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("numberEchelons");
        xmlWriter.writeCharacters(numberEchelons.join(",\n"));
        xmlWriter.writeEndElement();
//        xmlWriter.writeStartElement("numberState");
//        xmlWriter.writeCharacters((*it)->m_sourceRequest->SH);
//        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("nameCarried");
        xmlWriter.writeCharacters(echelons->NA);
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("numberCarried");
        xmlWriter.writeCharacters(echelons->ps.getPS().join("  "));
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("numberTrains");
        xmlWriter.writeCharacters(QString::number((*it)->m_sourceRequest->PK));
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("rate");
        xmlWriter.writeCharacters(QString::number((*it)->m_sourceRequest->TZ));
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("sender");
        xmlWriter.writeCharacters((*it)->m_sourceRequest->OT);
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("readiness");
        xmlWriter.writeCharacters(QString("T %1 %2").arg(QString::number((*it)->m_sourceRequest->DG),
                                  QString::number((*it)->m_sourceRequest->CG)));
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("recipient");
        xmlWriter.writeCharacters((*it)->m_sourceRequest->PY);
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("stationLoading");
        xmlWriter.writeCharacters(MyDB::instance()->stationByNumber((*it)->m_sourceRequest->SP)->name);
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("timeDepartureFirstEchelone");
        MyTime timeDepartureFirstEchelone = echelons[0].timesArrivalToStations.at(0);
        xmlWriter.writeCharacters(QString("T %1 %2").arg(QString::number(timeDepartureFirstEchelone.days()),
                                                         QString::number(timeDepartureFirstEchelone.hours())));
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("timeDepartureLastEchelone");
        MyTime timeDepartureLastEchelone = echelons[numEchelons - 1].timesArrivalToStations.at(0);
        xmlWriter.writeCharacters(QString("T %1 %2").arg(QString::number(timeDepartureLastEchelone.days()),
                                                         QString::number(timeDepartureLastEchelone.hours())));
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("stationUnloading");
        xmlWriter.writeCharacters(MyDB::instance()->stationByNumber((*it)->m_sourceRequest->SV)->name);
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("timeArrivalFirstEchelone");
        MyTime timeArrivalFirstEchelone = echelons[0].timesArrivalToStations.last();
        xmlWriter.writeCharacters(QString("T %1 %2").arg(QString::number(timeArrivalFirstEchelone.days()),
                                                         QString::number(timeArrivalFirstEchelone.hours())));
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("timeArrivalLastEchelone");
        MyTime timeArrivalEndEchelone = echelons[numEchelons - 1].timesArrivalToStations.last();
        xmlWriter.writeCharacters(QString("T %1 %2").arg(QString::number(timeArrivalEndEchelone.days()),
                                                         QString::number(timeArrivalEndEchelone.hours())));
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("route");
        xmlWriter.writeCharacters(timeStations.join("  "));
        xmlWriter.writeEndElement();

        xmlWriter.writeEndElement();
        ++it;
    }
    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();


#ifndef DEBUG:
    QFile file("test_output.xml");
    QTextStream out(&file);
    if (file.open(QIODevice::WriteOnly)) {
        out << output;
        file.close();
    }
#endif

    return output;
}
