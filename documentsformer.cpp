#include "documentsformer.h"
#include "mydb.h"
#include <QXmlStreamWriter>

DocumentsFormer::DocumentsFormer()
{
}

QByteArray DocumentsFormer::createF2(int VP_Start, int VP_End, int KP_Start, int KP_End,
                                     int NP_Start, int NP_End, QString grif)
{
    QByteArray ba;
    QXmlStreamWriter xmlWriter(&ba);
    //заполняем xml
    //обращаемся к базе через MyDB::instance()

    /* Writes a document start with the XML version number. */
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("students");

    xmlWriter.writeStartElement("student");
    /*add one attribute and its value*/
    xmlWriter.writeAttribute("name","Kate");
    /*add one attribute and its value*/
    xmlWriter.writeAttribute("surname","Johns");
    /*add one attribute and its value*/
    xmlWriter.writeAttribute("number","154455");
    /*add character data to tag student */
    xmlWriter.writeCharacters ("Student 1");
    /*close tag student  */
    xmlWriter.writeEndElement();

    /*end tag students*/
    xmlWriter.writeEndElement();
    /*end document */
    xmlWriter.writeEndDocument();

    return ba;
}
