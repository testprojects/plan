#ifndef DOCUMENTSFORMER_H
#define DOCUMENTSFORMER_H

#include <QByteArray>
#include <QMap>

class Stream;
class DocumentsFormer
{
public:
    DocumentsFormer();
    static QByteArray createXmlForm2(const QMap<int, QVector<Stream*> >);
};

#endif // DOCUMENTSFORMER_H
