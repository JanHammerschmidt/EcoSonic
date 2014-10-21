#include "qcarviz.h"

QCarViz::QCarViz(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    QPalette p = palette();
    p.setColor(backgroundRole(), Qt::white);
    setPalette(p);
    flash_timer.start();

    printf("%s\n", QDir::currentPath().toStdString().c_str());

    add_tree_type("tree", 0.2/3, 50*3);
    add_tree_type("birch", 0.08, 140);
    add_tree_type("spooky_tree", 0.06, 120);

    car_img.load("media/cars/car.png");
    track.load();
    update_track_path(height());
    fill_trees();
    tick_timer.setInterval(30); // minimum simulation interval
    QObject::connect(&tick_timer, SIGNAL(timeout()), this, SLOT(tick()));
}

void QCarViz::init(Car* car, QPushButton* start_button, QSlider* throttle, QSlider* breaking, QSpinBox* gear, QMainWindow* main_window, OSCSender* osc, bool start)
{
    this->car = car;
    this->start_button = start_button;
    throttle_slider = throttle;
    breaking_slider = breaking;
    gear_spinbox = gear;
    speedObserver.reset(new SpeedObserver(*this, *osc));
    QObject::connect(start_button, SIGNAL(clicked()),
                     this, SLOT(start_stop()));
    keyboard_input.init(main_window);
    consumption_monitor.osc = osc;
    if (start)
        QTimer::singleShot(500, this, SLOT(start()));
}

void QCarViz::copy_from_track_editor(QTrackEditor* track_editor)
{
    track = track_editor->track;
    track.sort_signs();
    update_track_path(height());
    speedObserver->tick();
    fill_trees();
}



bool QCarViz::tick() {
    Q_ASSERT(started);
    qreal dt;
    if (!time_delta.get_time_delta(dt))
        return false;

    static bool changed = false;
    // keyboard input
    if (keyboard_input.update()) {
        car->throttle = keyboard_input.throttle();
        car->breaking = keyboard_input.breaking();
        changed = true;
    }
    const int gear_change = keyboard_input.gear_change();
    if (gear_change) {
        if (gear_change > 0)
            car->gearbox.gear_up();
        else if (gear_change < 0)
            car->gearbox.gear_down();
        gear_spinbox->setValue(car->gearbox.gear+1);
    }
    const bool toggle_clutch = keyboard_input.toggle_clutch();
    if (toggle_clutch) {
        Clutch& clutch = car->gearbox.clutch;
        if (clutch.engage)
            clutch.disengage();
        else
            clutch.clutch_in(car->engine, &car->gearbox, car->speed);
    }
    if (track_started)
        car->gearbox.auto_clutch_control(car);


    // pedal input
    if (pedal_input.valid() && pedal_input.update()) {
        car->throttle = pedal_input.gas();
        car->breaking = pedal_input.brake();
        changed = true;
    }
    if (!track_started && car->throttle > 0) {
        track_started = true;
        track_started_time = time_delta.get_elapsed();
    }


    const qreal alpha = !track_path.length() ? 0 : atan(-track_path.slopeAtPercent(track_path.percentAtLength(current_pos))); // slope [rad]
    Q_ASSERT(!isnan(alpha));
    car->tick(dt, alpha);
    current_pos += car->speed * dt * 3;
    if (current_pos >= track_path.length()) {
        //current_pos = 0;
        //current_pos = track_path.length();
        stop();
    }
    if (track_started)
        consumption_monitor.tick(car->engine.get_consumption_L_s(), dt, car->speed);
    double l_100km;
    if (consumption_monitor.get_l_100km(l_100km, car->speed)) {
        hud.l_100km = l_100km;
        //printf("%.3f\n", l_100km);
    }

    static qreal last_elapsed = 0;
    qreal elapsed = time_delta.get_elapsed();
    if (elapsed - last_elapsed > 0.05) {
        if (changed) {
            throttle_slider->setValue(car->throttle * 100);
            breaking_slider->setValue(car->breaking * 100);
            changed = false;
        } else {
            car->throttle = throttle_slider->value() / 100.;
            car->breaking = breaking_slider->value() / 100.;
        }
        speedObserver->tick();
        emit slow_tick(elapsed - last_elapsed, elapsed, consumption_monitor);
        last_elapsed = elapsed;
    }
    return true;
}

void QCarViz::draw(QPainter& painter)
{
    if (flash_timer.elapsed() < 100)
        return;
    const qreal current_percent = track_path.percentAtLength(current_pos);
    const qreal car_x_pos = 50;

    const QPointF cur_p = track_path.pointAtPercent(current_percent);
    //printf("%.3f\n", current_alpha * 180 / M_PI);

    //draw the current speed limit / current_pos / time elapsed
    QString number; number.sprintf("%04.1f", time_delta.get_elapsed() - track_started_time);
    painter.setFont(QFont{"Eurostile", 18, QFont::Bold});
    QPointF p = {300,300};
    if (track_started)
        painter.drawText(p, number);


    // draw the HUD speedometer & revcounter
    hud.draw(painter, width(), car->engine.rpm(), Gearbox::speed2kmh(car->speed), consumption_monitor.liters_used);

    // draw the road
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::NoBrush);
    QTransform t;
    t.translate(car_x_pos - cur_p.x(),0);
    painter.setTransform(t);
    painter.drawPath(track_path);

    // draw the signs
    for (int i = 0; i < track.signs.size(); i++) {
        track.signs[i].draw(painter, track_path);
    }

    // draw the car
    const qreal car_width = 60.;
    const qreal car_height = (qreal) car_img.size().height() / car_img.size().width() * car_width;

    t.reset();
    t.translate(car_x_pos, cur_p.y());
    //t.rotateRadians(atan(slope));
    t.rotate(-track_path.angleAtPercent(current_percent));
    t.translate(-car_width/2, -car_height);
    painter.setTransform(t);
    painter.drawImage(QRectF(0,0, car_width, car_height), car_img);

    // draw the trees in the foreground
    t.reset();
    t.translate(car_x_pos - cur_p.x(),0);
    painter.setTransform(t);

    const qreal track_bottom = track_path.boundingRect().bottom();
    for (Tree tree : trees) {
        const qreal tree_x = tree.track_x(cur_p.x());
        if (tree_x > size().width() + cur_p.x() + 200)
            continue;
        QPointF tree_pos(tree_x, track_bottom);
        tree_types[tree.type].draw_scaled(painter, tree_pos, Gearbox::speed2kmh(car->speed), tree.scale);
    }
}
