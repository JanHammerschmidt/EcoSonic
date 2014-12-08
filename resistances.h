#ifndef RESISTANCES_H
#define RESISTANCES_H

#define GRAVITY 9.81 //[m/(s^2)]

namespace resistances {
    // stirnfläche [m^2], luftwiderstand [kg/m^3] (1.2250 bei 15°)
    // TODO: das ganze * 0.5 ?
    static qreal drag_resistance_coefficient(qreal drag_coefficient, qreal stirnflaeche, qreal air_density = 1.2250) {
        return drag_coefficient * stirnflaeche * air_density;
    }
    // returns [N], speed in [m/s]
    static inline qreal drag(qreal drag_resistance_coefficient, qreal speed) {
        return drag_resistance_coefficient * speed*speed;
    }

    // alpha: steigung [rad], mass [kg]
    static inline qreal rolling(qreal rolling_resistance_coefficient, qreal alpha, qreal mass) {
        return rolling_resistance_coefficient * cos(alpha) * mass * GRAVITY;
    }

    // mass [kg], alpha: steigung [rad] (kann negativ sein!)
    static inline qreal uphill(qreal mass, qreal alpha) {
        return mass * GRAVITY * sin(alpha);
    }
}

#endif // RESISTANCES_H
