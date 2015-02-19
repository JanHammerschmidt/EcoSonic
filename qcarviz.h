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

class SignObserverBase;
class TurnSignObserver;
struct TooSlowObserver;
class QCarViz;

struct TextHint {
    QString text;
    QElapsedTimer timer;
    qint64 normal_show_time = 2000;
    qreal fadeout_time = 1000;
    QFont font;
    QFontMetrics font_metrics;
    TextHint() : font{ "Helvetica Neue", 22, QFont::Bold }, font_metrics(font) {
        qDebug() << font_metrics.width("test");
    }
    void showText(const QString& text) {
        timer.start();
        this->text = text;
    }
    void draw(QPainter& painter, const QPointF pos) {
        if (!timer.isValid())
            return;
        qreal elapsed = timer.elapsed();
        if (elapsed >= normal_show_time + fadeout_time) {
            timer.invalidate();
            return;
        }
        painter.setFont(font);
        painter.setPen(Qt::black);
        if (elapsed <= normal_show_time) {
            //painter.setOpacity(1.0);
        } else {
            painter.setOpacity(1.0 - (elapsed - normal_show_time) / fadeout_time);
        }
        misc::draw_centered_text(painter, font_metrics, text, pos);
        //painter.drawText(QPointF(20,20), text);
        painter.setOpacity(1.0);
    }
};

enum TrafficViolation {
    Speeding,
    StopSign,
    TrafficLight,
};

struct EyeTrackerClient : public QThread
{
    Q_OBJECT

    typedef QAbstractSocket::SocketError SocketError;

signals:

    void destroyed();

public:
    EyeTrackerClient(QCarViz* car_viz, QCheckBox* connected_checkbox)
        : car_viz(car_viz)
        , connected_checkbox_(connected_checkbox)
    { }

    void disconnect() {
        quit = true;
    }

    void run() override {
        connect(this, &QThread::finished, this, &EyeTrackerClient::finish);
        socket.reset(new QTcpSocket());
        connect(socket.get(), &QTcpSocket::disconnected, this, &EyeTrackerClient::disconnected);
        connect(socket.get(), static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)> (&QTcpSocket::error),
                [&](QAbstractSocket::SocketError error)
        {
            if (!(socket->isOpen() && error == QAbstractSocket::SocketTimeoutError))
            qDebug() << "eye Tracker Socket ERROR:" << error;
        });
            //, &EyeTrackerClient::tcp_error(SocketAccessError));

        qDebug() << "connecting to EyeTracker-Server...";
        connected_checkbox_->setCheckState(Qt::PartiallyChecked);
       //socket->connectToHost("127.0.0.1", 7767);
        socket->connectToHost("192.168.0.10", 7767);
        if (!socket->waitForConnected(2000)) {
            qDebug() << "connecting failed!";
            connected_checkbox_->setCheckState(Qt::Unchecked);
            connected_checkbox_->setStyleSheet("QCheckBox { color: red }");
            return;
        }
        connected_checkbox_->setCheckState(Qt::Checked);
        connected_checkbox_->setStyleSheet("QCheckBox { color: black }");
        QElapsedTimer recv_timer; recv_timer.start();
        bool recv = true;
        while (!quit) {
            if (socket->waitForReadyRead(100)) {
                if (!recv) {
                    qDebug() << "eyetracker: incoming data";
                    connected_checkbox_->setStyleSheet("QCheckBox { color: black }");
                    recv = true;
                }
                read();
                recv_timer.start();
            } else if (recv_timer.elapsed() > 3000) {
                if (recv) {
                    qDebug() << "eyetracker: no data!";
                    connected_checkbox_->setStyleSheet("QCheckBox { color: red }");
                    recv = false;
                }
            }
        }
        socket->abort();
        socket->close();
        connected_checkbox_->setCheckState(Qt::Unchecked);
        connected_checkbox_->setStyleSheet("QCheckBox { color: red }");
        qDebug() << "stopping eye tracker thread";
    }

//    void destroy() {

//    }

protected slots:
    void disconnected() {
        qDebug() << "eye Tracker Socket disconnected";
        quit = true;
    }

    void finish() {
        emit destroyed();
        deleteLater();
    }

protected:
    void read();

    QCarViz* car_viz;
    QCheckBox* connected_checkbox_;
    //QTcpSocket socket;
    std::auto_ptr<QTcpSocket> socket;
    bool first_read = true;
    bool quit = false;
};

enum Condition {
    VIS = 0,
    SLP = 1,
    CNT = 2,
    NO_CONDITION = 3,
};

class QCarViz : public QWidget
{
    Q_OBJECT

public:
    friend class SignObserverBase;

    QCarViz(QWidget *parent = 0);

    virtual ~QCarViz() {
        if (eye_tracker_client != nullptr) {
            eye_tracker_client->disconnect();
            eye_tracker_client->wait(500);
        }
    }

    void init(Car* car, QPushButton* start_button, QCheckBox* eye_tracker_connected_checkbox, QSpinBox *vp_id, QComboBox* current_condition, QComboBox* next_condition,
              QSpinBox* run, QSlider* throttle, QSlider* breaking, QSpinBox* gear, QMainWindow* main_window, OSCSender* osc, bool start = true);

    void copy_from_track_editor(QTrackEditor* track_editor);

    void set_condition(Condition cond) {
        int sound_modus = (int) cond;
        set_sound_modus(sound_modus);
        current_condition_->setCurrentIndex(sound_modus);

        // always set next condition as well
        int idx = condition_order.indexOf(cond);
        Q_ASSERT(idx >= 0 && idx < 3);
        next_condition_->setCurrentIndex(idx == 2 ? 3 : (int) condition_order[idx + 1]);

        // reset run-counter
        run_->setValue(1);
    }

    void set_condition_order(const int id) {
        Q_ASSERT(id > 0 && id <= 6);
        condition_order.clear();
        switch (id) {
            case 1: condition_order << VIS << SLP << CNT; break;
            case 2: condition_order << SLP << VIS << CNT; break;
            case 3: condition_order << CNT << SLP << VIS; break;
            case 4: condition_order << VIS << CNT << SLP; break;
            case 5: condition_order << SLP << CNT << VIS; break;
            case 6: condition_order << CNT << VIS << SLP; break;
        }
        stop();
        reset();
        set_condition(condition_order[0]);
    }

    void set_sound_modus(int const sound_modus) {
        if (sound_modus == this->sound_modus)
            return;
        if (started)
            toggle_fedi(false);
        qDebug() << "sound modus:" << sound_modus;
        this->sound_modus = sound_modus;
        if (started)
            toggle_fedi(true);
    }

    void toggle_fedi(bool const enable) {
        if (enable) {
            Q_ASSERT(!sound_enabled);
            sound_enabled = true;
            if (sound_modus == 1)
                osc->call("/slurp_start");
            else if (sound_modus == 2)
                osc->call("/pitch_start");
            else if (sound_modus == 3)
                osc->call("/grain_start");
        } else {
            if (!sound_enabled)
                qDebug() << "warning: stopping sound that was not started";
            sound_enabled = false;
            if (sound_modus == 1)
                osc->call("/slurp_stop");
            else if (sound_modus == 2)
                osc->call("/pitch_stop");
            else if (sound_modus == 3)
                osc->call("/grain_stop");
        }
    }

    bool load_log(const QString filename, const bool start);
    void save_json(const QString filename);

    std::auto_ptr<HUDWindow> hud_window;

public slots:
    void stop(/*bool temporary_stop = false*/) {
        //tick_timer.stop();
        started = false;
        //if (!temporary_stop) {
            osc->call("/stopEngine");
            toggle_fedi(false);
        //}
        start_button->setText("Cont.");
        vp_id_->setReadOnly(false);
        current_condition_->setEnabled(true);
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

public:

    void globalToLocalCoordinates(QPointF &pos) const
    {
        const QWidget* w = this;
        while (w) {
            pos.rx() -= w->geometry().x();
            pos.ry() -= w->geometry().y();
            w = w->isWindow() ? 0 : w->parentWidget();
        }
    }

    void set_eye_tracker_point(QPointF& p) {
        // could me made thread-safe..
        if (!replay) {
            eye_tracker_point = p;
            t_last_eye_tracking_update = time_elapsed();
        }
    }

    void log_traffic_violation(const TrafficViolation violation);
    void show_traffic_violation(const TrafficViolation violation);
    void show_too_slow() {
        osc->call("/honk");
        text_hint.showText("You are driving a little too slow..");
        //text_hint.showText("Sie fahren aktuell etwas langsam..");
    }

    QPointF& get_eye_tracker_point() { return eye_tracker_point; }

    qreal get_kmh() { return Gearbox::speed2kmh(car->speed); }
    qreal get_user_steering() { return user_steering; }

    const QPainterPath& get_track_path() const { return track_path; }

    void steer(const qreal val) {
        steering = boost::algorithm::clamp(steering + val, -1, 1);
        //qDebug() << val << steering;
    }

    const Car* get_car() const { return car; }
    qreal get_current_pos() const { return current_pos; }

    Track track;
    QElapsedTimer flash_timer; // controls the display of a flash (white screen)

protected:

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
public:
    void reset();
    Log* log() { return car->log.get(); }
    bool is_replaying() { return replay; }
protected:

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
    qreal time_elapsed() {
        return time_delta.get_elapsed() - track_started_time;
    }
public:
    void toggle_connect_to_eyetracker() {
        if (eye_tracker_client != nullptr) {
            eye_tracker_client->disconnect();
            eye_tracker_client = nullptr;
        } else {
            eye_tracker_client = new EyeTrackerClient(this, eye_tracker_connected_checkbox_);
            connect(eye_tracker_client, &EyeTrackerClient::destroyed, [this](){
                //qDebug() << "eye_tracker_client = nullptr";
                eye_tracker_client = nullptr;
            });
            eye_tracker_client->start();
        }
    }
    void log_run();

protected:
    void draw(QPainter& painter);

    virtual void paintEvent(QPaintEvent *) {
//        static misc::FPSTimer fps("paint:");
//        fps.addFrame();
        if (started) {
            if (replay) {
                for (int i = 0; i < replay_speed_mult; i++)
                    if (!tick())
                        break;
            } else
                tick();
        }

        QPainter painter(this);
        draw(painter);

        if (started) {
            update();
        }
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
    std::auto_ptr<TooSlowObserver> tooslow_observer_;
    std::vector<SignObserverBase*> signObserver;
    TurnSignObserver* turnSignObserver = nullptr;
//    std::auto_ptr<TurnSignObserver> turnSignObserver;
//    std::auto_ptr<StopSignObserver> stopSignObserver;
    misc::TimeDelta time_delta;
    misc::TimeDelta time_delta_replay;
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
    bool track_started = false;
    qreal track_started_time = 0;
    OSCSender* osc = NULL;
    int sound_modus = 0;
    bool sound_enabled = false;
    QDateTime program_start_time;
    bool replay = false;
    int replay_index = 0;
    int replay_speed_mult = 1;
    std::auto_ptr<QSvgRenderer> turn_sign;
    QRectF turn_sign_rect;
    qreal steering = 0; // between -1 (left) and 1 (right)
    qreal user_steering = 0;
    QPointF eye_tracker_point;
    qreal t_last_eye_tracking_update = 0;
    EyeTrackerClient* eye_tracker_client = nullptr;
    QCheckBox* eye_tracker_connected_checkbox_ = nullptr;
    bool show_eye_tracker_point = false;
    TextHint text_hint;
    QSpinBox* vp_id_;
    QVector<Condition> condition_order;
    QComboBox* current_condition_;
    QComboBox* next_condition_;
    QSpinBox* run_;
    bool log_run_ = false;
};


#endif // QCARVIZ_H
