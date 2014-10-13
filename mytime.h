#ifndef MYTIME_H
#define MYTIME_H

class MyTime
{
private:
    int m_days;
    int m_hours;
    int m_minutes;

public:
    MyTime();
    MyTime(int days, int hours, int minutes);
    void adjust();                              //перевести, чтобы было правильно

    int days() const {return m_days;}
    int hours() const {return m_hours;}
    int minutes() const {return m_minutes;}

    static MyTime timeFromHours(int hours);
    static MyTime timeFromMinutes(int minutes);

    MyTime operator+(MyTime t);
    MyTime operator-(MyTime t);
};

#endif // TIME_H
