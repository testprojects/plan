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

    //QMap <код_груза, тип груза в БД>
    //типы грузов: 23, 24BP, 24GSM, 24PR, 25
    settings.setValue("goods/1", "21");
    settings.setValue("goods/2", "22");
    settings.setValue("goods/23", "23");
    settings.setValue("goods/4", "24");
    settings.setValue("goods/5", "24");
    for(int i = 601; i <= 620; i++)
        settings.setValue(QString("goods/%1").arg(i), "24");
    for(int i = 70; i <= 77; i++)
        settings.setValue(QString("goods/%1").arg(i), "25");
    settings.setValue("goods/8", "27");
}

void ProgramSettings::readSettings()
{
    QSettings settings("GVC", "plan");
    QStringList list = settings.allKeys();
    foreach (QString key, list) {
        if(key.startsWith("abbreviations")) {
            abbreviationsNA.insert(key, settings.value(key).toString());
        }
        else if(key.startsWith("section")) {
            sectionsNA.insert(key, settings.value(key).toString());
        }
        else if(key.startsWith("goods")) {
            goodsTypes.insert(key, settings.value(key).toString());
        }
    }

    qDebug() << "Аббревиатуры:";
    foreach (QString key, abbreviationsNA.keys()) {
        qDebug() << QString("%1  --  %2")
                    .arg(key)
                    .arg(abbreviationsNA.value(key));
    }

    qDebug() << "Разделы:";
    foreach (QString key, sectionsNA.keys()) {
        qDebug() << QString("%1  --  %2")
                    .arg(key)
                    .arg(sectionsNA.value(key));
    }

    qDebug() << "Коды грузов:";
    foreach (QString key, goodsTypes.keys()) {
        qDebug() << QString("%1  --  %2")
                    .arg(key)
                    .arg(goodsTypes.value(key));
    }

}
