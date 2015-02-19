#ifndef LOGGING_H
#define LOGGING_H

#include <QList>
#include <QVector>
#include "car.h"
#include "qcarviz.h"
#include "track.h"
#include "misc.h"

#define LOG_VERSION "1.7"
#define LOG_VERSION_JSON "1.0"

struct LogItem
{
    qreal throttle;
    qreal braking;
    int gear;
    QPointF eye_tracker_point;
    qreal steering;
    qreal dt;
};

struct LogItemJson : public LogItem
{
    qreal speed; // kmh
    qreal position; // on the track_path
    qreal rpm; // !!
    qreal acceleration; // !! [m/s^2]
    qreal consumption; // !! [L/s]
    qreal rel_consumption; // !! [L/100km]
    qreal rel_consumption_slow; // [L/100km] from the hud
    void write(QJsonObject& j) {
        j["throttle"] = throttle;
        j["braking"] = braking;
        j["gear"] = gear;
        //j["eye_tracker_point"] = !!
        j["steering"] = steering;
        j["dt"] = dt;
        j["speed"] = speed;
        j["position"] = position;
    }
};

struct LogEvent {
    enum Type {
        Speeding,
        StopSign,
        TrafficLight,
        TooSlow,
    } type;
    int index;
    void write(QJsonObject& j) {
        j["index"] = index;
        const char* stype = nullptr;
        switch (type) {
            case Speeding: stype = "Speeding"; break;
            case StopSign: stype = "StopSign"; break;
            case TrafficLight: stype = "TrafficLight"; break;
            default: Q_ASSERT(false);
        }
        Q_ASSERT(stype != nullptr);
        j["type"] = QString(stype);
    }
};

struct Log
{
    Log(Car* car, QCarViz* car_viz, Track* track) : car(car), car_viz(car_viz), track(track) {}

    void add_item(qreal throttle, qreal braking, int gear, qreal dt) {
        items.append({ throttle, braking, gear, car_viz->get_eye_tracker_point(), car_viz->get_user_steering(), dt });
    }
    void add_event(const LogEvent::Type type) {
        events.push_back({type, items.size()}); // we exepect the events to come in *before* the add_item!
    }
    LogEvent* next_event(int replay_index) {
        if (next_log_event && next_log_event->index == replay_index) {
            LogEvent* const ret = next_log_event;
            next_log_event++;
            if (next_log_event > &events.last())
                next_log_event = nullptr;
            return ret;
        }
        return nullptr;
    }

    bool save(const QString filename) const { return misc::saveObj(filename, *this); }
    bool load(const QString filename) { return misc::loadObj(filename, *this); }
    bool save_json(const QString filename) const { return misc::saveJson(filename, *this); }

    static QString condition_string(Condition cond) {
        switch (cond) {
            case VIS: return "VIS";
            case SLP: return "SLP";
            case CNT: return "CNT";
            case NO_CONDITION: return "NO_CONDITION";
        }
        Q_ASSERT(false);
        return "NO_CONDITION";
    }

    void write(QJsonObject& j) const {
        Q_ASSERT(log_run_finished);
        j["condition"] = sound_modus == 0 ? "VIS" : (sound_modus == 1 ? "SLP" : (sound_modus == 2 ? "CNT" : "UNDEFINED!"));
        j["log_version"] = LOG_VERSION_JSON;
        j["elapsed_time"] = elapsed_time;
        j["liters_used"] = liters_used;
        QJsonArray jitems;
        for (auto i : items_json) {
            QJsonObject ji;
            i.write(ji);
            jitems.append(ji);
        }
        j["items"] = jitems;
    }

    Car* car;
    QCarViz* car_viz;
    Track* track;
    QList<LogItem> items;
    QVector<LogEvent> events;
    QVector<LogItemJson> items_json;
    qreal elapsed_time = 0;
    qreal liters_used = 0;
    int sound_modus = 0;
    Condition condition = VIS; // this should be the same as sound_modus!
    qreal initial_angular_velocity = 0;

    int vp_id = 1001;
    int run = 1;
    int global_run_counter = 0;

    QString version = LOG_VERSION;
    bool valid = true;
    LogEvent* next_log_event = nullptr;
    bool log_run_finished = false;
};


inline QDataStream &operator<<(QDataStream &out, const LogItem &i) {
    out << i.throttle << i.braking << i.gear << i.eye_tracker_point << i.steering << i.dt;
    return out;
}
inline QDataStream &operator>>(QDataStream &in, LogItem &i) {
    in >> i.throttle >> i.braking >> i.gear >> i.eye_tracker_point >> i.steering >> i.dt;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const LogEvent &e) {
    out << e.index << (int) e.type;
    return out;
}
inline QDataStream &operator>>(QDataStream &in, LogEvent &e) {
    in >> e.index >> (int&) e.type;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const Log &log) {
    out << log.version << *log.car << *log.track << log.items << log.events << log.elapsed_time << log.liters_used
        << log.sound_modus << log.initial_angular_velocity << (int) log.condition << log.vp_id << log.run << log.global_run_counter;
    return out;
}
inline QDataStream &operator>>(QDataStream &in, Log &log) {
    int condition;
    in >> log.version >> *log.car >> *log.track >> log.items >> log.events >> log.elapsed_time >> log.liters_used
            >> log.sound_modus >> log.initial_angular_velocity >> condition >> log.vp_id >> log.run >> log.global_run_counter;
    log.condition = (Condition) condition;
    Q_ASSERT(condition == log.sound_modus);
    log.valid = (log.version == QString(LOG_VERSION));
    if (log.events.size() > 0)
        log.next_log_event = &log.events[0];
    log.log_run_finished = false;
    return in;
}

#endif // LOGGING_H
