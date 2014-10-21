#include "mytime.h"
#include <QString>

MyTime::MyTime(): m_days(0), m_hours(0), m_minutes(0)
{
}

MyTime::MyTime(int days, int hours, int minutes): m_days(days), m_hours(hours), m_minutes(minutes)
{
    adjust();
}

MyTime MyTime::timeFromHours(int hours)
{
    return MyTime(hours/24, hours%24, 0);
}

MyTime MyTime::timeFromMinutes(int minutes)
{
    return MyTime(minutes / 1440, minutes / 60, minutes % 60);
}

MyTime MyTime::operator +(MyTime t)
{
    return MyTime(this->days() + t.days(), this->hours() + t.hours(), this->minutes() + t.minutes());
}

MyTime MyTime::operator -(MyTime t)
{
    return MyTime(this->days() - t.days(), this->hours() + t.hours(), this->minutes() + t.minutes());
}

void MyTime::adjust()
{
//    if((m_minutes >= 60)) {
//        m_hours += m_minutes / 60;
//        m_minutes = m_minutes % 60;
//    }
//    if(m_minutes < 0) {
//        m_hours -= m
//    }
    if(qAbs(m_minutes) >= 60) {
        m_hours += m_minutes / 60;
        m_minutes = m_minutes % 60;
    }
    if(qAbs(m_hours) >= 24) {
        m_days += m_hours / 24;
        m_hours = m_hours % 24;
    }

    if(m_minutes < 0) {
        --m_hours;
        m_minutes += 60;
    }
    if(m_hours < 0) {
        --m_days;
        m_hours += 24;
    }

}

QString MyTime::getString()
{
    return QString::fromUtf8("день:%1 час:%2 минута:%3").arg(m_days).arg(m_hours).arg(m_minutes);
}
