#ifndef QTRACKEDITOR_H
#define QTRACKEDITOR_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QDir>
#include <QSpinBox>
#include <QPushButton>
#include <QMenu>
#include <math.h>
#include <QMessageBox>
#include <QLabel>
#include "track.h"

// TODO: angleAtPercent ?

class QTrackEditor : public QWidget
{
    Q_OBJECT
public:
    friend class QCarViz;
    explicit QTrackEditor(QWidget *parent = 0) {
        Q_UNUSED(parent);

        //setAutoFillBackground(true);
        QPalette p = palette();
        p.setColor(backgroundRole(), Qt::white);
        setPalette(p);

        if (!load_track())
            track.points.push_back(QPointF(0,100));
    }

    virtual ~QTrackEditor() {
        save_track();
    }

    void init(QSpinBox* width_control, QSpinBox* num_control_points, QSpinBox* show_control_points,
              QPushButton* add_sign, QPushButton* reset, QSpinBox* max_time,
              QPushButton* prune_points, QSpinBox* new_points_distance,
              QSpinBox* tl_min_time, QSpinBox* tl_max_time, QSpinBox* tl_distance,
              QLabel* lbl_min_time, QLabel* lbl_max_time, QLabel* lbl_distance,
              QDoubleSpinBox* steer_intensity, QDoubleSpinBox* steer_time, QDoubleSpinBox* steer_fade_in, QDoubleSpinBox* steer_fade_out,
              QLabel* lbl_steer_intensity, QLabel* lbl_steer_time, QLabel* lbl_steer_fade_in, QLabel* lbl_steer_fade_out)
    {
        QObject::connect(width_control, SIGNAL(valueChanged(int)),
                         this, SLOT(change_width(int)));
        QObject::connect(num_control_points, SIGNAL(valueChanged(int)),
                         this, SLOT(change_num_points(int)));
        QObject::connect(show_control_points, SIGNAL(valueChanged(int)),
                         this, SLOT(update()));
        QObject::connect(add_sign, SIGNAL(clicked()),
                         this, SLOT(add_sign()));
        QObject::connect(reset, SIGNAL(clicked()),
                         this, SLOT(reset()));
        QObject::connect(max_time, SIGNAL(valueChanged(int)),
                         this, SLOT(change_max_time(int)));
        QObject::connect(prune_points, SIGNAL(clicked()),
                         this, SLOT(prune_points()));

        // traffic light infos
        QObject::connect(tl_min_time, SIGNAL(valueChanged(int)),
                         this, SLOT(tl_min_time_changed(int)));
        QObject::connect(tl_max_time, SIGNAL(valueChanged(int)),
                         this, SLOT(tl_max_time_changed(int)));
        QObject::connect(tl_distance, SIGNAL(valueChanged(int)),
                         this, SLOT(tl_distance_changed(int)));

        // steering infos
        QObject::connect(steer_intensity, SIGNAL(valueChanged(double)),
                         this, SLOT(steer_intensity_changed(double)));
        QObject::connect(steer_time, SIGNAL(valueChanged(double)),
                         this, SLOT(steer_time_changed(double)));
        QObject::connect(steer_fade_in, SIGNAL(valueChanged(double)),
                         this, SLOT(steer_fade_in_changed(double)));
        QObject::connect(steer_fade_out, SIGNAL(valueChanged(double)),
                         this, SLOT(steer_fade_out_changed(double)));

        num_control_points->setValue(track.num_points);
        width_control->setValue(track.width / 10);
        this->num_control_points = num_control_points;
        this->show_control_points = show_control_points;
        this->new_points_distance = new_points_distance;
        max_time->setValue(track.max_time);

        // traffic light infos
        this->tl_distance = tl_distance;
        this->tl_min_time = tl_min_time;
        this->tl_max_time = tl_max_time;
        tl_widgets.append(tl_distance); tl_widgets.append(tl_min_time); tl_widgets.append(tl_max_time);
        tl_widgets.append(lbl_distance); tl_widgets.append(lbl_min_time); tl_widgets.append(lbl_max_time);
        for (auto w : tl_widgets)
            w->setVisible(false);

        // steering infos
        this->steer_intensity = steer_intensity; this->steer_time = steer_time;
        this->steer_fade_in = steer_fade_in; this->steer_fade_out = steer_fade_out;
        steer_widgets.append(steer_intensity); steer_widgets.append(steer_time), steer_widgets.append(steer_fade_in); steer_widgets.append(steer_fade_out);
        steer_widgets.append(lbl_steer_intensity), steer_widgets.append(lbl_steer_time), steer_widgets.append(lbl_steer_fade_in), steer_widgets.append(lbl_steer_fade_out);
        for (auto w : steer_widgets)
            w->setVisible(false);
    }

    bool save_track() {
        return track.save();
    }
    bool load_track() {
        return track.load();
        change_width(track.width / 10);
        update();
        return true;
    }

protected:

//    static inline QPointF scale(const QPointF& p, const qreal height) {
//        return QPointF(p.x(), p.y() * height);
//    }

    virtual void paintEvent(QPaintEvent *);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);

    virtual void mouseReleaseEvent(QMouseEvent *) {
        sign_moving = -1;
        point_moving = -1;
    }

    Track track;
    int point_moving = -1;
    int sign_moving = -1;
    qreal point_size = 6;
    QSpinBox* num_control_points = NULL;
    QSpinBox* show_control_points = NULL;
    QSpinBox* new_points_distance = NULL;

    QSpinBox* tl_min_time = NULL;
    QSpinBox* tl_max_time = NULL;
    QSpinBox* tl_distance = NULL;
    QList<QWidget*> tl_widgets;
    Track::Sign* selected_traffic_light = NULL;

    QDoubleSpinBox* steer_intensity = nullptr;
    QDoubleSpinBox* steer_time = nullptr;
    QDoubleSpinBox* steer_fade_in = nullptr;
    QDoubleSpinBox* steer_fade_out = nullptr;
    QList<QWidget*> steer_widgets;
    Track::Sign* selected_steer_sign = NULL;


public slots:
    void add_sign() {
        track.signs.push_back(Track::Sign{Track::Sign::Stop, 0});
        update();
    }
    void reset() {
        if (QMessageBox::question(this, "EcoSonic", "Sure?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
            return;
        track.points.resize(1);
        track.num_points = 0;
        track.signs.resize(0);
        num_control_points->setValue(0);
        update();
    }
    void prune_points() {
        track.points.resize(track.num_points * 3 + 1);
    }
    void tl_min_time_changed(int t) {
        Q_ASSERT(selected_traffic_light);
        selected_traffic_light->traffic_light_info.time_range.first = t;
        update();
    }
    void tl_max_time_changed(int t) {
        Q_ASSERT(selected_traffic_light);
        selected_traffic_light->traffic_light_info.time_range.second = t;
        update();
    }
    void tl_distance_changed(int d) {
        Q_ASSERT(selected_traffic_light);
        selected_traffic_light->traffic_light_info.trigger_distance = d;
        update();
    }

    // steering info
    void steer_intensity_changed(double d) {
        Q_ASSERT(selected_steer_sign);
        selected_steer_sign->steering_info.intensity = d;
        update();
    }
    void steer_time_changed(double d) {
        Q_ASSERT(selected_steer_sign);
        selected_steer_sign->steering_info.duration = d;
        update();
    }
    void steer_fade_in_changed(double d) {
        Q_ASSERT(selected_steer_sign);
        selected_steer_sign->steering_info.fade_in = d;
        update();
    }
    void steer_fade_out_changed(double d) {
        Q_ASSERT(selected_steer_sign);
        selected_steer_sign->steering_info.fade_out = d;
        update();
    }


    void change_width(int width) {
        width *= 10;
        setMinimumWidth(width);
        track.width = width;
    }
    void change_num_points(int num_points) {
        const int cur_num_points = (track.points.size() - 1) / 3; // "real" point count
        if (num_points > cur_num_points) {
            for (int i = track.points.size(); i <= num_points*3; i++)
                track.points.push_back(track.points.last() + QPointF(new_points_distance->value(),0));
        }
        track.num_points = num_points;
        update();
    }
    void change_max_time(int time) {
        track.max_time = time;
    }
};

#endif // QTRACKEDITOR_H
