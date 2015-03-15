#include "documentsformer.h"
#include "mydb.h"
#include <QXmlStreamWriter>
#include <QDebug>

#include <QFile>
#include <QTextStream>

DocumentsFormer::DocumentsFormer()
{
}

QByteArray DocumentsFormer::createXmlForm2(const QVector<Stream*> data)
{
    QByteArray output;
    QXmlStreamWriter xmlWriter(&output);
    xmlWriter.setAutoFormatting(true);
    //заполняем xml

    /* Writes a document start with the XML version number. */
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("document");
    QVector<Stream*>::const_iterator i;
    for (i = data.begin(); i != data.end(); ++i) {
        xmlWriter.writeStartElement("stream");
        xmlWriter.writeStartElement("typeTransport");
        xmlWriter.writeCharacters(QString::number((*i)->m_sourceRequest->VP));
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("codeRecipient");
        xmlWriter.writeCharacters(QString::number((*i)->m_sourceRequest->KP));
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("numberStream");
        xmlWriter.writeCharacters(QString::number((*i)->m_sourceRequest->NP));
        xmlWriter.writeEndElement();
        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();

    return output;
}
