#ifndef SPEED_OBSERVER_H
#define SPEED_OBSERVER_H

#include <QtConcurrent>
#include "qcarviz.h"

#define NEXT_SIGN_DISTANCE 800 // ??
#define MIN_DRIVING_DISTANCE 400 // ??
#define TOO_SLOW_TOLERANCE 0.2
#define TOO_FAST_TOLERANCE 0.1
#define MAX_TOO_FAST 2000 // ms
#define MAX_TOO_SLOW 3000 // ms
#define COOLDOWN_TIME 3000 // how long "nothing" happens after a honking (ms)

struct SpeedObserver {
    SpeedObserver(QCarViz& carViz, OSCSender& osc)
        : track(carViz.track), carViz(carViz), osc(osc)
    {
        last_state_change.start();
        cooldown_timer.start();
    }
    enum State {
        OK,
        too_slow,
        too_fast,
    };
    static void trigger_traffic_light(Track::Sign* traffic_light) {
        std::pair<qreal,qreal>& time_range = traffic_light->traffic_light_info.time_range;
        std::uniform_int_distribution<int> time(time_range.first,time_range.second);
        QThread::msleep(time(rng));
        traffic_light->traffic_light_state = Track::Sign::Yellow;
        QThread::msleep(1000);
        traffic_light->traffic_light_state = Track::Sign::Green;
    }
    void tick() {
        const qreal current_pos = carViz.current_pos;
        Track::Sign* current_speed_sign = NULL;
        Track::Sign* current_hold_sign = NULL;
        Track::Sign* next_sign = NULL;
        for (Track::Sign& s : track.signs) {
            if (current_pos < s.at_length) {
                if (s.at_length - current_pos > NEXT_SIGN_DISTANCE)
                    break;
                if ((s.type == Track::Sign::TrafficLight && s.traffic_light_state == Track::Sign::Green) // green traffic lights
                    || (s.is_speed_sign() && current_speed_sign && s.type > current_speed_sign->type)) // faster speed sign
                {
                        continue; // skip!
                }
                next_sign = &s;
                break;
            } else if (s.is_speed_sign()) {
                current_speed_sign = &s;
            } else
                current_hold_sign = &s;
        }
        current_speed_limit = current_speed_sign ? current_speed_sign->speed_limit() : DEFAULT_SPEED_LIMIT;

        if (next_sign && next_sign->type == Track::Sign::TrafficLight && next_sign->traffic_light_state == Track::Sign::Red
                && (next_sign->at_length - current_pos) < next_sign->traffic_light_info.trigger_distance)
        {
            printf("trigger!\n");
            next_sign->traffic_light_state = Track::Sign::Red_pending;
            QtConcurrent::run(trigger_traffic_light, next_sign);
        }
        const qreal kmh = Gearbox::speed2kmh(carViz.car->speed);
        const bool toofast = kmh > current_speed_limit * (1+TOO_FAST_TOLERANCE);
        bool tooslow = kmh < current_speed_limit * (1-TOO_SLOW_TOLERANCE);
        if (tooslow) {
            if (carViz.current_pos < MIN_DRIVING_DISTANCE)
                tooslow = false;
            if (next_sign && (next_sign->type == Track::Sign::TrafficLight || next_sign->type == Track::Sign::Stop
                || (next_sign->is_speed_sign() && kmh < next_sign->speed_limit() * (1-TOO_SLOW_TOLERANCE))))
            {
                    tooslow = false;
            }
        }
        if (cooldown_timer.elapsed() < COOLDOWN_TIME)
            return;
        if (current_hold_sign
                && !(current_hold_sign->type == Track::Sign::TrafficLight && current_hold_sign->traffic_light_state != Track::Sign::Red)
                && current_pos - current_hold_sign->at_length > 10)
        {
            osc.send_float("/flash", 0);
            carViz.flash_timer.start();
            cooldown_timer.start();
            return;
        }
        if (toofast) {
            if (state == too_fast) {
                if (last_state_change.elapsed() > MAX_TOO_FAST) {
                    osc.send_float("/flash", 0);
                    carViz.flash_timer.start();
                    cooldown_timer.start();
                    state = OK;
                }
            } else {
                state = too_fast;
                last_state_change.start();
            }
        } else if (tooslow) {
//            if (state == too_slow) {
//                if (last_state_change.elapsed() > MAX_TOO_SLOW) {
//                    osc.send_float("/honk", 0);
//                    cooldown_timer.start();
//                    state = OK;
//                }
//            } else {
//                state = too_slow;
//                last_state_change.start();
//            }
        }
    }
    State state = OK;
    QElapsedTimer last_state_change;
    QElapsedTimer cooldown_timer;
    qreal current_speed_limit = DEFAULT_SPEED_LIMIT;
    Track& track;
    QCarViz& carViz;
    OSCSender& osc;
};

#endif // SPEED_OBSERVER_H
