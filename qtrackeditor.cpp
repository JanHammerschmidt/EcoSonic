#include "stdafx.h"
#include "qtrackeditor.h"

void QTrackEditor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    const qreal h = height();
    //QVector<QPointF>& points = track.points;

    // draw the line
    QPainterPath path;
    track.get_path(path, h);
    painter.drawPath(path);

    // draw the signs
    for (int i = 0; i < track.signs.size(); i++) {
        track.signs[i].draw(painter, path, true);
    }

    // Draw the control points
    if (show_control_points->value() > 0) {
        const bool main_points_only = show_control_points->value() < 2;
        const qreal point_size = this->point_size;
        painter.setPen(QColor(50, 100, 120, 200));
        for (int i=0; i<=track.num_points*3; ++i) {
            QPointF pos = Track::tf(track.points[i],h);
            //if (i % 3)
                painter.setBrush(QColor(200, 200, 210, (i%3) ? 30 : 120));
            //else
                //painter.setBrush(QColor(160, 160, 160, 120));

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


void QTrackEditor::mousePressEvent(QMouseEvent *e)
{
    const qreal h = height();
    const QPointF mp = e->pos();

    // first check the signs
    sign_moving = -1;
    QPainterPath path;
    track.get_path(path, h);
    QRectF pos, pole_pos;
    for (int i = 0; i < track.signs.size(); i++) {
        if (track.signs[i].get_position(pos, pole_pos, path) && pos.contains(mp)) {
            sign_moving = i;
            selected_traffic_light = (track.signs[i].type == Track::Sign::TrafficLight) ? &track.signs[i] : nullptr;
            if (selected_traffic_light) {
                Track::Sign::TrafficLightInfo& info = selected_traffic_light->traffic_light_info;
                tl_min_time->setValue(info.time_range.first);
                tl_max_time->setValue(info.time_range.second);
                tl_distance->setValue(info.trigger_distance);
            }
            for (auto w : tl_widgets)
                w->setVisible(!!selected_traffic_light);
            break;
        }
    }
    if (sign_moving >= 0) {
        if (e->button() == Qt::RightButton) {
            QMenu menu(this);
            const QString del("Delete");
            menu.addAction(del);
            for (auto& i : track.images.sign_images)
                menu.addAction(i.name);
            QAction* a = menu.exec(e->globalPos());
            if (a) {
                QString text = a->text();
                if (text == del)
                    track.signs.remove(sign_moving);
                else {
                    for (int i = 0; i < track.images.sign_images.size(); i++) {
                        if (text == track.images.sign_images[i].name) {
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
void QTrackEditor::mouseMoveEvent(QMouseEvent *e) {
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
