#include "mytime.h"
#include <QString>

MyTime::MyTime(): m_days(0.0), m_hours(0.0), m_minutes(0.0)
{
}

MyTime::MyTime(float days, float hours, float minutes): m_days(days), m_hours(hours), m_minutes(minutes)
{
    adjust();
}

MyTime MyTime::timeFromHours(float hours)
{
    int days = (int)(hours / 24.0);
    hours -= days * 24;
    return MyTime(days, hours, 0);
}

MyTime MyTime::timeFromMinutes(float minutes)
{
    int days = (int) (minutes / 1440.0);
    minutes -= days * 1440;
    int hours = (int) (minutes / 60.0);
    minutes -= hours * 60;
    return MyTime(days, hours, minutes);
}

MyTime MyTime::operator +(const MyTime t) const
{
    return MyTime(this->days() + t.days(), this->hours() + t.hours(), this->minutes() + t.minutes());
}

MyTime MyTime::operator -(const MyTime t) const
{
    return MyTime(this->days() - t.days(), this->hours() - t.hours(), this->minutes() - t.minutes());
}

MyTime MyTime::operator *(int n) const
{
    return MyTime(this->days() * n, this->hours() * n, this->days() * n);
}

bool MyTime::operator ==(const MyTime t) const
{
    return ((this->days() == t.days()) && (this->hours() == t.hours()) && (this->minutes() == t.minutes()));
}

bool MyTime::operator <(const MyTime t) const
{
    return (this->toMinutes() < t.toMinutes());
}

bool MyTime::operator >(const MyTime t) const
{
    return (this->toMinutes() > t.toMinutes());
}

bool MyTime::operator <=(const MyTime t) const
{
    return (this->toMinutes() <= t.toMinutes());
}

bool MyTime::operator >=(const MyTime t) const
{
    return (this->toMinutes() >= t.toMinutes());
}

MyTime::operator QString () const
{
    return QString::fromUtf8("%1:%2:%3(%4ч.)")
            .arg(static_cast<int>(m_days + 1))
            .arg(static_cast<int>(m_hours))
            .arg(static_cast<int>(m_minutes))
            .arg(toHours());
}

void MyTime::adjust()
{
    if(qAbs(m_minutes) >= 60) {
        m_hours += (int) (m_minutes / 60.0);
        m_minutes = m_minutes - 24 * ((int)m_minutes % 60);
    }
    if(qAbs(m_hours) >= 24) {
        m_days += (int) (m_hours / 24.0);
        m_hours = m_hours - 24 * ((int)m_hours % 24);
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
