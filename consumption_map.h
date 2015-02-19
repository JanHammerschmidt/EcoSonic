#ifndef CONSUMPTION_MAP_H
#define CONSUMPTION_MAP_H

#include <math.h>
#include <QtGlobal>
#include <QPointF>
#include <QTransform>

template<class T> inline T sqr(const T x) { return x*x; }

// ConsumptionMap translates (relative) torque & rpm to (relative) consumption (100%- (e.g.) 150%) [1-1.5]
// it is currently implemented as an ellipse at a specific point in the torque/rpm diagram
struct ConsumptionMap {
public:
    ConsumptionMap()
        : ellipse(0.4, 0.16)
    {
        transform.rotate(-20);
        transform.translate(-0.45, -0.85);
//            qreal x2c[][2] = { {0.0, 1.0}, {0.5, 1.03}, {1.0, 1.}, {1.5, 1.1}, {2.0, 1.15}, {2.5,1.25}, {3.0, 1.75}, {4.0, 3.0} };
//            for (ulong i = 0; i < ARR_SIZE(x2c); i++) {
//                d2consumption.append(QPointF(x2c[i][0], x2c[i][1]));
//            }
    }

    // both torque & rpm must be relative! (0-1)
    qreal get_rel_consumption(qreal const rpm, qreal const torque) const {
        const QPointF x = transform.map(QPointF(rpm, torque));
        const qreal e = sqr(x.x() / ellipse.x()) + sqr(x.y() / ellipse.y()); // ellipse function
        //return log(e + 1) * 0.2 + 1; // mapping
        return pow(e, 0.7) * 0.1 + 1;
    }

//protected:
    //QPointF center;
    QPointF ellipse; // a/b parameter
    QTransform transform; // takes care of rotation & translation of the ellipse
//        QVector<QPointF> d2consumption; // maps the iso-lines of the ellipse to relative consumption, must be sorted by x
};

#endif // CONSUMPTION_MAP_H
