#ifndef TESTPLAN_H
#define TESTPLAN_H
#include <QObject>
#include <../myClient/types.h>
class Graph;

class TestPlan : public QObject
{
    Q_OBJECT
public:
    explicit TestPlan(QObject *parent = 0);

private slots:
    //тест на проверку расстояния между станциями
    void distanceBetweenStations();
    //тест на проверку загрузки на станции/ПВР/невозможности загрузки
    void canLoad();
    //тест на работу функции сдвига маршрута в зависимости от занятости участков
    void moveStream();
    //
    void syncDB();
private:
    Graph *gr;
};

#endif // TESTPLAN_H
