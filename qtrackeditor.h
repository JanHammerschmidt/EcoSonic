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

    void init(QSpinBox* width_control, QSpinBox* num_control_points, QSpinBox* show_control_points, QPushButton* add_sign, QPushButton* reset, QSpinBox* max_time) {
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
        num_control_points->setValue(track.num_points);
        width_control->setValue(track.width / 10);
        this->num_control_points = num_control_points;
        this->show_control_points = show_control_points;
        max_time->setValue(track.max_time);
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


public slots:
    void add_sign() {
        track.signs.push_back(Track::Sign{Track::Sign::Stop, 0});
        update();
    }
    void reset() {
        track.points.resize(1);
        track.num_points = 0;
        track.signs.resize(0);
        num_control_points->setValue(0);
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
                track.points.push_back(track.points.last() + QPointF(50,0));
        }
        track.num_points = num_points;
        update();
    }
    void change_max_time(int time) {
        track.max_time = time;
    }
};

#endif // QTRACKEDITOR_H
