#ifndef ENGINE_H
#define ENGINE_H

#include "consumption_map.h"
#include "torque_map.h"

class Engine
{
public:

    Engine(const qreal max_rpm, const qreal max_torque);

    // throttle (0..1)
    inline qreal update_torque(qreal throttle)
    {
        if (throttle < min_throttle && rpm() < 700) // ansonsten: Schubabschaltung!
            throttle = min_throttle;
        torque = max_torque * torque_map.get_torque(throttle, rel_rpm());
        torque_out = torque - engine_braking_coefficient * pow(rpm() / 60, 1.1)
                            - engine_braking_offset;
        return torque_out;
    }

    // returns [kW]
    inline qreal power_output() {
        return angular_velocity * torque / 1000;
    }

    inline qreal get_consumption() {
        qreal const power = power_output();
        qreal rel_consumption = consumption_map.get_rel_consumption(rel_rpm(), torque / max_torque);
        return rel_consumption * base_consumption * power;
    }

    // consumption [g/h], speed [m/s]
    inline qreal get_l100km(qreal speed, qreal consumption = -1) {
        if (speed < 0.001)
            return 0;
        if (consumption == -1)
            consumption = get_consumption();
        consumption /= 60*60; // [g/s]
        consumption /= speed * 1000; // [kg/m]
        consumption /= 0.75; // /dichte [kg/L] => [L/m]
        return consumption * 100 * 1000; // [L/100km]
    }

    inline double rpm() const { return angular_velocity * 60 / (2*M_PI); }
    inline void set_rpm(qreal rpm) { angular_velocity = rpm2angular_velocity(rpm); }
    inline double rel_rpm() const { return rpm() / max_rpm; }
    static inline double rpm2angular_velocity(qreal const rpm) { return rpm * 2 * M_PI / 60; }

    ConsumptionMap consumption_map;
    qreal base_consumption;
    TorqueMap torque_map;
    qreal const max_rpm; // [u/min]
    qreal const max_torque; // [N*m]
    qreal const engine_braking_coefficient = 0.9;
    qreal const engine_braking_offset = 0; // [N*m]
    qreal angular_velocity = 0; // [rad/s]
    //qreal rpm; // current rpm [u/min]
    qreal torque; // [N*m]
    qreal torque_out; // [N*m]
    qreal torque_counter; // [N*m] "counter force"
    qreal min_throttle = 0.07; // min throttle
    qreal inertia = 0.13; // [kg m^2]
};

#endif // ENGINE_H
