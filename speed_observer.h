#ifndef SPEED_OBSERVER_H
#define SPEED_OBSERVER_H

#include <QtConcurrent>
#include "qcarviz.h"

//#define NEXT_SIGN_DISTANCE 800 // ??
//#define MIN_DRIVING_DISTANCE 400 // ??
//#define TOO_SLOW_TOLERANCE 0.2
#define TOO_FAST_TOLERANCE 0.1
#define TOO_FAST_TOLERANCE_OFFSET 10
//#define MAX_TOO_FAST 2000 // ms
//#define MAX_TOO_SLOW 3000 // ms
#define COOLDOWN_TIME_SPEEDING 10000 // how long "nothing" happens after a speeding 'flash'
//#define COOLDOWN_TIME 3000 // how long "nothing" happens after a honking (ms)

class SignObserverBase
{
protected:
    SignObserverBase(QCarViz& carViz, const bool execute_when_replaying = false)
        : track(carViz.track), carViz(carViz), execute_when_replaying(execute_when_replaying)
    { }

    void find_next_sign()
    {
        //qDebug() << "find next sign";
        if (!track.signs.size())
            return;
        if (!next_sign)
            next_sign = &track.signs[0];
        else
            next_sign++;
        for ( ; ; next_sign++) {
            if (next_sign > &track.signs.last()) {
                next_sign = nullptr;
                return;
            }
            for (auto t : types) {
                if (next_sign->type == t) {
                    trigger_distance = get_trigger_distance(next_sign);
                    return;
                }
            }
        }
    }
    virtual void trigger(Track::Sign* /*sign*/, qreal const /*t*/) { } // save infos for 'tick_current_sign'
    virtual qreal get_trigger_distance(Track::Sign* /*next_sign*/) { return 0; }
    virtual void init() = 0; // add type(s)
    virtual bool tick_current_sign(const qreal /*t*/, const qreal /*dt*/) { return true; } // return true when finished

public:
    inline void reset() {
        next_sign = nullptr;
        current_sign = nullptr;
        find_next_sign();
    }
    virtual void tick(const bool replay, const qreal t, const qreal dt) {
        if (replay && !execute_when_replaying)
            return;
        if (current_sign && tick_current_sign(t, dt))
            current_sign = nullptr;
        if (!next_sign)
            return;
        if (next_sign->at_length - carViz.current_pos < trigger_distance) {
            //qDebug() << "trigger";
            current_sign = next_sign;
            trigger(current_sign, t);
            find_next_sign();
        }
    }
protected:
    Track::Sign* next_sign = nullptr;
    Track::Sign* current_sign = nullptr;
    std::vector<Track::Sign::Type> types;
    qreal trigger_distance = 0;
    Track& track;
    QCarViz& carViz;
    bool execute_when_replaying = false;
};

template<class T>
class SignObserver : public T
{
public:
    SignObserver(QCarViz& car_viz) : T(car_viz)
    {
        T::init();
        T::find_next_sign();
    }
};

class StopSignObserver : public SignObserverBase
{
protected:
	StopSignObserver(QCarViz& carViz) : SignObserverBase(carViz) {}
    //using SignObserverBase::SignObserverBase;
    void init() override { types.push_back(Track::Sign::Stop); }
    qreal get_trigger_distance(Track::Sign*) override {
        return 70;
    }
    bool tick_current_sign(const qreal, const qreal) override {
        if (carViz.get_car()->speed < 3) { // driving slow enough
            qDebug() << "stop sign: slow enough!";
            return true;
        }
        if (carViz.get_current_pos() - current_sign->at_length > 10) {
            carViz.log_traffic_violation(TrafficViolation::StopSign);
            qDebug() << "StopSign: flash!";
            return true;
        }
        return false;
    }
};

class SpeedObserver : public SignObserverBase
{
protected:
    SpeedObserver(QCarViz& carViz) : SignObserverBase(carViz) {
        cooldown_timer.start();
    }
    void init() override {
        for (auto t = Track::Sign::Speed30; t <= Track::Sign::Speed130; ((int&)t)++)
            types.push_back(t);
    }
    void trigger(Track::Sign* sign, qreal const) override {
        qDebug() << "speed limit:" << sign->speed_limit();
        current_speed_limit = sign->speed_limit() * (1+TOO_FAST_TOLERANCE) + TOO_FAST_TOLERANCE_OFFSET;
    }
    qreal get_trigger_distance(Track::Sign* next_sign) override {
        if (current_sign && next_sign->speed_limit() > current_sign->speed_limit())
            return 30;
        else
            return -30;
    }
    bool tick_current_sign(const qreal, const qreal) override {
        if (carViz.get_kmh() > current_speed_limit && cooldown_timer.elapsed() > COOLDOWN_TIME_SPEEDING) {
            qDebug() << "Speed: Flash!";
            carViz.log_traffic_violation(TrafficViolation::Speeding);
            cooldown_timer.start();
        }
        return false;
    }
    qreal current_speed_limit = 0;
    QElapsedTimer cooldown_timer;
};

class TrafficLightObserver : public SignObserverBase
{
protected:
    TrafficLightObserver(QCarViz& carViz) : SignObserverBase(carViz, true) { }
    void init() override { types.push_back(Track::Sign::TrafficLight); }
    static void trigger_traffic_light(Track::Sign* traffic_light) {
        std::pair<qreal,qreal>& time_range = traffic_light->traffic_light_info.time_range;
        std::uniform_int_distribution<int> time(time_range.first,time_range.second);
        QThread::msleep(time(rng));
        traffic_light->traffic_light_state = Track::Sign::Yellow;
        QThread::msleep(1000);
        traffic_light->traffic_light_state = Track::Sign::Green;
    }
    bool tick_current_sign(const qreal, const qreal) override {
        if (current_sign->traffic_light_state != Track::Sign::Red_pending) {
            qDebug() << "TrafficLight: okay";
            return true;
        }
        if (carViz.get_current_pos() - current_sign->at_length > 10) {
            qDebug() << "TrafficLight: flash!";
            carViz.log_traffic_violation(TrafficViolation::TrafficLight);
            return true;
        }
        return false;
    }
    qreal get_trigger_distance(Track::Sign *sign) {
        return sign->traffic_light_info.trigger_distance;
    }
    void trigger(Track::Sign * sign, const qreal) {
        Q_ASSERT(sign->traffic_light_state == Track::Sign::Red);
        sign->traffic_light_state = Track::Sign::Red_pending;
        QtConcurrent::run(trigger_traffic_light, next_sign);
    }
};

class TurnSignObserver : public SignObserverBase
{
protected:
    TurnSignObserver(QCarViz& car_viz) : SignObserverBase(car_viz, true) { }
    void init() override {
        types.push_back(Track::Sign::TurnLeft);
        types.push_back(Track::Sign::TurnRight);
    }
    bool tick_current_sign(const qreal t, const qreal dt) {
        const qreal tt = t - t0;
        if (tt > t_stages[stage]) {
            stage++;
            if (stage >= 3)
                return true;
        }
        qreal intensity = info->intensity * dt;
        switch (stage) {
            case 0: intensity *= tt / info->fade_in; break;
            case 1: break;
            case 2: intensity *= 1 - (tt - t_stages[1]) / info->fade_out; break;
            default: Q_ASSERT(false);
        }
        //qDebug() << "stage " << stage << " steer " << intensity / dt;
        carViz.steer(intensity * (info->left ? -1 : 1));
        return false;
    }
public:
    void trigger(Track::Sign *sign, qreal t) override {
        current_sign = next_sign;
        info = &sign->steering_info;
        t0 = t;
        stage = 0;
        info->left = (sign->type == Track::Sign::TurnLeft);
        t_stages[0] = info->fade_in;
        t_stages[1] = t_stages[0] + info->duration;
        t_stages[2] = t_stages[1] + info->fade_out;
    }
protected:
    Track::Sign::SteeringInfo* info = nullptr;
    qreal t0 = 0;
    int stage = 0;
    qreal t_stages[3];
};

//struct TooSlowObserver {
//    TooSlowObserver(QCarViz& car_viz) : car_viz(car_viz), track(car_viz.track) { }
//    void reset() {
//        const QVector<Track::Sign>& signs = track.signs;

//    }

//    QCarViz& car_viz;
//    Track& track;
//};

//struct SpeedObserver {
//    SpeedObserver(QCarViz& carViz, OSCSender& osc)
//        : track(carViz.track), carViz(carViz), osc(osc)
//    {
//        last_state_change.start();
//        cooldown_timer.start();
//    }
//    enum State {
//        OK,
//        too_slow,
//        too_fast,
//    };
////    static void trigger_traffic_light(Track::Sign* traffic_light) {
////        std::pair<qreal,qreal>& time_range = traffic_light->traffic_light_info.time_range;
////        std::uniform_int_distribution<int> time(time_range.first,time_range.second);
////        QThread::msleep(time(rng));
////        traffic_light->traffic_light_state = Track::Sign::Yellow;
////        QThread::msleep(1000);
////        traffic_light->traffic_light_state = Track::Sign::Green;
////    }
//    void tick() {
//        const qreal current_pos = carViz.get_current_pos();
//        Track::Sign* current_speed_sign = NULL;
//        Track::Sign* current_hold_sign = NULL;
//        Track::Sign* next_sign = NULL;
//        for (Track::Sign& s : track.signs) {
//            if (current_pos < s.at_length) {
//                if (s.at_length - current_pos > NEXT_SIGN_DISTANCE)
//                    break;
//                if ((s.type == Track::Sign::TrafficLight && s.traffic_light_state == Track::Sign::Green) // green traffic lights
//                    || (s.is_speed_sign() && current_speed_sign && s.type > current_speed_sign->type) // faster speed sign
//                    || (s.is_turn_sign()))
//                {
//                        continue; // skip!
//                }
//                next_sign = &s;
//                break;
//            } else if (s.is_speed_sign()) {
//                current_speed_sign = &s;
//            } else if (!s.is_turn_sign())
//                current_hold_sign = &s;
//        }
//        current_speed_limit = current_speed_sign ? current_speed_sign->speed_limit() : DEFAULT_SPEED_LIMIT;

//        if (next_sign && next_sign->type == Track::Sign::TrafficLight && next_sign->traffic_light_state == Track::Sign::Red
//                && (next_sign->at_length - current_pos) < next_sign->traffic_light_info.trigger_distance)
//        {
//            qDebug() << "trigger traffic light";
//            next_sign->traffic_light_state = Track::Sign::Red_pending;
//            //QtConcurrent::run(trigger_traffic_light, next_sign);
//        }
//        const qreal kmh = carViz.get_kmh(); //Gearbox::speed2kmh(carViz.car->speed);
//        const bool toofast = kmh > current_speed_limit * (1+TOO_FAST_TOLERANCE) + TOO_FAST_TOLERANCE_OFFSET;
//        bool tooslow = kmh < current_speed_limit * (1-TOO_SLOW_TOLERANCE);
//        if (tooslow) {
//            if (carViz.get_current_pos() < MIN_DRIVING_DISTANCE)
//                tooslow = false;
//            if (next_sign && (next_sign->type == Track::Sign::TrafficLight || next_sign->type == Track::Sign::Stop
//                || (next_sign->is_speed_sign() && kmh < next_sign->speed_limit() * (1-TOO_SLOW_TOLERANCE))))
//            {
//                    tooslow = false;
//            }
//        }
//        if (cooldown_timer.elapsed() < COOLDOWN_TIME)
//            return;
//        if (current_hold_sign
//                && !(current_hold_sign->type == Track::Sign::TrafficLight && current_hold_sign->traffic_light_state != Track::Sign::Red)
//                && current_pos - current_hold_sign->at_length > 10)
//        {
//            qDebug() << "stop sign / traffic light!";
//            osc.send_float("/flash", 0);
//            carViz.flash_timer.start();
//            cooldown_timer.start();
//            return;
//        }
//        if (toofast) {
//            if (state == too_fast) {
//                if (last_state_change.elapsed() > MAX_TOO_FAST) {
//                    qDebug() << "too fast!";
//                    osc.send_float("/flash", 0);
//                    carViz.flash_timer.start();
//                    cooldown_timer.start();
//                    state = OK;
//                }
//            } else {
//                state = too_fast;
//                last_state_change.start();
//            }
//        } else if (tooslow) {
////            if (state == too_slow) {
////                if (last_state_change.elapsed() > MAX_TOO_SLOW) {
////                    osc.send_float("/honk", 0);
////                    cooldown_timer.start();
////                    state = OK;
////                }
////            } else {
////                state = too_slow;
////                last_state_change.start();
////            }
//        }
//    }
//    State state = OK;
//    QElapsedTimer last_state_change;
//    QElapsedTimer cooldown_timer;
//    qreal current_speed_limit = DEFAULT_SPEED_LIMIT;
//    Track& track;
//    QCarViz& carViz;
//    OSCSender& osc;
//};

#endif // SPEED_OBSERVER_H
