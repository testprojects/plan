#ifndef MYTIME_H
#define MYTIME_H
class QString;
class MyTime
{
private:
    float m_days;
    float m_hours;
    float m_minutes;

public:
    MyTime();
    MyTime(float days, float hours, float minutes);// (0д, -15ч, -5мин)

    void adjust();                              //перевести, чтобы было правильно

    float days() const {return m_days;}
    float hours() const {return m_hours;}
    float minutes() const {return m_minutes;}

public:
    int toMinutes() const {return m_days * 24 * 60 + m_hours * 60 + m_minutes;}
    //возвращает округлённое в большую сторону значение часов
    int toHours() const {
        if(m_minutes == 0)
            return m_days * 24 + m_hours;
        else
            return m_days * 24 + m_hours + 1;
    }

    static MyTime timeFromHours(float hours);
    static MyTime timeFromMinutes(float minutes);

    MyTime operator+ (const MyTime t) const;
    MyTime operator- (const MyTime t) const;
    MyTime operator* (int n) const;
    bool operator== (const MyTime t) const;
    bool operator< (const MyTime t) const;
    bool operator> (const MyTime t) const;
    bool operator>= (const MyTime t) const;
    bool operator<= (const MyTime t) const;
    operator QString () const;
};

#endif // TIME_H
