#ifndef TESTPLAN_H
#define TESTPLAN_H

#include <QObject>
class Graph;

class TestPlan : public QObject
{
    Q_OBJECT
public:
    explicit TestPlan(QObject *parent = 0);

private slots:
    //тест на проверку расстояния между станциями
    void distanceBetweenStations();
    void canLoad();
    void moveStream();

private:
    Graph *gr;
};

#endif // TESTPLAN_H
