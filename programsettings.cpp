#include "programsettings.h"
#include <QSettings>
#include <QDebug>

ProgramSettings::ProgramSettings()
{}

ProgramSettings::~ProgramSettings()
{
    if(_self)
        delete _self;
}

ProgramSettings* ProgramSettings::_self = 0;

ProgramSettings* ProgramSettings::instance()
{
    if(!_self) {
        _self = new ProgramSettings();
    }
    return _self;
}

void ProgramSettings::writeSettings()
{
    QSettings settings("GVC", "plan");
    settings.setDefaultFormat(QSettings::IniFormat);
    //настройки сокращений
    settings.setValue("abbreviations/ОФ", QString::fromUtf8("ОФИЦЕРЫ"));
    settings.setValue("abbreviations/С/С", QString::fromUtf8("СЕРЖАНТЫ И СОЛДАТЫ"));
    settings.setValue("abbreviations/А/М", QString::fromUtf8("АВТОМАШИНЫ"));
    settings.setValue("abbreviations/ПРЦ", QString::fromUtf8("ПРИЦЕПЫ"));
    settings.setValue("abbreviations/ТР", QString::fromUtf8("ТРАКТОРА"));
    settings.setValue("abbreviations/АВТОШИНЫ", QString::fromUtf8("АВТОШИНЫ"));
    settings.setValue("abbreviations/АВТОКРАНЫ", QString::fromUtf8("АВТОКРАНЫ ИЛИ А/К"));
    settings.setValue("abbreviations/КРАНЫ", QString::fromUtf8("КРАНЫ"));
    //настройки разделов
    settings.setValue("section/ОФ", "ЛЮДИ");
    settings.setValue("section/С/С", "ЛЮДИ");
    settings.setValue("section/А/М", "ABTOTPAHCПOPT C Л.C.");
    settings.setValue("section/ТР", "ABTOTPAHCПOPT C Л.C.");
    settings.setValue("section/АВТОШИНЫ", "ABTOШИHЫ");
    settings.setValue("section/АВТОКРАНЫ", "ABTOTPAHCПOPT C Л.C.");
    settings.setValue("section/КРАНЫ", "ABTOTPAHCПOPT C Л.C.");

    //QMap <код_груза, вид перевозок>
    settings.setValue("goods/1", "21");
    settings.setValue("goods/2", "22");
    settings.setValue("goods/3", "23");
    settings.setValue("goods/4", "24");
    settings.setValue("goods/5", "24");
    for(int i = 601; i <= 620; i++)
        settings.setValue(QString("goods/%1").arg(i), "24");
    for(int i = 70; i <= 77; i++)
        settings.setValue(QString("goods/%1").arg(i), "25");
    settings.setValue("goods/8", "27");

    //QMap <код_груза, тип груза в БД>
    //типы грузов: 23, 24BP, 24GSM, 24PR, 25
    settings.setValue("goodsDB/23", "23");
    settings.setValue("goodsDB/3", "23");
    settings.setValue("goodsDB/4", "24_BP");
    settings.setValue("goodsDB/5", "24_GSM");
    for(int i = 601; i <= 620; i++)
        settings.setValue(QString("goodsDB/%1").arg(i), "24_PR");
    for(int i = 70; i <= 77; i++)
        settings.setValue(QString("goodsDB/%1").arg(i), "25");

    //QMap <код_груза, наименование_груза>
    //VP == 24
    settings.setValue("goodsNames/4", "БОЕПРИПАСЫ");
    settings.setValue("goodsNames/5", "ГОРЮЧЕ-СМАЗОЧНЫЕ МАТЕРИАЛЫ");
    settings.setValue("goodsNames/601", "ПРОДОВОЛЬСТВИЕ");
    settings.setValue("goodsNames/602", "ВЕЩЕВОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/603", "ВООРУЖЕНИЕ");
    settings.setValue("goodsNames/604", "БРОНЕТАНКОВОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/605", "ИНЖЕНЕРНОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/606", "ИМУЩЕСТВО СВЯЗИ");
    settings.setValue("goodsNames/607", "ХИМИЧЕСКОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/608", "АВТОТРАКТОРНОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/609", "АВТОМОБИЛЬНОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/610", "АВИАТЕХНИЧЕСКОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/611", "ИНЖЕНЕРНО-АЭРОДРОМНОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/612", "ТЕХНИЧЕСКИЕ СРЕДСТВА И ИМУЩЕСТВО СЛУЖБЫ ГОРЮЧЕГО");
    settings.setValue("goodsNames/613", "ТОПОГРАФИЧЕСКОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/614", "ИМУЩЕСТВО МОРАЛЬНО-ПСИХОЛОГИЧЕСКОЙ ПОДГОТОВКИ И ВОСПИТАТЕЛЬНОЙ РАБОТЫ");
    settings.setValue("goodsNames/615", "МЕДИЦИНСКОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/616", "ВЕТЕРИНАРНОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/617", "КВАРТИРНО-ЭКСПЛУАТАЦИОННОЕ ИМУЩЕСТВО");
    settings.setValue("goodsNames/618", "ИМУЩЕСТВО ВОЕНТОРГА");
    settings.setValue("goodsNames/619", "МАТЕРИАЛЫ И ИМУЩЕСТВО ДЛЯ ВОССТАНОВЛЕНИЯ ЖЕЛЕЗНЫХ ДОРОГ");
    settings.setValue("goodsNames/620", "ДОРОЖНО-МОСТОВОЕ ИМУЩЕСТВО");
    //VP == 25
    settings.setValue("goodsNames/70", "ЛЮДИ");
    settings.setValue("goodsNames/71", "ЛЮДИ И ТЕХНИКА");
    settings.setValue("goodsNames/72", "ТЕХНИКА");
    settings.setValue("goodsNames/73", "АВТОШИНЫ");
    settings.setValue("goodsNames/74", "КРАНЫ");
    settings.setValue("goodsNames/75", "БРОНЕТЕХНИКА");
    settings.setValue("goodsNames/76", "ТАНКИ");
    settings.setValue("goodsNames/77", "АВТОМОБИЛИ");

    //QMap <номер_дороги, наименование_дороги>
    settings.setValue(QString("roads/1"), QString::fromUtf8("ОКТЯБРЬСКАЯ"));
    settings.setValue(QString("roads/2"), QString::fromUtf8("КАЛИНИНГРАДСКАЯ"));
    settings.setValue(QString("roads/3"), QString::fromUtf8("МОСКОВСКАЯ"));
    settings.setValue(QString("roads/4"), QString::fromUtf8("ГОРЬКОВСКАЯ"));
    settings.setValue(QString("roads/5"), QString::fromUtf8("СЕВЕРНАЯ"));
    settings.setValue(QString("roads/6"), QString::fromUtf8("СЕВЕРО-КАВКАЗСКАЯ"));
    settings.setValue(QString("roads/7"), QString::fromUtf8("ЮГО-ВОСТОЧНАЯ"));
    settings.setValue(QString("roads/8"), QString::fromUtf8("ПРИВОЛЖСКАЯ"));
    settings.setValue(QString("roads/9"), QString::fromUtf8("КУЙБЫШЕВСКАЯ"));
    settings.setValue(QString("roads/10"), QString::fromUtf8("СВЕРДЛОВСКАЯ"));
    settings.setValue(QString("roads/11"), QString::fromUtf8("ЮЖНО-УРАЛЬСКАЯ"));
    settings.setValue(QString("roads/12"), QString::fromUtf8("ЗАПАДНО-СИБИРСКАЯ"));
    settings.setValue(QString("roads/14"), QString::fromUtf8("КРАСНОЯРСКАЯ"));
    settings.setValue(QString("roads/15"), QString::fromUtf8("ВОСТОЧНО-СИБИРСКАЯ"));
    settings.setValue(QString("roads/16"), QString::fromUtf8("ЗАБАЙКАЛЬСКАЯ"));
    settings.setValue(QString("roads/17"), QString::fromUtf8("ДАЛЬНЕВОСТОЧНАЯ"));
    settings.setValue(QString("roads/19"), QString::fromUtf8("САХАЛИНСКАЯ"));
    settings.setValue(QString("roads/20"), QString::fromUtf8("КРЫМСКАЯ"));

}

void ProgramSettings::readSettings()
{
    QSettings settings("GVC", "plan");
    settings.setDefaultFormat(QSettings::IniFormat);
    QStringList list = settings.allKeys();
    foreach (QString key, list) {
        QString _key = key;
        if(_key.startsWith("abbreviations/")) {
            m_abbreviationsNA.insert(_key.remove(0, QString("abbreviations/").length()), settings.value(key).toString());
        }
        else if(_key.startsWith("section/")) {
            m_sectionsNA.insert(_key.remove(0, QString("section/").length()), settings.value(key).toString());
        }
        else if(_key.startsWith("goodsDB/")) {
            m_goodsTypesDB.insert(_key.remove(0, QString("goodsDB/").length()).toInt(), settings.value(key).toString());
        }
        else if(_key.startsWith("goods/")) {
            m_goodsTypes.insert(_key.remove(0, QString("goods/").length()).toInt(), settings.value(key).toInt());
        }
        else if(_key.startsWith("goodsNames/")) {
            m_goodsNames.insert(_key.remove(0, QString("goodsNames/").length()).toInt(), settings.value(key).toString());
        }
        else if(_key.startsWith("roads/")) {
            m_roads.insert(_key.remove(0, QString("roads/").length()).toInt(), settings.value(key).toString());
        }
    }

//    qDebug() << "Аббревиатуры:";
//    foreach (QString key, m_abbreviationsNA.keys()) {
//        qDebug() << QString("%1  --  %2")
//                    .arg(key)
//                    .arg(m_abbreviationsNA.value(key));
//    }

//    qDebug() << "Разделы:";
//    foreach (QString key, m_sectionsNA.keys()) {
//        qDebug() << QString("%1  --  %2")
//                    .arg(key)
//                    .arg(m_sectionsNA.value(key));
//    }

//    qDebug() << "Коды грузов:";
//    foreach (int key, m_goodsTypes.keys()) {
//        qDebug() << QString("%1  --  %2")
//                    .arg(key)
//                    .arg(m_goodsTypes.value(key));
//    }

//    qDebug() << "Типы грузов:";
//    foreach (int key, goodsTypesDB.keys()) {
//        qDebug() << QString("%1  --  %2")
//                    .arg(key)
//                    .arg(goodsTypesDB.value(key));
//    }

//    qDebug() << "Железные дороги:";
//    foreach (int key, m_roads.keys()) {
//        qDebug() << QString("%1  --  %2")
//                    .arg(key)
//                    .arg(m_roads.value(key));
//    }
}
