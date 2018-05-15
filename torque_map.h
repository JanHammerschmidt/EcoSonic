#ifndef TORQUE_MAP_H
#define TORQUE_MAP_H

#define ARR_SIZE(x) (sizeof(x)/sizeof(x[0]))
static qreal throttle10[][2] = { {0.0, 0.2}, {0.2,0.8}, {0.5,1}, {0.8,0.8}, {1.0,0} };
static qreal throttle5[][2] = { {0.0, 0.2}, {0.2,0.65}, {0.5,0.55}, {0.7,0.4}, {1.0,0} };

// TorqueMap translates rpm and throttle into torque
// both rpm and throttle are relative! (range: 0..1)
struct TorqueMap {
    TorqueMap() {
        // load default ramps
        torque_ramps.append(TorqueRamp(.5, throttle5, ARR_SIZE(throttle5)));
        torque_ramps.append(TorqueRamp(1, throttle10, ARR_SIZE(throttle10)));
        //TorqueRamp tr(throttle5, )
    }
    // maps (relative) rpm to (relative) torque for a given throttle
    struct TorqueRamp {
        TorqueRamp() = default;
        TorqueRamp(qreal throttle) : throttle(throttle) { }
        TorqueRamp(qreal throttle, qreal values[][2], ulong const n) : throttle(throttle) {
            for (ulong i = 0; i < n; i++) {
                rpm2torque.append(QPointF(values[i][0], values[i][1]));
            }
        }

        // simple linear interpolation
        // this could be replaced e.g. by something "spliny" ..
        qreal get_torque(qreal rpm) {
            //Q_ASSERT(rpm >= 0 && rpm <= 1);
            if (rpm <= rpm2torque[0].x())
                return rpm2torque[0].y();
            if (rpm >= rpm2torque.last().x())
                return rpm2torque.last().y();
            //qDebug("rpm: %.3f\n", rpm);
            for (int i = 1; i < rpm2torque.size(); i++) {
                //qDebug("%.3f", rpm2torque[i].x());
                if (rpm <= rpm2torque[i].x()) {
                    return linear_interp(rpm, rpm2torque[i-1], rpm2torque[i]);
                }
            }
            Q_ASSERT(false);
            return 0;
        }

        qreal throttle;
        // x=rpm, y=torque. must be sorted by rpm
        QVector<QPointF> rpm2torque;
    };

    // throttle (0..1), rpm (0..1)
    inline qreal get_torque(qreal const throttle, qreal const rpm) {
        Q_ASSERT(throttle >= 0); // && rpm <= 1);
        if (throttle <= torque_ramps[0].throttle)
            return linear_interp(throttle, 0, torque_ramps[0].throttle, 0, torque_ramps[0].get_torque(rpm));
        if (throttle >= torque_ramps.last().throttle)
            return torque_ramps.last().get_torque(rpm);
        for (int i = 1; i < torque_ramps.size(); i++)
            if (throttle <= torque_ramps[i].throttle) {
                return linear_interp(throttle, torque_ramps[i-1].throttle, torque_ramps[i].throttle
                                             , torque_ramps[i-1].get_torque(rpm), torque_ramps[i].get_torque(rpm));
            }
        Q_ASSERT(false);
        return 0;
    }

    static qreal linear_interp(qreal x, QPointF& p0, QPointF& p1) {
        return linear_interp(x, p0.x(), p1.x(), p0.y(), p1.y());
    }
    static qreal linear_interp(qreal x, qreal x0, qreal x1, qreal y0, qreal y1) {
        Q_ASSERT(x1 > x0);
        return (y0 * (x1-x) + y1 * (x-x0)) / (x1 - x0);
    }
//        static qreal cosine_interp(qreal x, QPointF& p0, QPointF& p1) {
//            return cosine_interp(x, p0.x(), p1.x(), p0.y(), p1.y());
//        }
//        static qreal cosine_interp(qreal x, qreal x0, qreal x1, qreal y0, qreal y1) {
//            Q_ASSERT(x1 > x0);
//            qreal const mu = (x-x0) / (x1 - x0);
//            qreal const mu2 = (1-cos(mu*M_PI))/2;
//            return y0 * (1-mu2) + y1 * mu2;
//        }

    // TorqueMaps: must be sorted (by throttle-value)!
    QList<TorqueRamp> torque_ramps;

};

inline QDataStream &operator<<(QDataStream &out, const TorqueMap::TorqueRamp &tr)
{
    out << tr.rpm2torque << tr.throttle;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, TorqueMap::TorqueRamp &tr)
{
    in >> tr.rpm2torque >> tr.throttle;
    return in;
}

#endif // TORQUE_MAP_H
