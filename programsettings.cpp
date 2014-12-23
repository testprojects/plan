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
}

void ProgramSettings::readSettings()
{
    QSettings settings("GVC", "plan");
    QStringList list;
    list.append(settings.value("abbreviations/ТР").toString());
    list.append(settings.value("abbreviations/ОФ").toString());
    list.append(settings.value("abbreviations/С/С").toString());
    qDebug() << "File: " << settings.fileName();
    qDebug() << "Format: " << settings.format();
    qDebug() << "Settings:";
    foreach (QString buf, list) {
        qDebug() << buf;
    }
}
