#include "engine.h"

Engine::Engine(qreal const max_rpm, qreal const max_torque)
    : base_consumption(206)
    , max_rpm(max_rpm)
    , max_torque(max_torque)
    , engine_braking_offset(0)
    , torque(0)
{
    set_rpm(2500);
}
