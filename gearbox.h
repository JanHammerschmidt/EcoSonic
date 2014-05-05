#ifndef GEARBOX_H
#define GEARBOX_H

#include "engine.h"

struct Gearbox
{
    Gearbox()
        : gear(0)
        , end_transmission(4.764)
        , rolling_circumference(193)
        , wheel_radius(0.216)
        , clutch(0.0)
    {
        qreal g[] = {3.266, 2.130, 1.517, 1.147, 0.921, 0.738}; // übersetzungen
        qreal mf[] = {1.4,1.28,1.15,1.1,1.07,1.06}; // massenfaktoren
        gears.resize(ARR_SIZE(g));
        memcpy(gears.data(), g, sizeof(g));
        mass_factors.resize(ARR_SIZE(mf));
        memcpy(mass_factors.data(), mf, sizeof(mf));
    }

    void update_engine_speed(Engine& engine, qreal speed, qreal dt) {
        if (clutch >= 1.0)
            engine.set_rpm(speed2engine_rpm(speed));
        else
            engine.angular_velocity += (engine.torque_out - engine.torque_counter) / engine.inertia * dt;
        return;
        if (clutch == 0) {
            engine.angular_velocity += engine.torque_out / engine.inertia * dt;
        } else {
            qreal rpm = speed2engine_rpm(speed);
            if (clutch < 1.0) {
                    //taken from TORCS
                    //const qreal transf = pow(clutch,2); // 2
                    //const qreal freerads = engine.angular_velocity + engine.torque_out / engine.inertia * dt;
                    //engine.angular_velocity = transf * Engine::rpm2angular_velocity(rpm) + (1 - transf) * freerads;
                const qreal counter_torque = std::max(clutch * 30, clutch * engine.torque_out);
                const qreal torque = engine.torque_out - counter_torque;
                engine.angular_velocity += torque / engine.inertia * dt;
            } else {
                engine.set_rpm(rpm);
            }
        }
    }

    // speed [m/s]
    qreal inline speed2engine_rpm(qreal speed) {
        qreal rpm = speed * 100 / rolling_circumference; // [u/sek]
        rpm *= (gears[gear] * end_transmission);
        Q_ASSERT(!isnan(rpm));
        return rpm * 60; // [u/min]
    }

    void auto_clutch_control(Engine& engine, const qreal dt) {
        static bool releasing = true;
        if (releasing) {
            if (clutch >= 1.0)
                releasing = false;
            if (engine.rpm() > 4000 || clutch > 0) {
                clutch += 0.5 * dt;
                //clutch += (engine.rpm() - 3000) * 0.00001;
            }
        } else {
            // keeps being applied so far ..
        }
    }

    // returns [N]
    qreal torque2force_engine2wheels(Engine& engine, const qreal speed, const qreal dt) {
        if (!clutch) {
            engine.torque_counter = 0;
            clutch_max_rpm_diff = abs(engine.rpm() - speed2engine_rpm(speed));
            return 0;
        }
        engine.torque_counter = engine.torque_out;
        if (clutch < 1) {
            qreal rpm = speed2engine_rpm(speed); // what the engine rpm should be
            qreal rpm_diff = engine.rpm() - rpm;
            qreal max_rpm_diff = clutch_max_rpm_diff * (1-clutch); // linear?
            if (abs(rpm_diff) > max_rpm_diff) {
                engine.torque_counter = engine.inertia / dt * Engine::rpm2angular_velocity(
                            rpm_diff > 0 ? rpm_diff-max_rpm_diff : (max_rpm_diff+rpm_diff));
            }
        }
        return engine.torque_counter * gears[gear] * end_transmission / wheel_radius;
    }

    void gear_up() {
//        if (clutch == 0)

        gear = std::min(gear+1, gears.size()-1);
    }
    void gear_down() { gear = std::max(gear -1, 0); }
    void engine_idle() { gear = 0; clutch = 0; }

    // returns [kW], torque [N*m], rpm [s^-1]
    static inline qreal torque2power(qreal const torque, qreal const rpm) {
        return (2 * M_PI * (rpm / 60) * torque) / 1000;
    }

    // speed is [m/s]
    static inline qreal speed2kmh(qreal const speed) {
        return speed * 3.6; // * 60^2 / 1000
    }
    static inline qreal kmh2speed(qreal const kmh) {
        return kmh / 3.6;
    }

    int gear;
    QVector<qreal> gears; // gear-transmissions
    QVector<qreal> mass_factors;
    qreal end_transmission;
    qreal rolling_circumference; // cm
    qreal wheel_radius; // m
    qreal clutch; // (0..1)
    qreal clutch_max_rpm_diff = 4000;
};

#endif // GEARBOX_H
