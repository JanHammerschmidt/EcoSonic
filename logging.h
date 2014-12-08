#ifndef LOGGING_H
#define LOGGING_H

#include <QList>
#include "car.h"

struct LogItem
{
    qreal throttle;
    qreal braking;
    int gear;
    qreal dt;
};

struct Log
{
    Log(Car* car) : car(car) {}

    void add_item(qreal throttle, qreal braking, int gear, qreal dt) {
        //LogItem i = { throttle, braking, gear, dt };
        items.append({ throttle, braking, gear, dt });
    }

    Car* car;
    QList<LogItem> items;
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
    out << *log.car << log.items;
    return out;
}
//inline QDataStream &operator>>(QDataStream &in, Log &log) {
//    in >> *log.car >> log.items;
//    return in;
//}

#endif // LOGGING_H
