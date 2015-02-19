#include "stdafx.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "hudwindow.h"
#include "engine.h"

Track::Images Track::images;

void plot_torque_map(QCustomPlot* plot, Engine& engine)
{
    QVector<double> x(101), y(101);
    for (int i=0; i<101; ++i)
        x[i] = i/100.0; // x goes from -1 to 1
    for (qreal throttle = 0.1; throttle <= 1.05; throttle += 0.1) {
        plot->addGraph();
        for (int i=0; i<101; ++i)
            y[i] = engine.torque_map.get_torque(throttle, x[i]);
        plot->graph()->setData(x, y);
    }
    // give the axes some labels:
    plot->xAxis->setLabel("x");
    plot->yAxis->setLabel("y");
    // set axes ranges, so we see all data:
    plot->xAxis->setRange(0, 1);
    plot->yAxis->setRange(0, 1);
    //plot->graph(0)->setScatterStyle(QCPScatterStyle::ssDiamond);
    plot->replot();
}

void plot_rpm2torque(StaticMap& plot, Car& car) {
    QCPDataMap& data = *plot.plot.graph()->data();
    for (int i = 0; i < plot.x_values.size(); i++) {
        qreal const x = plot.x_values[i];
        data[x].value = car.engine.torque_map.get_torque(car.throttle, x);
    }
    plot.plot.graph(0)->clearData();
    plot.plot.graph(0)->addData(car.engine.rel_rpm(), car.engine.torque_map.get_torque(car.throttle, car.engine.rel_rpm()));
    plot.plot.replot();
}

void plot_throttle2torque(StaticMap& plot, Car& car) {
    QCPDataMap& data = *plot.plot.graph()->data();
    qreal engine_rpm = car.engine.rpm();
    for (int i = 0; i < plot.x_values.size(); i++) {
        qreal const x = plot.x_values[i];
        data[x].value = car.engine.torque_map.get_torque(x, engine_rpm / car.engine.max_rpm);
    }
    plot.plot.graph(0)->clearData();
    plot.plot.graph(0)->addData(car.throttle, car.engine.torque_map.get_torque(car.throttle, car.engine.rel_rpm()));
    plot.plot.replot();
}

void setup_plot(QCustomPlot* plot, QString xLabel, QString yLabel) {
    plot->xAxis->setLabel(xLabel);
    plot->yAxis->setLabel(yLabel);
    plot->xAxis->setRange(0, 1);
    plot->yAxis->setRange(0, 1);
    plot->replot();
    plot->addGraph();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , car(&osc)
    , timer(this)
{
    ui->setupUi(this);
    last_tab = ui->tabWidget->currentIndex();
#ifdef CAR_VIZ_FINAL_STUDY
    ui->tabWidget->setEnabled(false);
#endif

    Track::images.load_sign_images();
    ui->track_editor->init(ui->track_width, ui->track_points, ui->track_show_control_points,
                           ui->track_add_sign, ui->track_reset, ui->max_time, ui->track_prune_points,
                           ui->track_new_points_distance, ui->track_tl_min_time, ui->track_tl_max_time,
                           ui->track_tl_distance, ui->lbl_min_time, ui->lbl_max_time, ui->lbl_distance,
                           ui->steer_intensity, ui->steer_time, ui->steer_fade_in, ui->steer_fade_out,
                           ui->lbl_steer_intensity, ui->lbl_steer_time, ui->lbl_steer_fade_in, ui->lbl_steer_fade_out);
    ui->car_viz->init(&car, ui->start, ui->eyetracker, ui->vp_id, ui->current_condition, ui->next_condition,
                      ui->run, ui->throttle, ui->breaking, ui->gear, this, &osc);
    QObject::connect(ui->car_viz, SIGNAL(slow_tick(qreal,qreal, ConsumptionMonitor&)),
                     this, SLOT(update_plots(qreal,qreal,ConsumptionMonitor&)));
    QObject::connect(&car.gearbox, &Gearbox::gear_changed, [=](int gear){
        ui->gear->setValue(gear+1);
        osc.send_float("/gear", gear+1);
    });

#if 0
    plot_torque_map(ui->plot_speed, car.engine);
#else
    //setup plots
    setup_plot(ui->plot_speed, "time", "speed [kmh]");
    setup_plot(ui->plot_rpm, "time", "rpm");
    setup_plot(ui->plot_acceleration, "time", "acceleration [m/s^2]");
    setup_plot(ui->plot_resistance, "time", "resistane [N]");
    setup_plot(ui->plot_power, "time", "engine power output [kW]");
    setup_plot(ui->plot_consumption, "time", "consumption [L/s]");
    setup_plot(ui->plot_liters, "time", "relative consumption [L/100km]");
    setup_plot(ui->plot_rpm2torque, "rpm", "rel. torque");
    rpm2torque.reset(new StaticMap(*ui->plot_rpm2torque));
    setup_plot(ui->plot_throttle2torque, "rel. throttle", "rel. torque");
    throttle2torque.reset(new StaticMap(*ui->plot_throttle2torque));


    //QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(tick()));
    //timer.start(500);
    //timer.start(1000 / 33.);
#endif

}

MainWindow::~MainWindow()
{
    ui->car_viz->toggle_fedi(false);
    osc.call("/stopEngine");
    delete ui;
}

void add_plot_value(QCustomPlot* plot, qreal t, qreal y) {
    QCPDataMap& data = *plot->graph()->data();
    QCPData item;
    item.key = t;
    item.value = y;
#if 1
    while (data.size() > 0 && t - data.firstKey() > 60)
        data.remove(data.firstKey());
#endif
    data.insertMulti(item.key, item);
    plot->xAxis->setRange(data.firstKey(), data.lastKey());
    plot->yAxis->rescale();
//    plot->yAxis->setRange(
//                fmin(data.first().value, data.last().value),
//                fmax(data.first().value, data.last().value));
    plot->replot();

}

void MainWindow::update_plots(qreal, qreal elapsed, ConsumptionMonitor& consumption_monitor) {
    const qreal t = elapsed;
    add_plot_value(ui->plot_speed, t, car.gearbox.speed2kmh(car.speed));
    add_plot_value(ui->plot_rpm, t, car.engine.rpm());
    add_plot_value(ui->plot_acceleration, t, car.current_acceleration);
    add_plot_value(ui->plot_resistance, t, car.current_accumulated_resistance);
    add_plot_value(ui->plot_power, t, car.engine.power_output());
    add_plot_value(ui->plot_consumption, t, car.engine.get_consumption_L_s());
    add_plot_value(ui->plot_liters, t, std::min(40., car.engine.get_l100km(car.speed)));
    plot_rpm2torque(*rpm2torque, car);
    plot_throttle2torque(*throttle2torque, car);

    // update from user input
    car.gearbox.set_gear(ui->gear->value() - 1);
    //car.gearbox.auto_clutch_control(car.engine, dt);
    //ui->clutch->setValue(car.gearbox.clutch * 100);
    //car.gearbox.clutch = ui->clutch->value() / 100.;

    // update sound parameters
    osc.send_float("/rpm", 0.1 + car.engine.rel_rpm() * 0.8);
    //osc.send_float("/throttle", car.throttle);
    osc.send_float("/ml_sec", consumption_monitor.liters_per_second_cont * 1000);
    osc.send_float("/L_100km", consumption_monitor.liters_per_100km_cont);

}

void MainWindow::on_tabWidget_currentChanged(int index)
{
//    if (index != 1)
//        ui->tabWidget->setTab
    if (last_tab == 2) {
        ui->car_viz->copy_from_track_editor(ui->track_editor);
    }
    last_tab = index;
}

void MainWindow::on_actionOpen_Log_triggered()
{
    ui->car_viz->stop(); //true);
    QString filename = QFileDialog::getOpenFileName(this, "Open Log", QDir::homePath()+"/EcoSonic", "Log Files (*.log)");
    if (filename != "" && QFile(filename).exists()) {
        ui->car_viz->load_log(filename, true);
    } else {
        ui->car_viz->start();
    }
}


void MainWindow::on_hud_toggle_clicked()
{
    QCarViz& car_viz = *ui->car_viz;
    if (car_viz.hud_window.get() == nullptr) {
        car_viz.hud_window.reset(new HUDWindow());
        HUDWindow& hw = *car_viz.hud_window.get();
        QDesktopWidget d;
        if (d.screenCount() == 2) {
            QRect r1 = d.availableGeometry(0);
            hw.move(r1.center() - QPoint(hw.width() / 2, 0));
        }
        hw.show();
    } else {
        car_viz.hud_window.reset();
    }
}

void MainWindow::on_actionConvert_Log_triggered()
{
    ui->car_viz->stop(); //true);
    QString filename = QFileDialog::getOpenFileName(this, "Convert Log", QDir::homePath()+"/EcoSonic", "Log Files (*.log)");
    if (filename != "" && QFile(filename).exists()) {
        convert_log(filename);
    } else {
        ui->car_viz->start();
    }
}


void MainWindow::on_eyetracker_clicked()
{
    ui->car_viz->toggle_connect_to_eyetracker();
}

void MainWindow::on_vp_id_valueChanged(int arg1)
{
    if (arg1 % 1000 > 6) {
        ui->vp_id->setValue((arg1 / 1000 + 1) * 1000 + 1);
        return;
    }
    if (arg1 % 1000 < 1) {
        ui->vp_id->setValue((arg1 / 1000 - 1) * 1000 + 6);
        return;
    }
    ui->car_viz->set_condition_order(arg1 % 1000);
}

void MainWindow::on_current_condition_currentIndexChanged(int index)
{
    ui->car_viz->set_condition((Condition) index);
}

void MainWindow::on_reset_clicked()
{
    ui->car_viz->reset();
}

void MainWindow::on_actionConvert_All_Logs_in_a_Directory_triggered()
{
    ui->car_viz->stop(); //true);
    QString directory = QFileDialog::getExistingDirectory(this, "Convert all Logs in this directory", QDir::homePath()+"/EcoSonic");
    QDir dir(directory);
    if (directory != "" && dir.exists()) {
        QStringList files = dir.entryList(QStringList("*.log"));
        for (auto f : files) {
            convert_log(dir.filePath(f));
        }
        qDebug() << "done converting directory" << dir.absolutePath();
    } else {
        ui->car_viz->start();
    }

}

void MainWindow::convert_log(QString filename)
{
    qDebug() << "converting" << filename;
    ui->car_viz->load_log(filename, false);
    ui->car_viz->log_run();
    const int dot = filename.lastIndexOf('.');
    if (dot != -1)
        filename = filename.left(dot);
    filename += ".json";
    ui->car_viz->save_json(filename);
    qDebug() << filename << "saved";
}
