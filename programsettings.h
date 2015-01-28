#ifndef PROGRAMSETTINGS_H
#define PROGRAMSETTINGS_H
#include <QString>
#include <QMap>

class ProgramSettings
{
private:
    static ProgramSettings* _self;
    ProgramSettings();
    ProgramSettings(const ProgramSettings&);
    ProgramSettings& operator =(const ProgramSettings&);
    virtual ~ProgramSettings();
public:
    static ProgramSettings *instance();

public:
    void writeSettings();
    void readSettings();

public:
    QMap<QString, QString> m_abbreviationsNA; //<аббревиатура NA 25VP, полное наименование NA 25 VP>
    QMap<QString, QString> m_sectionsNA; //<аббревиатура NA 25VP, раздел NA 25VP>
    QMap<int, int> m_goodsTypes; //<код_груза, вид_перевозок>
    QMap<int, QString> m_goodsTypesDB; //<код_груза, поле_в_БД> (23, 24BP, 24GSM, 24PR, 25)
    QMap<int, QString> m_roads; //<номер_дороги, наименование_дороги>
};

#endif // PROGRAMSETTINGS_H
