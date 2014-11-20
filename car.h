#ifndef CAR_H
#define CAR_H

#include "engine.h"
#include "gearbox.h"
#include "resistances.h"

class Car
{
public:
    // mass [kg]
    inline Car(qreal mass = 1500, qreal drag_resistance_coefficient = -1, qreal rolling_resistance_coefficient = 0.015)
        : mass(mass)
        , drag_resistance_coefficient(drag_resistance_coefficient)
        , rolling_resistance_coefficient(rolling_resistance_coefficient)
        , engine(7000, 240) // ?? 180 // 250
    {
        this->drag_resistance_coefficient = (drag_resistance_coefficient == -1) ?
                resistances::drag_resistance_coefficient(0.3, 2) // ??
                : drag_resistance_coefficient;
    }

    // throttle: (0..1), alpha: up/downhill [rad]
    // returns acceleration [m/s^2]
    inline qreal tick(qreal const dt, qreal alpha)
    {
        if (dt <= 0)
            return 0;

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
        F -= breaking * max_breaking_force; // breaking force

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

    qreal mass; // [kg]
    qreal speed = 0; // [m/s]
    qreal throttle = 0; // (0..1)
    qreal breaking = 0; // (0..1)
    qreal max_breaking_force = 20000;
    qreal drag_resistance_coefficient;
    qreal rolling_resistance_coefficient;

    qreal current_accumulated_resistance;
    qreal current_wheel_force;
    qreal current_acceleration;

    Engine engine;
    Gearbox gearbox;
};

#endif // CAR_H
