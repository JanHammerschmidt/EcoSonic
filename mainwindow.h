#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "car.h"

#include <lib/qcustomplot/qcustomplot.h>
#include "OSCSender.h"

class StaticMap {
public:
    StaticMap(QCustomPlot& ref, const int max_x = 100)
        : x_values(max_x + 1)
        , plot(ref)
    {
        plot.addGraph();
        plot.graph(0)->data()->insertMulti(0.5, QCPData(0.5, 0.5));
        plot.graph(0)->setScatterStyle(QCPScatterStyle::ssDiamond);
        for (int i=0; i<=max_x; i++) {
            x_values[i] = (qreal)i/max_x; // x goes from -1 to 1
        }
        QVector<qreal> y(x_values.size(), 0);
        plot.graph()->setData(x_values, y);
    }

//    void plot(std::function<qreal(qreal)> f) {
//        QCPDataMap& data = *plot.plot.graph()->data();
//        for (int i = 0; i < plot.x_values.size(); i++) {
//            qreal const x = plot.x_values[i];
//            data[x].value = f(x);
//        }
//        plot.plot.replot();
//    }

//protected:
    QVector<qreal> x_values;
    QCustomPlot& plot;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void update_plots(qreal dt, qreal elapsed, ConsumptionMonitor& consumption_monitor);
//    void tick();

private slots:
    void on_tabWidget_currentChanged(int index);

    void on_actionOpen_Log_triggered();

    void on_track_prune_points_clicked();

private:
    Ui::MainWindow *ui;
    Car car;
    QTimer timer;
    OSCSender osc;
    std::auto_ptr<StaticMap> rpm2torque, throttle2torque;
    int last_tab = 1;
    //StaticMap *torque_rpm;
    //StaticMap *torque_rpm
};

#endif // MAINWINDOW_H
