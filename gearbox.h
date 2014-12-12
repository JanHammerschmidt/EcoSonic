#ifndef GEARBOX_H
#define GEARBOX_H

#include "engine.h"

class Gearbox;
class Car;

// kinematic clutch, loosely based on
// http://blog.xogeny.com/blog/part-2-kinematic/
// Only simulating time-based engagement (not disengagement!)
struct Clutch
{
    inline void clutch_in(const Engine& engine, const Gearbox* const gearbox, const double speed);
    inline double counter_torque(const Engine& engine, const Gearbox* const gearbox, const double speed, const double dt);

    void disengage() {
        this->engage = false;
    }
    bool acting() {
        return engage && t <= t_shift;
    }
    void update(const double dt) { t += dt; }

    double a_w; // acceleration (a) of the difference of the rotation speeds (w)
    /*const*/ double t_shift = 0.8; // shift time (seconds)
    double t = t_shift; // accumulated time since beginning of engagement
    double w_t0; // delta of rotation speeds at t0
    bool engage = false; // is engaged or should engage
};

class Gearbox : public QObject
{
    Q_OBJECT
public:
    Gearbox()
        : gear(0)
        , end_transmission(4.764)
        , rolling_circumference(193)
        , wheel_radius(0.216)
    {
        qreal g[] = {3.266, 2.130, 1.517, 1.147, 0.921, 0.738}; // übersetzungen
        qreal mf[] = {1.4,1.28,1.15,1.1,1.07,1.06}; // massenfaktoren
        gears.resize(ARR_SIZE(g));
        memcpy(gears.data(), g, sizeof(g));
        mass_factors.resize(ARR_SIZE(mf));
        memcpy(mass_factors.data(), mf, sizeof(mf));
    }

    void reset() {
        gear = 0;
        emit gear_changed(0);
    }

    void update_engine_speed(Engine& engine, qreal speed, qreal dt) {
        if (!clutch.acting() && clutch.engage) { // clutch is fully engaged
            engine.set_rpm(speed2engine_rpm(speed));
        } else {
            const double delta_w = (engine.torque_out - engine.torque_counter) / engine.inertia;
            engine.angular_velocity += delta_w * dt;
//            if (clutch.engage)
//                printf("is: %.3f (%.3f)\n", delta_w, Engine::angular_velocity2rpm(delta_w));
        }
        Q_ASSERT(!isnan(engine.angular_velocity));

//        if (clutch >= 1.0)
//            engine.set_rpm(speed2engine_rpm(speed));
//        else
//            engine.angular_velocity += (engine.torque_out - engine.torque_counter) / engine.inertia * dt;
//        return;
//        if (clutch == 0) {
//            engine.angular_velocity += engine.torque_out / engine.inertia * dt;
//        } else {
//            qreal rpm = speed2engine_rpm(speed);
//            if (clutch < 1.0) {
//                    //taken from TORCS
//                    //const qreal transf = pow(clutch,2); // 2
//                    //const qreal freerads = engine.angular_velocity + engine.torque_out / engine.inertia * dt;
//                    //engine.angular_velocity = transf * Engine::rpm2angular_velocity(rpm) + (1 - transf) * freerads;
//                const qreal counter_torque = std::max(clutch * 30, clutch * engine.torque_out);
//                const qreal torque = engine.torque_out - counter_torque;
//                engine.angular_velocity += torque / engine.inertia * dt;
//            } else {
//                engine.set_rpm(rpm);
//            }
//        }
    }

    // speed [m/s]
    qreal inline speed2engine_rpm(qreal speed) const {
        qreal rpm = speed * 100 / rolling_circumference; // [u/sek]
        rpm *= (gears[gear] * end_transmission);
        Q_ASSERT(!isnan(rpm));
        return rpm * 60; // [u/min]
    }

    // (only clutching in!)
    void auto_clutch_control(Car* car);

//    void auto_clutch_control(Engine& engine, const qreal dt) {
//        static bool releasing = true;
//        if (releasing) {
//            if (clutch >= 1.0)
//                releasing = false;
//            if (engine.rpm() > 4000 || clutch > 0) {
//                clutch += 0.5 * dt;
//                //clutch += (engine.rpm() - 3000) * 0.00001;
//            }
//        } else {
//            // keeps being applied so far ..
//        }
//    }

    // returns [N]
    qreal torque2force_engine2wheels(Engine& engine, const qreal speed, const qreal dt) {
        clutch.update(dt);
        if (clutch.acting()) {
            engine.torque_counter = clutch.counter_torque(engine, this, speed, dt);
        } else if (clutch.engage) // clutch is (fully) engaged
            engine.torque_counter = engine.torque_out;
        else // clutch is (completely) disengaged
            engine.torque_counter = 0;
        return engine.torque_counter * gears[gear] * end_transmission / wheel_radius;


//        if (!clutch) {
//            engine.torque_counter = 0;
//            clutch_max_rpm_diff = abs(engine.rpm() - speed2engine_rpm(speed));
//            return 0;
//        }
//        engine.torque_counter = engine.torque_out;
//        if (clutch < 1) {
//            qreal rpm = speed2engine_rpm(speed); // what the engine rpm should be
//            qreal rpm_diff = engine.rpm() - rpm;
//            qreal max_rpm_diff = clutch_max_rpm_diff * (1-clutch); // linear?
//            if (abs(rpm_diff) > max_rpm_diff) {
//                engine.torque_counter = engine.inertia / dt * Engine::rpm2angular_velocity(
//                            rpm_diff > 0 ? rpm_diff-max_rpm_diff : (max_rpm_diff+rpm_diff));
//            }
//        }
//        return engine.torque_counter * gears[gear] * end_transmission / wheel_radius;
    }

    void gear_up() {
        gear = std::min(gear+1, gears.size()-1);
    }
    void gear_down() { gear = std::max(gear -1, 0); }
    //void disengage() { clutch.disengage(); }
    //void engine_idle() { gear = 0; clutch = 0; }

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

signals:
    void gear_changed(int gear);

public:
    int gear;
    QVector<qreal> gears; // gear-transmissions
    QVector<qreal> mass_factors;
    qreal end_transmission;
    qreal rolling_circumference; // cm
    qreal wheel_radius; // m
    Clutch clutch;
    //qreal clutch; // (0..1)
    //qreal clutch_max_rpm_diff = 4000;
};

inline QDataStream &operator<<(QDataStream &out, const Gearbox &g)
{
    out << g.gears << g.mass_factors << g.end_transmission << g.rolling_circumference << g.wheel_radius << g.clutch.t_shift;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Gearbox &g) {
    in >> g.gears >> g.mass_factors >> g.end_transmission >> g.rolling_circumference >> g.wheel_radius >> g.clutch.t_shift;
    g.clutch.t = g.clutch.t_shift;
    return in;
}


inline void Clutch::clutch_in(const Engine& engine, const Gearbox* const gearbox, const double speed) {
    if (!engage) {
        t = 0;
        const qreal rpm = gearbox->speed2engine_rpm(speed); // what the engine rpm should be
        w_t0 = engine.angular_velocity - Engine::rpm2angular_velocity(rpm);
        a_w = -w_t0 / t_shift;
        //printf("should be: %.3f (%.3f) cur: %.3f/%.3f\n", a_w, Engine::angular_velocity2rpm(a_w), engine.angular_velocity, Engine::angular_velocity2rpm(engine.angular_velocity));
        engage = true;
    }
}

inline double Clutch::counter_torque(const Engine& engine, const Gearbox* const gearbox, const double speed, const double dt) {
    Q_ASSERT(acting());
    const qreal w_t = w_t0 + t * a_w; // what the angular_velocity-difference should be
    const qreal rpm = gearbox->speed2engine_rpm(speed); // what the engine rpm would be if clutch was engaged
    const qreal w_t_real = engine.angular_velocity - Engine::rpm2angular_velocity(rpm); // what the angular_velocity-difference *is*
    const qreal a_w_e = (w_t - w_t_real) / dt; // what the acceleration of the engine-angular_velocity must be
    //double const a_w_engine = a_w + (w_t0 ( - t * )
    return engine.torque_out - engine.inertia * a_w_e;
    //engine.angular_velocity += (engine.torque_out - engine.torque_counter) / engine.inertia * dt;
}




#endif // GEARBOX_H
