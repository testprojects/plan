#ifndef PROGRAMSETTINGS_H
#define PROGRAMSETTINGS_H

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
};

#endif // PROGRAMSETTINGS_H
