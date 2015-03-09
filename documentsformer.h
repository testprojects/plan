#ifndef DOCUMENTSFORMER_H
#define DOCUMENTSFORMER_H

#include <QByteArray>

class DocumentsFormer
{
public:
    DocumentsFormer();
    static QByteArray createF2(int VP_Start, int VP_End, int KP_Start, int KP_End, int NP_Start, int NP_End, QString grif);
};

#endif // DOCUMENTSFORMER_H
