#include "stdafx.h"
#include <QDateTime>
#include "car.h"
#include "logging.h"
#include "qcarviz.h"

void Car::save_log(const bool intro_run, QDateTime& program_start_time) {
    Q_UNUSED(program_start_time);
    Q_ASSERT(log != nullptr);
    //log->save(QDir::homePath()+"/EcoSonic/"+program_start_time.toString("yyyy-MM-dd_hh-mm")+"/"+QDateTime::currentDateTime().toString("mm-ss")+".log");
    QString filename;
    QTextStream f(&filename);
    f << QDir::homePath()+"/EcoSonic/";
    f << "VP" << log->vp_id << "/" << log->vp_id << "_";
    if (intro_run)
        f << "intro-run" << "_";
    else {
        f << log->global_run_counter << "_";
        f << Log::condition_string(log->condition) << "_";
        f << log->run << "_";
    }
    f << QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    f << ".log";
    qDebug() << filename;
    log->save(filename);
    log.reset();
}

qreal Car::tick(qreal dt, qreal const alpha, bool const replay)
{
    //dt = 0.017;
    if (dt <= 0)
        return 0;
//    if (dt > 0.1)
//        dt = 0.1;

    //logging
    if (log && !replay) {
        log->add_item(throttle, braking, gearbox.get_gear(), dt);
    }

    // get forward force
    Q_ASSERT(!isnan(engine.angular_velocity));
    gearbox.tick(dt);
    engine.update_torque(gearbox.gear_change() ? 0 : throttle);
    qreal F = gearbox.torque2force_engine2wheels(engine, speed, dt);
    //current_wheel_force = F;

    // get resisting forces
    qreal drag_resistance = resistances::drag(drag_resistance_coefficient, speed);
    qreal rolling_resistance = resistances::rolling(rolling_resistance_coefficient, alpha, mass);
    qreal uphill_resistance = resistances::uphill(mass, alpha);
    current_single_resistance = drag_resistance;
    osc->send_float("/uphill_resistance", uphill_resistance);
    current_accumulated_resistance = drag_resistance + rolling_resistance + uphill_resistance;

    F -= current_accumulated_resistance;
    F -= braking * max_breaking_force; // breaking force

    // calculate acceleration
    qreal const mass_factor = gearbox.mass_factors[gearbox.get_gear()];
    qreal a;
    if (F > 0) {
        a = F / (mass * mass_factor);
    } else {
        a = F * mass_factor / mass;
        //a = (F * (1+engine.rpm*0.01) / mass); // TODO: das ist sicherlich nicht *ganz* richtig...
    }
    // update speed & rpm
    speed += a * dt;
    if (speed < 0) // TODO: sure?
        speed = 0;
    Q_ASSERT(!isnan(speed));
    Q_ASSERT(!isnan(engine.angular_velocity));
    gearbox.update_engine_speed(engine, speed, dt);
    Q_ASSERT(!isnan(engine.angular_velocity));
    current_acceleration = a;
    return a;
}
