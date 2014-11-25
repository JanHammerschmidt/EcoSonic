#ifndef MISC_H
#define MISC_H

struct FPSTimer {
    FPSTimer(std::string name = "FPS: ", qint64 interval = 2000)
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
            printf("%s%.3f\n", name.c_str(), double(count*1000) / elapsed);
            count = 0;
            timer.restart();
        }
    }

    QElapsedTimer timer;
    int frames = 0;
    std::string name;
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
    qreal get_elapsed() {
        return elapsed * 0.001;
    }

    QElapsedTimer timer;
    qint64 last_time = 0;
    qint64 elapsed = 0;
};

#endif // MISC_H
