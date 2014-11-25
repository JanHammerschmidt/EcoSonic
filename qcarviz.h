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
#include "hud.h"
#include "qtrackeditor.h"
#include "PedalInput.h"
#include "KeyboardInput.h"
#include "misc.h"
#include "track.h"
#include "car.h"

#define DEFAULT_SPEED_LIMIT 300 // kmh

static std::mt19937_64 rng(std::random_device{}());

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
            consumption_monitor.reset();
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



#endif // QCARVIZ_H
