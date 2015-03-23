#ifndef PS_H
#define PS_H
#include <QString>

//подвижной состав
class PS
{
public:
    PS(): pass(0), luds(0), krit(0), kuhn(0), plat(0), polu(0), spec(0), ledn(0), cist(0), total(0) {}
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
    QStringList getPS();

//    void operator =(const PS &psSrc);
};

#endif // PS_H
