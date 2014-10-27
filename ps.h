#ifndef PS_H
#define PS_H
#include <QString>

//подвижной состав
class PS
{
public:
    int pass;
    int luds;
    int krit;
    int kuhn;
    int plat;
    int polu;
    int spec;
    int ledn;
    int cist;
    int total;

    QString getString();

//    void operator =(const PS &psSrc);
};

#endif // PS_H
