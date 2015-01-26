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
#include <QTcpSocket>
#include <boost/algorithm/clamp.hpp>
#include <algorithm>
#include <random>
#include "hud.h"
#include "qtrackeditor.h"
#include "wingman_input.h"
#include "KeyboardInput.h"
#include "misc.h"
#include "track.h"
#include "car.h"
#include "hudwindow.h"

#include <QMessageBox>
#include <QLabel>
#include <QMenu>



#define DEFAULT_SPEED_LIMIT 300 // kmh

static std::mt19937_64 rng(std::random_device{}());

struct SpeedObserver;
struct TurnSignObserver;

class QCarViz : public QWidget
{
    Q_OBJECT

public:
    friend struct SpeedObserver;
    friend struct TurnSignObserver;
    friend struct Log;
    QCarViz(QWidget *parent = 0);

    void init(Car* car, QPushButton* start_button, QSlider* throttle, QSlider* breaking, QSpinBox* gear, QMainWindow* main_window, OSCSender* osc, bool start = true);

    void copy_from_track_editor(QTrackEditor* track_editor);

    void update_sound_modus(int const sound_modus) {
        if (sound_modus != this->sound_modus) {
            if (this->sound_modus == 1)
                osc->call("/slurp_stop");
            else if (this->sound_modus == 2)
                osc->call("/pitch_stop");
            else if (this->sound_modus == 3)
                osc->call("/grain_stop");

            if (sound_modus == 1)
                osc->call("/slurp_start");
            else if (sound_modus == 2)
                osc->call("/pitch_start");
            else if (sound_modus == 3)
                osc->call("/grain_start");
            this->sound_modus = sound_modus;
        }
    }

    bool load_log(const QString filename);

    std::auto_ptr<HUDWindow> hud_window;

public slots:
    void stop(bool temporary_stop = false) {
        tick_timer.stop();
        started = false;
        if (!temporary_stop) {
            osc->call("/stopEngine");
            update_sound_modus(0);
        }
        start_button->setText("Cont.");
        //save_svg();
    }

    void start();

signals:
    void slow_tick(qreal dt, qreal elapsed, ConsumptionMonitor& consumption_monitor);

protected slots:

    void start_stop() {
        started ? stop() : start();
    }

    bool tick();

    void eye_tracker_connected() {
        qDebug() << "eye Tracker Socket connected";
    }
    void eye_tracker_disconnected() {
        qDebug() << "eye Tracker Socket disconnected";
    }

    void eye_tracker_error(QAbstractSocket::SocketError error) {
        qDebug() << "eye Tracker Socket ERROR:" << error;
    }

    void eye_tracker_read() {
        static const qint64 chunk_size = sizeof(double)*2;
        char raw_data[chunk_size*50];
        qDebug() << "eye Tracker Socket read";
        qint64 avail = eye_tracker_socket.bytesAvailable();
        if (avail < chunk_size)
            return;
        const qint64 size = std::min((avail/chunk_size) * chunk_size, (qint64) sizeof(raw_data));
        eye_tracker_socket.read(raw_data, size);
        double* data = (double*) &raw_data[size-chunk_size];
        //qDebug() << data[0] << data[1];
        eye_tracker_point.setX(data[0]);
        eye_tracker_point.setY(data[1]);
        globalToLocalCoordinates(eye_tracker_point);
    }

protected:

    void globalToLocalCoordinates(QPointF &pos) const
    {
        const QWidget* w = this;
        while (w) {
            pos.rx() -= w->geometry().x();
            pos.ry() -= w->geometry().y();
            w = w->isWindow() ? 0 : w->parentWidget();
        }
    }

    void fill_trees() {
        trees.clear();
        const qreal first_distance = 40; // distance from starting position of the car
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

    void prepare_track();

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

    void trigger_arrow();
    void steer(const qreal val) {
        steering = boost::algorithm::clamp(steering + val, -1, 1);
        //qDebug() << val << steering;
    }
    qreal time_elapsed() {
        return time_delta.get_elapsed() - track_started_time;
    }

    void connect_to_eyetracker() {
        if (eye_tracker_socket.isOpen()) {
            qDebug() << "disconnecting...";
            eye_tracker_socket.abort();
            eye_tracker_socket.close();
        } else {
            qDebug() << "connecting...";
            eye_tracker_socket.connectToHost("127.0.0.1", 7767);
        }
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
    std::auto_ptr<TurnSignObserver> turnSignObserver;
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
    WingmanInput wingman_input;
    KeyboardInput keyboard_input;
    ConsumptionMonitor consumption_monitor;
    HUD hud;
    QElapsedTimer flash_timer; // controls the display of a flash (white screen)
    bool track_started = false;
    qreal track_started_time = 0;
    OSCSender* osc = NULL;
    int sound_modus = 0;
    QDateTime program_start_time;
    bool replay = false;
    int replay_index = 0;
    std::auto_ptr<QSvgRenderer> turn_sign;
    QRectF turn_sign_rect;
    qreal steering = 0; // between -1 (left) and 1 (right)
//    qreal turn_sign_length = 0;
//    qreal current_turn_sign_length = 0;
    QTcpSocket eye_tracker_socket;
    QPointF eye_tracker_point;
};


#endif // QCARVIZ_H
