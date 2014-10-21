#ifndef QCARVIZ_H
#define QCARVIZ_H

#include <QWidget>
#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QThread>
#include <QElapsedTimer>
#include <string>
#include <QTimer>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTextStream>
#include <QShortcut>
#include <QMainWindow>
#include <QSvgGenerator>
#include <QSound>
#include <random>
#include "qtrackeditor.h"
#include "PedalInput.h"
#include "track.h"
#include "car.h"

#define DEFAULT_SPEED_LIMIT 300 // kmh

static std::mt19937_64 rng(std::random_device{}());

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

struct Speedometer {
    void draw(QPainter& painter, const QPointF pos, const qreal kmh)
    {
        // angle calculations
        const qreal start_angle = 0.5 * (M_PI + gap); // angle for min_speed (everything's on top!)
        const qreal angle_delta = (2*M_PI - gap) / speed_steps;

        // set font
        QFont font("Eurostile", 18);
        font.setBold(true);
        painter.setFont(font);
        QFontMetrics fm = painter.fontMetrics();

        // draw main ticks
        painter.setPen(QPen(Qt::black, 4));
        for (int i = 0; i <= speed_steps; i++) {
            const qreal angle = start_angle + i * angle_delta;
            const QPointF vec(cos(angle), sin(angle));
            painter.drawLine(pos + (radius-10) * vec, pos + radius * vec);
            QString str;
            QTextStream(&str) << min_speed + i * speed_delta;
            draw_centered_text(painter, fm, str, pos + (radius-19-fabs(cos(angle))*0.4*fm.width(str)) * vec);
        }

        // draw secondary ticks
        painter.setPen(QPen(Qt::black, 2));
        for (int i = 0; i < speed_steps; i++) {
            const qreal angle = start_angle + 0.5 * angle_delta + i * angle_delta;
            const QPointF vec(cos(angle), sin(angle));
            painter.drawLine(pos + (radius-8) * vec, pos + radius * vec);
        }

        // draw caption
        painter.setFont(QFont("Arial", 12));
        fm = painter.fontMetrics();
        draw_centered_text(painter, fm, caption, pos+QPointF(0, -0.5 * radius));

        // draw needle
        //painter.setPen(QPen(QBrush(QColor(201,14,14)), 6));
        painter.setPen(QPen(Qt::black, 1));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(QColor(201,14,14)));
        const qreal angle = start_angle + (kmh-min_speed) * angle_delta / speed_delta;
        const QPointF vec(cos(angle), sin(angle));
        const QPointF vecP(vec.y(), -vec.x()); // perpendicular vector
        const QPointF v1 = pos-0.1*radius*vec;
        const QPointF v2 = pos+(radius-30)*vec;
        const QPointF points[4] = { v1-4*vecP, v1+4*vecP, v2+vecP, v2-vecP };
        painter.drawConvexPolygon(points, 4);
        //painter.drawLine(pos-0.1*radius*vec, pos+(radius-20)*vec);
        //painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(Qt::black);
        painter.drawEllipse(pos, 12, 12);

    }
    static void draw_centered_text(QPainter& painter, QFontMetrics& fm, QString text, const QPointF pos) {
        const QPointF p = pos + QPointF(-0.5 * fm.width(text), 0.5 * fm.height());
        painter.drawText(p, text);
    }

    qreal max_speed() { return min_speed + speed_steps * speed_delta; }

    qreal radius = 100;
    qreal gap = M_PI/2; // gap at the bottom [degree]
    int min_speed = 10;
    int speed_delta = 20;
    int speed_steps = 10; // how many steps between min and max speed
    QString caption = "km/h";
};

struct RevCounter : public Speedometer
{
    RevCounter() {
        min_speed = 0;
        speed_delta = 10;
        speed_steps = 7;
        caption = "tr/min x100";
    }
};

struct HUD {
    struct ConsumptionDisplay {
        ConsumptionDisplay(QString legend1 = "L/", QString legend2 = "100km", const char* const number_format = "%04.1f")
            : number_format(number_format)
        {
            legend[0] = legend1;
            legend[1] = legend2;
            for (int i = 0; i < 3; i++)
                font_heights[i] = font_metrics[i].height();
        }
        void draw(QPainter& painter, QPointF pos, qreal consumption, bool draw_number = true) {
            QString number; number.sprintf(number_format, consumption); //QString::number(consumption, 'f', 1);

            const qreal l2_x_offset = -3; // x-offset of 2nd part of legend
            const qreal l2_y_offset = 2; // y-offset of 2nd part of legend (100km)
            const qreal y_gap = 0; // gap between number and legend
            //const qreal border_x = 0;
            //const qreal border_y = 0; // border for the bounding rect
            const qreal rect_y_offset = 3;
            const QSizeF rect_size(45,33);

            const qreal legend_height =  font_heights[1] + std::max(l2_y_offset, 0.);
            const qreal height = font_heights[0] + y_gap + legend_height; // total height

            const qreal number_width = draw_number ? font_metrics[0].width(number) : 0;
            const int legend_widths[2] = { font_metrics[1].width(legend[0]), font_metrics[2].width(legend[1]) };
            const qreal legend_width = legend_widths[0] + legend_widths[1] + l2_x_offset;
            //const qreal width = std::max(number_width, legend_width); // total width

            painter.setBrush(QBrush(QColor(180,180,180)));
            painter.setPen(Qt::NoPen);
            //painter.drawRoundedRect(QRectF(pos + QPointF(-0.5 * width - border_x, -0.5 * height - border_y + rect_y_offset), QSizeF(width + 2*border_x, height + 2*border_y)), 1, 1);
            painter.drawRoundedRect(QRectF(pos + QPointF(-0.5 * rect_size.width(), -0.5 * rect_size.height() + rect_y_offset), rect_size), 5, 3);
            painter.setPen(QPen(Qt::white));

            QPointF p;
            // draw number
            if (draw_number) {
                painter.setFont(fonts[0]);
                p = pos + QPointF(-0.5 * number_width, -0.5 * height + font_heights[0]);
                painter.drawText(p, number);
            }

            // draw 1st part of legend
            painter.setFont(fonts[1]);
            p = pos + QPointF(-0.5 * legend_width, 0.5 * height - l2_y_offset);
            painter.drawText(p, legend[0]);

            //draw 2nd part of legend
            painter.setFont(fonts[2]);
            p.setX(p.x() + legend_widths[0] + l2_x_offset);
            p.setY(p.y() + l2_y_offset);
            painter.drawText(p, legend[1]);
        }

        // fonts for number, 'L/', 100kmh
        QFont fonts[3] = { {"Eurostile", 18, QFont::Bold}, {"Eurostile", 12, QFont::Bold}, {"Eurostile", 8, QFont::Bold} };
        QFontMetrics font_metrics[3] = {QFontMetrics(fonts[0]), QFontMetrics(fonts[1]), QFontMetrics(fonts[2])};
        qreal font_heights[3];
        QString legend[2] = {"L/", "100km"};
        const char* const number_format;
    };

    ConsumptionDisplay consumption_display;
    ConsumptionDisplay trip_consumption{"dl/", "Trip", "%04.2f"};
    Speedometer speedometer;
    RevCounter rev_counter;
    qreal l_100km = 0;
    void draw(QPainter& painter, const qreal width, const qreal rpm, const qreal kmh, const qreal liters_used) {
        const qreal gap = 15;
        const qreal mid = width / 2.;
        rev_counter.draw(painter, QPointF(mid - rev_counter.radius - gap, rev_counter.radius + gap), 0.01 * rpm);
        speedometer.draw(painter, QPointF(mid + speedometer.radius + gap, speedometer.radius + gap), kmh);
        consumption_display.draw(painter, QPointF(mid, 180), l_100km, kmh >= 5);
        trip_consumption.draw(painter, QPointF(mid, 25), liters_used * 10);
    }

};

struct TreeType {
    struct SpeedyImage {
        QImage img;
        qreal kmh;
    };

    TreeType() {}
    TreeType(QString path, const qreal scale, const qreal y_offset)
        : scale(scale)
        , y_offset(y_offset)
    {
        img.load(path);
    }
    void draw_scaled(QPainter& painter, const QPointF pos, qreal kmh, qreal scale = 1) const {
        scale *= this->scale;
        QSizeF size(img.width() * scale, img.height() * scale);
        const QImage* img = &this->img;
        for (int i = 0; i < speedy_images.size(); i++) {
            if (kmh >= speedy_images[i].kmh)
                img = &speedy_images[i].img;
        }
        painter.drawImage(QRectF(pos.x() - 0.5 * size.width(), pos.y() - size.height() + y_offset * scale, size.width(), size.height()), *img);
    }
    void add_speedy_image(QString path, const qreal kmh) {
        speedy_images.append(SpeedyImage{QImage(path), kmh});
    }

    QImage img;
    QVector<SpeedyImage> speedy_images;
    qreal scale;
    qreal y_offset;
};

struct Tree {
    Tree() {}
    Tree(const int tree_type, const qreal pos, const qreal scale = 1, const qreal speed_scale = 1)
        : type(tree_type), pos(pos), scale(scale), speed_scale(speed_scale) {}

    qreal track_x(const qreal cur_x) {
        return cur_x + (pos - cur_x) * speed_scale;
    }

    int type = 0; // which type of tree
    qreal pos; // position on the track
    qreal scale = 1; // size-multiplier
    qreal speed_scale = 1; // speed-multiplier
};

struct KeyboardInput : public QObject
{
    void init(QMainWindow* main_window) {
        main_window->installEventFilter(this);
    }

    double throttle() { return keys_pressed.contains(Qt::Key_Up) ? 1 : 0; }
    double breaking() { return keys_pressed.contains(Qt::Key_Down) ? 1 : 0; }
    int gear_change() {
        const int ret = gears;
        gears = 0;
        return ret;
    }
    bool toggle_clutch() {
        const bool ret = clutch;
        clutch = false;
        return ret;
    }

    bool update() {
        const bool ret = changed;
        changed = false;
        return ret;
    }

protected:
    virtual bool eventFilter(QObject *obj, QEvent *e) {
        Q_UNUSED(obj);
//        if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) {
//            QKeyEvent *key = static_cast<QKeyEvent *>(e);
//            printf("%s: %d\n", (e->type() == QEvent::KeyPress) ? "down" : "up", key->key());
//        }
        int const key = static_cast<QKeyEvent *>(e)->key();
        if (key == Qt::Key_Up || key == Qt::Key_Down)
            changed = true;

        if (e->type() == QEvent::KeyPress) {
            switch (key) {
                case Qt::Key_W: gears += 1; break;
                case Qt::Key_S: gears -= 1; break;
                case Qt::Key_Space: clutch = true; break;
                default: keys_pressed.insert(key);
            }
        }
        else if (e->type() == QEvent::KeyRelease) {
            keys_pressed.remove(key);
        }
        return false;
    }
    QSet<int> keys_pressed;
    bool changed = false;
    int gears = 0;
    bool clutch = false;
};

struct SpeedObserver;

class QCarViz : public QWidget
{
    Q_OBJECT

public:
    friend struct SpeedObserver;
    QCarViz(QWidget *parent = 0);

    void init(Car* car, QPushButton* start_button, QSlider* throttle, QSlider* breaking, QSpinBox* gear, QMainWindow* main_window, OSCSender* osc, bool start = true);

    void copy_from_track_editor(QTrackEditor* track_editor);
signals:
    void slow_tick(qreal dt, qreal elapsed, ConsumptionMonitor& consumption_monitor);

protected slots:

    void start_stop() {
        started ? stop() : start();
    }

    void start() {
        if (current_pos >= track_path.length()) {
            current_pos = 0;
            car->gearbox.clutch.disengage();
            car->engine.reset();
            car->gearbox.reset();
            car->speed = 0;
            track_started = false;
            for (Track::Sign& s : track.signs) {
                if (s.type == Track::Sign::TrafficLight)
                    s.traffic_light_state = Track::Sign::Red;
            }
        }
        time_delta.start();
        tick_timer.start();
        started = true;
        start_button->setText("Pause");
    }
    void stop() {
        tick_timer.stop();
        started = false;
        start_button->setText("Cont.");
        //save_svg();
    }

    bool tick();

protected:

    void fill_trees() {
        trees.clear();
        const qreal first_distance = 40; // distance from starting position of the car!
        const qreal track_length = track_path.boundingRect().width();
        std::uniform_int_distribution<int> tree_type(0,tree_types.size()-1);
        std::uniform_real_distribution<qreal> dist(5,50); // distance between the trees
        const qreal first_tree = track_path.pointAtPercent(track_path.percentAtLength(initial_pos)).x() + first_distance;
        const qreal scale = 5;

        for (double x = first_tree; x < track_length; x += dist(rng)) {
            trees.append(Tree(tree_type(rng), x, scale, 10*scale));
        }
        //printf("%.3f\n", trees[0].pos);
    }

    void save_svg() {
        QSvgGenerator generator;
        generator.setFileName("/Users/jhammers/car_simulator.svg");
        const QSize size = this->size();
        generator.setSize(size);
        generator.setViewBox(QRect(0, 0, size.width(), size.height()));

        QPainter painter;
        painter.begin(&generator);
        draw(painter);
        painter.end();
    }

    void draw(QPainter& painter);

    virtual void paintEvent(QPaintEvent *) {
//        static FPSTimer fps("paint: ");
//        fps.addFrame();
        if (started)
            tick();

        QPainter painter(this);
        draw(painter);

        if (started)
            update();
    }

    void add_tree_type(const QString name, const qreal scale, const qreal y_offset) {
        const QString path = "media/trees/" + name;
        tree_types.append(TreeType(path + "_base.png", scale, y_offset));
        tree_types.last().add_speedy_image(path + "1", 10);
        tree_types.last().add_speedy_image(path + "2", 30);
        tree_types.last().add_speedy_image(path + "3", 50);
        tree_types.last().add_speedy_image(path + "4", 70);
        tree_types.last().add_speedy_image(path + "5", 100);
    }

    void update_track_path(const int height) {
        QPainterPath path;
        track.get_path(path, height);
        track_path.swap(path);
    }

    virtual void resizeEvent(QResizeEvent *e) {
        update_track_path(e->size().height());
    }

    const qreal initial_pos = 40;
    qreal current_pos = initial_pos; // current position of the car. max is: track_path.length()
    QImage car_img;
    Track track;
    std::auto_ptr<SpeedObserver> speedObserver;
    TimeDelta time_delta;
    Car* car;
    QPainterPath track_path;
    QVector<TreeType> tree_types;
    QVector<Tree> trees;
    bool started = false;
    QTimer tick_timer; // for simulation-ticks if the window is not visible
    QPushButton* start_button = NULL;
    QSlider* throttle_slider = NULL;
    QSlider* breaking_slider = NULL;
    QSpinBox* gear_spinbox = NULL;
    PedalInput pedal_input;
    KeyboardInput keyboard_input;
    ConsumptionMonitor consumption_monitor;
    HUD hud;
    QElapsedTimer flash_timer; // controls the display of a flash (white screen)
    bool track_started = false;
    qreal track_started_time = 0;
};

#include <QtConcurrent>

#define NEXT_SIGN_DISTANCE 800 // ??
#define MIN_DRIVING_DISTANCE 400 // ??
#define TOO_SLOW_TOLERANCE 0.2
#define TOO_FAST_TOLERANCE 0.1
#define MAX_TOO_FAST 2000 // ms
#define MAX_TOO_SLOW 3000 // ms
#define COOLDOWN_TIME 3000 // how long nothing happens after a honking (ms)

struct SpeedObserver {
    SpeedObserver(QCarViz& carViz, OSCSender& osc)
        : track(carViz.track), carViz(carViz), osc(osc)
    {
        last_state_change.start();
        cooldown_timer.start();
    }
    enum State {
        OK,
        too_slow,
        too_fast,
    };
    static void trigger_traffic_light(Track::Sign* traffic_light) {
        std::pair<qreal,qreal>& time_range = traffic_light->traffic_light_info.time_range;
        std::uniform_int_distribution<int> time(time_range.first,time_range.second);
        QThread::msleep(time(rng));
        traffic_light->traffic_light_state = Track::Sign::Yellow;
        QThread::msleep(1000);
        traffic_light->traffic_light_state = Track::Sign::Green;
    }
    void tick() {
        const qreal current_pos = carViz.current_pos;
        Track::Sign* current_speed_sign = NULL;
        Track::Sign* current_hold_sign = NULL;
        Track::Sign* next_sign = NULL;
        for (Track::Sign& s : track.signs) {
            if (current_pos < s.at_length) {
                if (s.at_length - current_pos > NEXT_SIGN_DISTANCE)
                    break;
                if ((s.type == Track::Sign::TrafficLight && s.traffic_light_state == Track::Sign::Green) // green traffic lights
                    || (s.is_speed_sign() && s.type > current_speed_sign->type)) { // faster speed sign
                        continue; // skip!
                }
                next_sign = &s;
                break;
            } else if (s.is_speed_sign()) {
                current_speed_sign = &s;
            } else
                current_hold_sign = &s;
        }
        current_speed_limit = current_speed_sign ? current_speed_sign->speed_limit() : DEFAULT_SPEED_LIMIT;

        if (next_sign && next_sign->type == Track::Sign::TrafficLight && next_sign->traffic_light_state == Track::Sign::Red
                && (next_sign->at_length - current_pos) < next_sign->traffic_light_info.trigger_distance)
        {
            printf("trigger!\n");
            next_sign->traffic_light_state = Track::Sign::Red_pending;
            QtConcurrent::run(trigger_traffic_light, next_sign);
        }
        const qreal kmh = Gearbox::speed2kmh(carViz.car->speed);
        const bool toofast = kmh > current_speed_limit * (1+TOO_FAST_TOLERANCE);
        bool tooslow = kmh < current_speed_limit * (1-TOO_SLOW_TOLERANCE);
        if (tooslow) {
            if (carViz.current_pos < MIN_DRIVING_DISTANCE)
                tooslow = false;
            if (next_sign && (next_sign->type == Track::Sign::TrafficLight || next_sign->type == Track::Sign::Stop
                || (next_sign->is_speed_sign() && kmh < next_sign->speed_limit() * (1-TOO_SLOW_TOLERANCE))))
            {
                    tooslow = false;
            }
        }
        if (cooldown_timer.elapsed() < COOLDOWN_TIME)
            return;
        if (current_hold_sign
                && !(current_hold_sign->type == Track::Sign::TrafficLight && current_hold_sign->traffic_light_state != Track::Sign::Red)
                && current_pos - current_hold_sign->at_length > 10)
        {
            osc.send_float("/flash", 0);
            carViz.flash_timer.start();
            cooldown_timer.start();
            return;
        }
        if (toofast) {
            if (state == too_fast) {
                if (last_state_change.elapsed() > MAX_TOO_FAST) {
                    osc.send_float("/flash", 0);
                    carViz.flash_timer.start();
                    cooldown_timer.start();
                    state = OK;
                }
            } else {
                state = too_fast;
                last_state_change.start();
            }
        } else if (tooslow) {
            if (state == too_slow) {
                if (last_state_change.elapsed() > MAX_TOO_SLOW) {
                    osc.send_float("/honk", 0);
                    cooldown_timer.start();
                    state = OK;
                }
            } else {
                state = too_slow;
                last_state_change.start();
            }
        }
    }
    State state = OK;
    QElapsedTimer last_state_change;
    QElapsedTimer cooldown_timer;
    qreal current_speed_limit = DEFAULT_SPEED_LIMIT;
    Track& track;
    QCarViz& carViz;
    OSCSender& osc;
};

#endif // QCARVIZ_H
