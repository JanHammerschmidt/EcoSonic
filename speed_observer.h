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

struct TurnSignObserver {
    struct TurnSignExecutor {
        TurnSignExecutor(Track::Sign* sign, const qreal t0, QCarViz& car_viz)
            : info(sign->steering_info), t0(t0), car_viz(car_viz)
        {
            info.left = (sign->type == Track::Sign::TurnLeft);
            t_stages[0] = info.fade_in;
            t_stages[1] = t_stages[0] + info.duration;
            t_stages[2] = t_stages[1] + info.fade_out;
        }
        bool tick(const qreal t, const qreal dt) {
            const qreal tt = t - t0;
            if (tt > t_stages[stage]) {
                stage++;
                if (stage >= 3)
                    return true;
            }
            qreal intensity = info.intensity * dt;
            switch (stage) {
                case 0: intensity *= tt / info.fade_in; break;
                case 1: break;
                case 2: intensity *= 1 - (tt - t_stages[1]) / info.fade_out; break;
                default: Q_ASSERT(false);
            }
            //qDebug() << "stage " << stage << " steer " << intensity / dt;
            car_viz.steer(intensity * (info.left ? -1 : 1));
            return false;
        }
        Track::Sign::SteeringInfo& info;
        const qreal t0;
        QCarViz& car_viz;
        int stage = 0;
        qreal t_stages[3];
    };

    TurnSignObserver(QCarViz& carViz)
        : track(carViz.track), carViz(carViz)
    {
        find_next_sign();
    }

    void tick(const qreal t, const qreal dt) {
        if (!!current_sign.get() && current_sign->tick(t, dt)) {
            current_sign.reset();
        }
        if (!next_sign)
            return;
        const qreal current_pos = carViz.current_pos;
        if (current_pos > next_sign->at_length) {
            trigger(next_sign, t);
            find_next_sign();
        }
    }
    void trigger(Track::Sign* sign, qreal t0) {
        Q_ASSERT(sign->is_turn_sign());
        current_sign.reset(new TurnSignExecutor(sign, t0, carViz));
    }

    void find_next_sign() {
        qDebug() << "find next sign!";
        if (!track.signs.size())
            return;
        if (!next_sign)
            next_sign = &track.signs[0];
        else
            next_sign++;
        for ( ; ; next_sign++) {
            if (next_sign > &track.signs.last()) {
                next_sign = nullptr;
                break;
            }
            if (next_sign->type == Track::Sign::TurnLeft || next_sign->type == Track::Sign::TurnRight)
                break;
        }
    }

    inline void reset() {
        next_sign = nullptr;
        find_next_sign();
    }

    Track::Sign* next_sign = nullptr;
    Track& track;
    QCarViz& carViz;
    std::auto_ptr<TurnSignExecutor> current_sign;
};

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
                    || (s.is_speed_sign() && current_speed_sign && s.type > current_speed_sign->type) // faster speed sign
                    || (s.is_turn_sign()))
                {
                        continue; // skip!
                }
                next_sign = &s;
                break;
            } else if (s.is_speed_sign()) {
                current_speed_sign = &s;
            } else if (!s.is_turn_sign())
                current_hold_sign = &s;
        }
        current_speed_limit = current_speed_sign ? current_speed_sign->speed_limit() : DEFAULT_SPEED_LIMIT;

        if (next_sign && next_sign->type == Track::Sign::TrafficLight && next_sign->traffic_light_state == Track::Sign::Red
                && (next_sign->at_length - current_pos) < next_sign->traffic_light_info.trigger_distance)
        {
            qDebug() << "trigger!";
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
            qDebug() << "stop sign / traffic light!";
            osc.send_float("/flash", 0);
            carViz.flash_timer.start();
            cooldown_timer.start();
            return;
        }
        if (toofast) {
            if (state == too_fast) {
                if (last_state_change.elapsed() > MAX_TOO_FAST) {
                    qDebug() << "too fast!";
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
