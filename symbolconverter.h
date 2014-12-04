#ifndef SYMBOLCONVERTER_H
#define SYMBOLCONVERTER_H
#include <QString>

class symbolConverter
{
public:
    symbolConverter();
    static void toRUS();
    static QString convertChar(QString src);
};

#endif // SYMBOLCONVERTER_H
