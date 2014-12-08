#include "car.h"
#include "logging.h"

qreal Car::tick(qreal const dt, qreal alpha)
{
    if (dt <= 0)
        return 0;

    //logging
    if (log) {
        log->add_item(throttle, braking, gearbox.gear, dt);
    }

    // get forward force
    Q_ASSERT(!isnan(engine.angular_velocity));
    engine.update_torque(throttle);
    qreal F = gearbox.torque2force_engine2wheels(engine, speed, dt);
    current_wheel_force = F;

    // get resisting forces
    qreal drag_resistance = resistances::drag(drag_resistance_coefficient, speed);
    qreal rolling_resistance = resistances::rolling(rolling_resistance_coefficient, alpha, mass);
    qreal uphill_resistance = resistances::uphill(mass, alpha);
    current_accumulated_resistance = drag_resistance + rolling_resistance + uphill_resistance;
    F -= current_accumulated_resistance;
    F -= braking * max_breaking_force; // breaking force

    // calculate acceleration
    qreal const mass_factor = gearbox.mass_factors[gearbox.gear];
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
