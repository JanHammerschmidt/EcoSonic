#ifndef CAR_H
#define CAR_H

#include "engine.h"
#include "gearbox.h"
#include "resistances.h"
#include <memory>

struct Log;

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

    void save_log(QDateTime& program_start_time);

    // throttle: (0..1), alpha: up/downhill [rad]
    // returns acceleration [m/s^2]
    qreal tick(qreal const dt, qreal alpha, bool replay);

    qreal mass; // [kg]
    qreal speed = 0; // [m/s]
    qreal throttle = 0; // (0..1)
    qreal braking = 0; // (0..1)
    qreal max_breaking_force = 20000;
    qreal drag_resistance_coefficient;
    qreal rolling_resistance_coefficient;

    qreal current_accumulated_resistance;
    qreal current_wheel_force;
    qreal current_acceleration;

    Engine engine;
    Gearbox gearbox;

    std::shared_ptr<Log> log;
};

inline QDataStream &operator<<(QDataStream &out, const Car &car)
{
    out << car.mass << car.max_breaking_force << car.drag_resistance_coefficient << car.rolling_resistance_coefficient << car.engine << car.gearbox;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Car &car) {
    in >> car.mass >> car.max_breaking_force >> car.drag_resistance_coefficient >> car.rolling_resistance_coefficient >> car.engine >> car.gearbox;
    return in;
}



#endif // CAR_H
