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
#include <random>
#include "qtrackeditor.h"
#include "PedalInput.h"
#include "track.h"
#include "car.h"

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
        ConsumptionDisplay(QString legend2 = "100km", const char* const number_format = "%04.1f")
            : number_format(number_format)
        {
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
    ConsumptionDisplay trip_consumption{"Trip", "%04.2f"};
    Speedometer speedometer;
    RevCounter rev_counter;
    qreal l_100km = 0;
    void draw(QPainter& painter, const qreal width, const qreal rpm, const qreal kmh, const qreal liters_used) {
        const qreal gap = 15;
        const qreal mid = width / 2.;
        rev_counter.draw(painter, QPointF(mid - rev_counter.radius - gap, rev_counter.radius + gap), 0.01 * rpm);
        speedometer.draw(painter, QPointF(mid + speedometer.radius + gap, speedometer.radius + gap), kmh);
        consumption_display.draw(painter, QPointF(mid, 180), l_100km, kmh >= 5);
        trip_consumption.draw(painter, QPointF(mid, 25), liters_used);
    }

};

struct TreeType {
    TreeType() {}
    TreeType(QString path, const qreal scale, const qreal y_offset)
        : scale(scale)
        , y_offset(y_offset)
    {
        img.load(path);
    }
    void draw_scaled(QPainter& painter, const QPointF pos, qreal scale = 1) {
        scale *= this->scale;
        QSizeF size(img.width() * scale, img.height() * scale);
        painter.drawImage(QRectF(pos.x() - 0.5 * size.width(), pos.y() - size.height() + y_offset * scale, size.width(), size.height()), img);
    }

    QImage img;
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

class QCarViz : public QWidget
{
    Q_OBJECT

public:
    friend class CarUpdateThread;
    QCarViz(QWidget *parent = 0)
        : QWidget(parent)
    {
        setAutoFillBackground(true);
        QPalette p = palette();
        p.setColor(backgroundRole(), Qt::white);
        setPalette(p);

        printf("%s\n", QDir::currentPath().toStdString().c_str());
        tree_types.append(TreeType("media/trees/tree.png", 0.2/3, 50*3));
        tree_types.append(TreeType("media/trees/birch.png", 0.08, 140));
        tree_types.append(TreeType("media/trees/spooky_tree.png", 0.06, 120));
        car_img.load("media/cars/car.png");
        track.load();
        update_track_path(height());
        fill_trees();
        tick_timer.setInterval(30); // minimum simulation interval
        QObject::connect(&tick_timer, SIGNAL(timeout()), this, SLOT(tick()));
    }

    void init(Car* car, QPushButton* start_button, QSlider* throttle, QSlider* breaking, QSpinBox* gear, QMainWindow* main_window, bool start = true) {
        this->car = car;
        this->start_button = start_button;
        throttle_slider = throttle;
        breaking_slider = breaking;
        gear_spinbox = gear;
        QObject::connect(start_button, SIGNAL(clicked()),
                         this, SLOT(start_stop()));
        keyboard_input.init(main_window);
        if (start)
            this->start();
    }

    void start() {
        time_delta.start();
        tick_timer.start();
        started = true;
        start_button->setText("Pause");
    }
    void stop() {
        tick_timer.stop();
        started = false;
        start_button->setText("Cont.");
    }

    void copy_from_track_editor(QTrackEditor* track_editor) {
        track = track_editor->track;
        update_track_path(height());
        fill_trees();
    }

signals:
    void slow_tick(qreal dt, qreal elapsed, ConsumptionMonitor& consumption_monitor);

protected slots:

    void start_stop() {
        started ? stop() : start();
    }

    bool tick() {
        Q_ASSERT(started);
        qreal dt;
        if (!time_delta.get_time_delta(dt))
            return false;

        static bool changed = false;
        // keyboard input
        if (keyboard_input.update()) {
            car->throttle = keyboard_input.throttle();
            car->breaking = keyboard_input.breaking();
            changed = true;
        }
        const int gear_change = keyboard_input.gear_change();
        if (gear_change) {
            if (gear_change > 0)
                car->gearbox.gear_up();
            else if (gear_change < 0)
                car->gearbox.gear_down();
            gear_spinbox->setValue(car->gearbox.gear+1);
        }
        const bool toggle_clutch = keyboard_input.toggle_clutch();
        if (toggle_clutch) {
            Clutch& clutch = car->gearbox.clutch;
            if (clutch.engage)
                clutch.disengage();
            else
                clutch.clutch_in(car->engine, &car->gearbox, car->speed);
        }

        // pedal input
        if (pedal_input.valid() && pedal_input.update()) {
            car->throttle = pedal_input.gas();
            car->breaking = pedal_input.brake();
            changed = true;
        }

        const qreal alpha = atan(-track_path.slopeAtPercent(track_path.percentAtLength(current_pos))); // slope [rad]
        Q_ASSERT(!isnan(alpha));
        car->tick(dt, alpha);
        current_pos += car->speed * dt * 3;
        if (current_pos > track_path.length())
            current_pos = 0;
        consumption_monitor.tick(car->engine.get_consumption_L_s(), dt, car->speed);
        double l_100km;
        if (consumption_monitor.get_l_100km(l_100km, car->speed)) {
            hud.l_100km = l_100km;
            printf("%.3f\n", l_100km);
        }

        static qreal last_elapsed = 0;
        qreal elapsed = time_delta.get_elapsed();
        if (elapsed - last_elapsed > 0.05) {
            if (changed) {
                throttle_slider->setValue(car->throttle * 100);
                breaking_slider->setValue(car->breaking * 100);
                changed = false;
            } else {
                car->throttle = throttle_slider->value() / 100.;
                car->breaking = breaking_slider->value() / 100.;
            }
            emit slow_tick(elapsed - last_elapsed, elapsed, consumption_monitor);
            last_elapsed = elapsed;
        }
        return true;
    }

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
        printf("%.3f\n", trees[0].pos);
    }

    virtual void paintEvent(QPaintEvent *) {
//        static FPSTimer fps("paint: ");
//        fps.addFrame();
        if (started)
            tick();

        const qreal current_percent = track_path.percentAtLength(current_pos);
        const qreal car_x_pos = 50;
        QPainter painter(this);
        const QPointF cur_p = track_path.pointAtPercent(current_percent);
        //printf("%.3f\n", current_alpha * 180 / M_PI);

        // draw the HUD speedometer & revcounter
        hud.draw(painter, width(), car->engine.rpm(), Gearbox::speed2kmh(car->speed), consumption_monitor.liters_used);

        // draw the road
        painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(Qt::NoBrush);
        QTransform t;
        t.translate(car_x_pos - cur_p.x(),0);
        painter.setTransform(t);
        painter.drawPath(track_path);

        // draw the signs
        for (int i = 0; i < track.signs.size(); i++) {
            track.signs[i].draw(painter, track_path);
        }

        // draw the car
        const qreal car_width = 60.;
        const qreal car_height = (qreal) car_img.size().height() / car_img.size().width() * car_width;

        t.reset();
        t.translate(car_x_pos, cur_p.y());
        //t.rotateRadians(atan(slope));
        t.rotate(-track_path.angleAtPercent(current_percent));
        t.translate(-car_width/2, -car_height);
        painter.setTransform(t);
        painter.drawImage(QRectF(0,0, car_width, car_height), car_img);

        // draw the trees in the foreground
        t.reset();
        t.translate(car_x_pos - cur_p.x(),0);
        painter.setTransform(t);

        const qreal track_bottom = track_path.boundingRect().bottom();
        for (Tree tree : trees) {
            const qreal tree_x = tree.track_x(cur_p.x());
            QPointF tree_pos(tree_x, track_bottom);
            tree_types[tree.type].draw_scaled(painter, tree_pos, tree.scale);
        }

        if (started)
            update();
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
    qreal current_pos = initial_pos; // max is: track_path.length()
    QImage car_img;
    Track track;
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
};


#endif // QCARVIZ_H
