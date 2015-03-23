#ifndef SPEED_OBSERVER_H
#define SPEED_OBSERVER_H

#include <QtConcurrent>
#include "qcarviz.h"
#include "logging.h"

// TODO: put these config values into the log!
#define TOO_FAST_TOLERANCE 0.1 // PERCENT OF CURRENT SPEED
#define TOO_FAST_TOLERANCE_OFFSET 10 // KMH OFFSET
#define TOO_SLOW_TOLERANCE 0.1 // percent of current speed
#define TOO_SLOW_TOLERANCE_OFFSET 10 // kmh offset
#define COOLDOWN_TIME_SPEEDING 10000 // how long "nothing" happens after a speeding 'flash'
#define TOOSLOW_OBSERVER_ACCELERATION 0.1 // kmh per "track_path unit"
#define TOOSLOW_OBSERVER_SLOWING_DOWN 0.05
#define TOOSLOW_OBSERVER_COOLDOWN 10000 //how long nothing happens after getting "honked"

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
    virtual void reset() {
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
    void reset() override {
        SignObserverBase::reset();

    }
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
        if (carViz.get_current_pos() - current_sign->at_length > 0) {
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
        if (carViz.is_log_run())
            return;
        Q_ASSERT(sign->traffic_light_state == Track::Sign::Red);
        sign->traffic_light_state = Track::Sign::Red_pending;
        QtConcurrent::run(trigger_traffic_light, sign);
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
        qreal const steering = intensity * (info->left ? -1 : 1);
        carViz.steer(steering);
        carViz.set_scripted_steering(steering);
        return false;
    }
public:
    void trigger(Track::Sign *sign, qreal t) override {
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

struct TooSlowObserver {
    TooSlowObserver(QCarViz& car_viz, OSCSender& osc) : car_viz(car_viz), track(car_viz.track), osc_(osc) {
        cooldown_timer_.start();
    }
    void reset() {
        min_speed_.clear();
        const qreal track_length = car_viz.get_track_path().length() * track_mult;
        min_speed_.resize((int) track_length + 5);
        min_speed_.fill(0);
        QVector<Track::Sign> signs = track.signs;
        signs.insert(signs.begin(), Track::Sign(Track::Sign::Speed130, 10));

        // get all speed signs
        QVector<const Track::Sign*> speed_signs;
        for (const Track::Sign& s : signs) {
            if (s.is_speed_sign())
                speed_signs.push_back(&s);
        }
        if (!speed_signs.size())
            return;

        // set speed based on speed signs
        for (int i = 0; i < speed_signs.size(); i++) {
            const Track::Sign* const cur = speed_signs[i];
            const Track::Sign* const prev = !i ? nullptr : speed_signs[i-1];
            const Track::Sign* const next = i+1 >= speed_signs.size() ? nullptr : speed_signs[i+1];
            const int start = (int) ceil(speed_signs[i]->at_length * track_mult);
            const int end = (int) floor(next ? next->at_length * track_mult : track_length);
            const qreal limit = honk_limit(cur->speed_limit());
            const qreal prev_limit = prev ? honk_limit(prev->speed_limit()) : 200;
            const qreal next_limit = next ? honk_limit(next->speed_limit()) : limit;

            for (int j = start; j <= end; j++)
                set_min_speed(j, limit);

            if (prev_limit < limit) {
                qreal tlimit = prev_limit;
                const qreal inc = TOOSLOW_OBSERVER_ACCELERATION / track_mult;
                for (int j = start; j <= end; j++) {
                    tlimit += inc;
                    set_min_speed(j, tlimit);
                    if (tlimit > limit)
                        break;
                }
            }
            if (next_limit < limit) {
                qreal tlimit = next_limit;
                const qreal inc = TOOSLOW_OBSERVER_SLOWING_DOWN / track_mult;
                for (int j = end-1; j >= start; j--) {
                    tlimit += inc;
                    set_min_speed(j, tlimit);
                    if (tlimit > limit)
                        break;
                }
            }
        }
        signs.remove(0);
        for (int j = 50 * track_mult; j >= 0; j--)
            set_min_speed(j, -1);
        insert_stop_sign(40, track_length);

        // get all stop signs & traffic lights
        QVector<const Track::Sign*> stop_signs;
        for (const Track::Sign& s : signs) {
            if (s.type == Track::Sign::Stop || s.type == Track::Sign::TrafficLight)
                stop_signs.push_back(&s);
        }
        if (!stop_signs.size())
            return;

        for (const Track::Sign* const s : stop_signs) {
            const int pos = (int) s->at_length;
            insert_stop_sign(pos, track_length);
        }

        struct speed {
            speed(QVector<qreal>& min_speed) : min_speed_(min_speed) {}
            void write(QJsonObject& j) const {
                QJsonArray jitems;
                for (auto i : min_speed_) {
                    jitems.append(i);
                }
                j["speed"] = jitems;
            }
            const QVector<qreal>& min_speed_;
        };

        speed s(min_speed_);
        misc::saveJson("/Users/jhammers/test.json.zip", s, true);
    }
    void insert_stop_sign(const int pos, const int track_length) {
        const int left = boost::algorithm::clamp((pos - 100) * track_mult, 0, track_length);
        const int right = boost::algorithm::clamp((pos + 25) * track_mult, 0, track_length);
        for (int j = left+1; j < right; j++)
            set_min_speed(j, -1);
        qreal inc = TOOSLOW_OBSERVER_SLOWING_DOWN / track_mult;
        qreal tlimit = 0;
        for (int j = left; j >= 0; j--) {
            tlimit += inc;
            set_min_speed(j, tlimit);
        }
        inc = TOOSLOW_OBSERVER_ACCELERATION / track_mult;
        tlimit = 0;
        for (int j = right; j <= track_length; j++) {
            tlimit += inc;
            set_min_speed(j, tlimit);
        }
    }

    inline void set_min_speed(const int pos, const qreal speed) {
        Q_ASSERT(pos >= 0 && pos < min_speed_.size());
        qreal& val = min_speed_[pos];
        if (!val || speed < val)
            val = speed;
    }

    inline qreal min_speed(qreal pos) const {
        pos *= track_mult;
        const int p1 = boost::algorithm::clamp((int) floor(pos), 0, min_speed_.size() - 1);
        const int p2 = boost::algorithm::clamp((int) ceil(pos), 0, min_speed_.size() - 1);
        //Q_ASSERT(p1 >= 0 && p2 >= 0);
        const qreal v = pos - floor(pos);
        return v * min_speed_[p2] + (1-v) * min_speed_[p1];
    }
    static inline qreal honk_limit(qreal l) {
        return l * (1-TOO_SLOW_TOLERANCE) - TOO_SLOW_TOLERANCE_OFFSET;
    }

    void tick() {
        const qreal pos = car_viz.get_current_pos();
        const qreal slow_speed_threshold = min_speed(pos);
        const qreal current_speed = car_viz.get_kmh();
        if (current_speed < slow_speed_threshold && cooldown_timer_.elapsed() > TOOSLOW_OBSERVER_COOLDOWN) {
            qDebug() << "Too Slow!";
            if (!car_viz.is_replaying()) {
                car_viz.log()->add_event(LogEvent::TooSlow);
                car_viz.show_too_slow();
            }
            cooldown_timer_.start();
        }
    }

    const qreal track_mult = 0.5; // check min_speed()! (current_pos * track_mult) is the position in the min_speed_ array
                                  // the bigger track_mult is, the more precise (and slow) the calculation is
    QVector<qreal> min_speed_;
    QCarViz& car_viz;
    Track& track;
    OSCSender& osc_;
    QElapsedTimer cooldown_timer_;
};

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
