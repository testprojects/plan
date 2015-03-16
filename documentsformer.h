#ifndef DOCUMENTSFORMER_H
#define DOCUMENTSFORMER_H

#include <QByteArray>

class Stream;
class DocumentsFormer
{
public:
    DocumentsFormer();
    static QByteArray createXmlForm2(const QVector<Stream*>);
};

#endif // DOCUMENTSFORMER_H
