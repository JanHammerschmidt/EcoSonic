#ifndef LOGGING_H
#define LOGGING_H

#include <QList>
#include <QVector>
#include "car.h"
#include "qcarviz.h"
#include "track.h"
#include "misc.h"

#define LOG_VERSION "1.6"

struct LogItem
{
    qreal throttle;
    qreal braking;
    int gear;
    QPointF eye_tracker_point;
    qreal steering;
    qreal dt;
};

struct LogEvent {
    enum Type {
        Speeding,
        StopSign,
        TrafficLight,
    } type;
    int index;
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

    Car* car;
    QCarViz* car_viz;
    Track* track;
    QList<LogItem> items;
    QVector<LogEvent> events;
    qreal elapsed_time = 0;
    qreal liters_used = 0;
    int sound_modus = 0;
    qreal initial_angular_velocity = 0;
    QString version = LOG_VERSION;
    bool valid = true;
    LogEvent* next_log_event = nullptr;
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
    out << log.version << *log.car << *log.track << log.items << log.events << log.elapsed_time << log.liters_used << log.sound_modus << log.initial_angular_velocity;
    return out;
}
inline QDataStream &operator>>(QDataStream &in, Log &log) {
    in >> log.version >> *log.car >> *log.track >> log.items >> log.events >> log.elapsed_time >> log.liters_used >> log.sound_modus >> log.initial_angular_velocity;
    log.valid = (log.version == QString(LOG_VERSION));
    if (log.events.size() > 0)
        log.next_log_event = &log.events[0];
    return in;
}

#endif // LOGGING_H
