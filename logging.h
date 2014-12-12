#ifndef LOGGING_H
#define LOGGING_H

#include <QList>
#include "car.h"
#include "track.h"
#include "misc.h"

struct LogItem
{
    qreal throttle;
    qreal braking;
    int gear;
    qreal dt;
};

struct Log
{
    Log(Car* car, Track* track) : car(car), track(track) {}

    void add_item(qreal throttle, qreal braking, int gear, qreal dt) {
        //LogItem i = { throttle, braking, gear, dt };
        items.append({ throttle, braking, gear, dt });
    }

    bool save(const QString filename) const { return saveObj(filename, *this); }
    //bool load(const QString filename) { return loadObj(filename, *this); }

    Car* car;
    Track* track;
    QList<LogItem> items;
    qreal elapsed_time = 0;
    qreal liters_used = 0;
};


inline QDataStream &operator<<(QDataStream &out, const LogItem &i) {
    out << i.throttle << i.braking << i.gear << i.dt;
    return out;
}
inline QDataStream &operator>>(QDataStream &in, LogItem &i) {
    in >> i.throttle >> i.braking >> i.gear >> i.dt;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const Log &log) {
    out << *log.car << *log.track << log.items << log.elapsed_time << log.liters_used;
    return out;
}
inline QDataStream &operator>>(QDataStream &in, Log &log) {
    in >> *log.car >> *log.track >> log.items >> log.elapsed_time >> log.liters_used;
    return in;
}

#endif // LOGGING_H
