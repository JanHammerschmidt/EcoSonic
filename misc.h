#ifndef MISC_H
#define MISC_H

#include <QElapsedTimer>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <JlCompress.h>
#include <QDebug>

namespace misc {

template<class T>
bool saveObj(const QString filename, const T& obj) {
    QFile file(filename);
    QDir().mkpath(QFileInfo(file).absolutePath());
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QDataStream out(&file);
    out << obj;
    return true;
}
template<class T>
bool loadObj(const QString filename, T& obj) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QDataStream in(&file);
    in >> obj;
    return true;
}

template<class T>
bool saveJson(const QString filename, const T& obj, bool compressed = true) {
    Q_ASSERT(!compressed || filename.endsWith(".zip"));
    const QString log_filename = filename.left(filename.length()-4);
    QFile file(log_filename);
    QDir().mkpath(QFileInfo(file).absolutePath());
    if (!file.open(QIODevice::WriteOnly)) {
        Q_ASSERT(false);
        return false;
    }

    QJsonObject json_obj;
    obj.write(json_obj);
    QJsonDocument doc(json_obj);
    file.write(doc.toJson());
    file.close();
    if (compressed) {
        QString save_to = filename; //filename.endsWith(".zip") ? filename : filename + ".zip";
        JlCompress::compressFile(save_to, log_filename);
        file.remove();
    }
    return true;
}


struct FPSTimer {
    FPSTimer(const QString& name = "FPS: ", qint64 interval = 2000)
        : name(name)
        , interval(interval)
        , count(0)
    {
        timer.start();
    }
    void addFrame() {
        count++;
        qint64 elapsed = timer.elapsed();
        if (elapsed > interval) {
            qDebug() << name << double(count*1000) / elapsed;
            //printf("%s%.3f\n", name.c_str(), double(count*1000) / elapsed);
            count = 0;
            timer.restart();
        }
    }

    QElapsedTimer timer;
    int frames = 0;
    QString name;
    qreal interval;
    int count;
};

struct TimeDelta {
    void start() {
        last_time = 0;
        timer.start();
    }
    bool get_time_delta(qreal& dt) {
        qint64 t = timer.elapsed();
        if (t > last_time) {
            elapsed += t-last_time;
            dt = (t-last_time) * 0.001;
            last_time = t;
            return true;
        }
        return false;
    }
    void add_dt(qreal const dt) { elapsed += dt * 1000; }
    qreal get_elapsed() {
        return elapsed * 0.001;
    }

    QElapsedTimer timer;
    qint64 last_time = 0;
    qint64 elapsed = 0;
};

template<class T, class U> T interp(const T& p1, const T& p2, const U f) {
    Q_ASSERT(f >= 0 && f <= 1);
    return (1-f) * p1 + f * p2;
}

inline void draw_centered_text(QPainter& painter, QFontMetrics& fm, QString text, const QPointF pos, const qreal frac = 0.3) {
    const QPointF p = pos + QPointF(-0.5 * fm.width(text), frac * fm.height()); // eigentlich: fm.ascent() ..
//    painter.drawEllipse(pos, 2, 2);
//    painter.drawEllipse(pos - QPointF(0, fm.ascent()), 2, 2);
//    painter.drawEllipse(pos - QPointF(0, fm.ascent()+fm.descent()), 2, 2);
//    painter.drawEllipse(pos - QPointF(0, fm.ascent()+fm.descent()+1), 2, 2);
//    painter.drawEllipse(pos - QPointF(0, fm.height()), 2, 2);
    painter.drawText(p, text);
}

} // namespace misc

#endif // MISC_H
