#include "engine.h"
#include "gearbox.h"
#include "car.h"

Engine::Engine(qreal const max_rpm, qreal const max_torque)
    : max_rpm(max_rpm)
    , max_torque(max_torque)
    , engine_braking_offset(0)
    , torque(0)
{
    reset();
}


void Gearbox::auto_clutch_control(Car* car) {
    if (!clutch.engage && car->engine.rpm() > 1000)
        clutch.clutch_in(car->engine, &car->gearbox, car->speed);
}
