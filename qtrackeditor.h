#ifndef QTRACKEDITOR_H
#define QTRACKEDITOR_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QDir>
#include <QSpinBox>
#include <QPushButton>
#include <QMenu>
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

    void init(QSpinBox* width_control, QSpinBox* num_control_points, QSpinBox* show_control_points, QPushButton* add_sign, QPushButton* reset) {
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
        num_control_points->setValue(track.num_points);
        width_control->setValue(track.width / 10);
        this->num_control_points = num_control_points;
        this->show_control_points = show_control_points;
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

    virtual void paintEvent(QPaintEvent *) {
        QPainter painter(this);
        const qreal h = height();
        //QVector<QPointF>& points = track.points;

        // draw the line
        QPainterPath path;
        track.get_path(path, h);
        painter.drawPath(path);

        // draw the signs
        for (int i = 0; i < track.signs.size(); i++) {
            track.signs[i].draw(painter, path);
        }

        // Draw the control points
        if (show_control_points->value() > 0) {
            const bool main_points_only = show_control_points->value() < 2;
            const qreal point_size = this->point_size;
            painter.setPen(QColor(50, 100, 120, 200));
            painter.setBrush(QColor(200, 200, 210, 120));
            for (int i=0; i<=track.num_points*3; ++i) {
                QPointF pos = Track::tf(track.points[i],h);
                painter.drawEllipse(QRectF(pos.x() - point_size,
                                           pos.y() - point_size,
                                           point_size*2, point_size*2));
                if (main_points_only)
                    i += 2;
            }
//            painter.drawEllipse(QRectF(pp.x() - point_size,
//                                       pp.y() - point_size,
//                                       point_size*2, point_size*2));

        }
        // draw a line between the control points
        if (show_control_points->value() >= 2) {
            painter.setPen(QPen(Qt::lightGray, 0, Qt::SolidLine));
            painter.setBrush(Qt::NoBrush);
            QPainterPath points_path;
            track.get_path_points(points_path, h);
            painter.drawPath(points_path);
//            painter.drawPolyline(&track.points[0],track.num_points*3+1);
        }
    }

    void mousePressEvent(QMouseEvent *e)
    {
        const qreal h = height();
        const QPointF mp = e->pos();

        // first check the signs
        sign_moving = -1;
        QPainterPath path;
        track.get_path(path, h);
        QRectF pos;
        for (int i = 0; i < track.signs.size(); i++) {
            if (track.signs[i].get_position(pos, path) && pos.contains(mp)) {
                sign_moving = i;
                break;
            }
        }
        if (sign_moving >= 0) {
            if (e->button() == Qt::RightButton) {
                QMenu menu(this);
                const QString del("Delete!");
                menu.addAction(del);
                for (auto& i : track.sign_images)
                    menu.addAction(i.name);
                QAction* a = menu.exec(e->globalPos());
                if (a) {
                    QString text = a->text();
                    if (text == del)
                        track.signs.remove(sign_moving);
                    else {
                        for (int i = 0; i < track.sign_images.size(); i++) {
                            if (text == track.sign_images[i].name) {
                                track.signs[sign_moving].type = (Track::Sign::Type) i;
                                break;
                            }
                        }
                    }
                    sign_moving = -1;
                    update();
                }
            } else
                mouseMoveEvent(e);
            return;
        }

        // then check the control points
        if (show_control_points->value() <= 0)
            return;

        QVector<QPointF>& points = track.points;

        const bool main_points_only = show_control_points->value() < 2;
        qreal dist = -1;
        for (int i = 0; i < points.size(); i++) {
            const QPointF p = Track::tf(points[i], h);
            const qreal d = QLineF(mp.x(), mp.y(), p.x(), p.y()).length();
            if (d < dist || (dist < 0 && d < 2 * point_size)) {
                dist = d;
                point_moving = i;
            }
            if (main_points_only)
                i += 2;
        }
        if (point_moving != -1)
            mouseMoveEvent(e);
    }
    void mouseMoveEvent(QMouseEvent *e) {
        if (sign_moving >= 0) {
            const qreal x = e->x();
            QPainterPath path;
            track.get_path(path, height());
            const qreal length = path.length();
            qreal l = x - track.points[0].x();
            while (l < length) {
                if (path.pointAtPercent(path.percentAtLength(l)).x() >= x)
                    break;
                l++;
            }
            track.signs[sign_moving].at_length = std::min(l,length);
//            qreal p = path.percentAtLength(l);
//            pp = path.pointAtPercent(p);
//            //printf("(%.3f,%.3f)\n", pp.x(), pp.y());
            update();
        }
        else if (point_moving >= 0) {
            const qreal h = height();
            QVector<QPointF>& points = track.points;
            QPointF p = Track::tf_1(e->pos(), h);
            if (p.x() < 0) p.setX(0);
            if (p.x() > size().width()) p.setX(size().width());
            if (p.y() < 0) p.setY(0);
            if (p.y() > h) p.setY(h);
            QPointF& pn = points[point_moving];
            const QPointF delta = p - pn;
            pn = p;
            if (point_moving % 3 == 0) { // end-point
                if (point_moving > 0)
                    points[point_moving-1] += delta;
                if (point_moving < points.size()-1)
                    points[point_moving+1] += delta;
            } else if (point_moving > 1 && point_moving < points.size()-2){ // control-point
                const bool first_cp = point_moving % 3 == 1; // first_control-point ?
                const QPointF& center = points[point_moving + (first_cp ? -1 : 1)]; // end-point
                const QPointF diff = Track::tf(pn,h) - Track::tf(center,h);
                const qreal angle = atan2(diff.y(), diff.x()) + M_PI; // angle (for other point)
                QPointF& np = points[point_moving + (first_cp ? -2 : 2)]; // other point
                const qreal l = QLineF(Track::tf(np,h), Track::tf(center,h)).length(); // length to other point
                np = Track::tf_1(Track::tf(center,h) + QPointF(cos(angle), sin(angle)) * l, h);
            }
            update();
        }
    }
    void mouseReleaseEvent(QMouseEvent *) {
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
};

#endif // QTRACKEDITOR_H
