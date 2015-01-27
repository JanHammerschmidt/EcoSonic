#ifndef LOGGING_H
#define LOGGING_H

#include <QList>
#include "car.h"
#include "qcarviz.h"
#include "track.h"
#include "misc.h"

#define LOG_VERSION "1.3"

struct LogItem
{
    qreal throttle;
    qreal braking;
    int gear;
    QPointF eye_tracker_point;
    qreal steering;
    qreal dt;
};

struct Log
{
    Log(Car* car, QCarViz* car_viz, Track* track) : car(car), car_viz(car_viz), track(track) {}

    void add_item(qreal throttle, qreal braking, int gear, qreal dt) {
        //LogItem i = { throttle, braking, gear, dt };
        items.append({ throttle, braking, gear, car_viz->eye_tracker_point, car_viz->steering, dt });
    }

    bool save(const QString filename) const { return saveObj(filename, *this); }
    bool load(const QString filename) { return loadObj(filename, *this); }

    Car* car;
    QCarViz* car_viz;
    Track* track;
    QList<LogItem> items;
    qreal elapsed_time = 0;
    qreal liters_used = 0;
    int sound_modus = 0;
    qreal initial_angular_velocity = 0;
    QString version = LOG_VERSION;
    bool valid = true;
};


inline QDataStream &operator<<(QDataStream &out, const LogItem &i) {
    out << i.throttle << i.braking << i.gear << i.eye_tracker_point << i.dt;
    return out;
}
inline QDataStream &operator>>(QDataStream &in, LogItem &i) {
    in >> i.throttle >> i.braking >> i.gear >> i.eye_tracker_point >> i.dt;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const Log &log) {
    out << log.version << *log.car << *log.track << log.items << log.elapsed_time << log.liters_used << log.sound_modus << log.initial_angular_velocity;
    return out;
}
inline QDataStream &operator>>(QDataStream &in, Log &log) {
    in >> log.version >> *log.car >> *log.track >> log.items >> log.elapsed_time >> log.liters_used >> log.sound_modus >> log.initial_angular_velocity;
    log.valid = (log.version == QString(LOG_VERSION));
    return in;
}

#endif // LOGGING_H
