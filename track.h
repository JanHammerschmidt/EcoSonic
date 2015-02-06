#ifndef TRACK_H
#define TRACK_H

#include <QPainterPath>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <hud.h>
#include "misc.h"

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

struct Track {
    struct SignImage {
        bool load(const QString filename, const QString name, const qreal scale) {
            is_svg = filename.endsWith("svg", Qt::CaseInsensitive);
            if (is_svg)
                svg = new QSvgRenderer(filename);
            else
                img = new QImage(filename);
            if (is_svg ? !svg->isValid() : img->isNull())
                return false;
            this->name = name;
            size = is_svg ? svg->defaultSize() : img->size();
            size *= scale;
            return true;
        }

        QImage* img = NULL;
        QSvgRenderer* svg = NULL;
        bool is_svg = false;
        QString name;
        QSizeF size;
    };

    struct Sign {
        enum Type {
            Stop = 0,
            Speed30,
            Speed40,
            Speed50,
            Speed60,
            Speed70,
            Speed80,
            Speed90,
            Speed100,
            Speed110,
            Speed120,
            Speed130,
            TrafficLight,
            TurnLeft,
            TurnRight,
            __length
        };
        enum TrafficLightState {
            Red = 0,
            Red_pending,
            Yellow,
            Green,
            __State_length
        };
        struct TrafficLightInfo {
            qreal trigger_distance = 500; //200;
            std::pair<qreal,qreal> time_range = {7000,7000}; //{3000,3000}; //{0,3000};
        };
        struct SteeringInfo {
            qreal intensity = 0.5; // steering "power" (intensity * dt)
            qreal duration = 1.2; // how long (on given intensity)
            qreal fade_in = 0.2; // how long to "fade in" to the given intensity
            qreal fade_out = 0.2; // same for fading out
        //private:
            bool left = false; // otherwise to the right! ;) [determined by the sign itself!]
        };
        Sign() {}
        Sign(const Type type, const qreal at_length) : type(type), at_length(at_length) {}
        bool operator<(const Sign& s2) const { return at_length < s2.at_length; }
        bool get_position(QRectF& pos, QRectF& pole_pos, QPainterPath& path) {
            if (at_length > path.length())
                return false;
            qreal const percent = path.percentAtLength(at_length);
            const QPointF p = path.pointAtPercent(percent);
            QSizeF& size = (type == TrafficLight ? images.traffic_light_images[traffic_light_state]
                                                 : images.sign_images[type]).size;
            pos = QRectF(p.x() - size.width()/2, p.y() - size.height() - images.pole_size.height(),
                          size.width(), size.height());
            pole_pos = QRectF(p.x() - 0.5 * images.pole_size.width(), p.y() - images.pole_size.height(), images.pole_size.width(), images.pole_size.height());
            return true;
        }
        bool draw(QPainter& painter, QPainterPath& path, bool editor = false) {
            if (!editor && (type == TurnLeft || type == TurnRight))
                return false;
            QRectF pos, pole_pos;
            if (!get_position(pos, pole_pos, path))
                return false;
            images.pole_image.render(&painter, pole_pos);
            Track::SignImage& img = type == TrafficLight ? images.traffic_light_images[traffic_light_state]
                                                 : images.sign_images[type];
            img.is_svg ? img.svg->render(&painter, pos) : painter.drawImage(pos, *img.img);
            if (editor && type == TrafficLight) {
                // draw time range
                QFont font("Eurostile", 12);
                painter.setFont(font);
                QFontMetrics fm = painter.fontMetrics();
                QString s;
                QPointF p((pos.left() + pos.right()) / 2, pos.top() - 1.1 * fm.height());
                s.sprintf("%02.1f", traffic_light_info.time_range.first / 1000.);
                misc::draw_centered_text(painter, fm, s, p);
                s.sprintf("%02.1f", traffic_light_info.time_range.second / 1000.);
                p.setY(p.y() - 1.1 * fm.height());
                misc::draw_centered_text(painter, fm, s, p);
                // draw trigger distance
                qreal const percent = path.percentAtLength(at_length - traffic_light_info.trigger_distance);
                p = path.pointAtPercent(percent);
                painter.drawLine(p + QPointF(0, 5), p - QPointF(0, 10));
            }
            if (editor && is_turn_sign()) {
                QFont font("Eurostile", 12);
                painter.setFont(font);
                QFontMetrics fm = painter.fontMetrics();
                QString s;
                QPointF p((pos.left() + pos.right()) / 2, pos.top() - 1.1 * fm.height());
                for (auto i : {steering_info.fade_out, steering_info.fade_in, steering_info.duration, steering_info.intensity}) {
                    s.sprintf("%02.1f", i);
                    misc::draw_centered_text(painter, fm, s, p);
                    p.setY(p.y() - 1.1 * fm.height());
                }
            }
            return true;
        }
        bool is_turn_sign() const { return type == TurnLeft || type == TurnRight; }
        bool is_speed_sign() const { return type >= Speed30 && type <= Speed130; }
        qreal speed_limit() const { Q_ASSERT(is_speed_sign()); return 30 + (type - Speed30) * 10; }

        Type type = Stop;
        qreal at_length = 0;
        TrafficLightState traffic_light_state = Red;
        TrafficLightInfo traffic_light_info;
        SteeringInfo steering_info;
    };

    void check_traffic_light_distance(Sign& s, qreal prev_pos = 0, const qreal min_dist = 100) {
        qreal& dist = s.traffic_light_info.trigger_distance;
        if (s.at_length - prev_pos - 50 < dist)
            dist = s.at_length - prev_pos - 50;
        if (dist < min_dist)
            dist = min_dist;
        // also check time range
        std::pair<qreal,qreal>& tr = s.traffic_light_info.time_range;
        if (tr.first > tr.second)
            tr.first = tr.second;
    }

    void prepare_track() {
        sort_signs();
        Sign* prev_traffic_light = NULL;
        for (Sign& s : signs) {
            if (s.type == Sign::TrafficLight) {
                check_traffic_light_distance(s, prev_traffic_light ? prev_traffic_light->at_length : 0);
                prev_traffic_light = &s;
            }
        }
    }

    void sort_signs() {
        std::sort(signs.begin(), signs.end());
    }

    QVector<QPointF> points;
    int num_points = 0;
    int width = 1000;
    QVector<Sign> signs;
    int max_time = 0; // how much time the user has to finish the track

    struct Images {
        QVector<SignImage> sign_images;
        QSvgRenderer pole_image;
        QSize pole_size;
        SignImage traffic_light_images[Sign::__State_length];
        void load_sign_images() {
            pole_image.load(QString("media/signs/pole.svg"));
            pole_size = pole_image.defaultSize() * 1;
            sign_images.resize(Sign::__length);
            sign_images[Sign::Stop].load("media/signs/stop-sign.svg", "Stop", 0.02);
            for (int i = 0; i < 11; i++) {
                QString path; path.sprintf("media/signs/%isign.svg", 30+i*10);
                QString name; name.sprintf("Speed: %i", 30+i*10);
                sign_images[Sign::Speed30 + i].load(path, name, 0.05);
            }
            traffic_light_images[Sign::Red].load("media/signs/traffic_red.svg", "", 0.07);
            traffic_light_images[Sign::Red_pending].load("media/signs/traffic_red.svg", "", 0.07);
            traffic_light_images[Sign::Yellow].load("media/signs/traffic_yellow.svg", "", 0.07);
            traffic_light_images[Sign::Green].load("media/signs/traffic_green.svg", "", 0.07);
            sign_images[Sign::TrafficLight].name = "Traffic Light";
            //printf("%i %i\n", Sign::Type::TurnRight, sign_images.size());
            sign_images[Sign::TurnRight].load("media/signs/turn_right.svg", "right turn", 0.03);
            sign_images[Sign::TurnLeft].load("media/signs/turn_left.svg", "left turn", 0.03);
        }
    };
    static Images images;

    inline bool save(const QString filename = QDir::homePath()+"/EcoSonic/track.bin") const { return misc::saveObj(filename, *this); }
    inline bool load(const QString filename = QDir::homePath()+"/EcoSonic/track.bin") { return misc::loadObj(filename, *this); }
    void get_path(QPainterPath& path, const qreal height) {
        if (!points.size())
            return;
        const qreal h = height;
        path.moveTo(tf(points[0],h));
        for (int i = 0; i < num_points; i++) {
            const int j = i*3+1;
            path.cubicTo(tf(points[j],h), tf(points[j+1],h), tf(points[j+2],h));
        }
    }
    void get_path_points(QPainterPath& path, const qreal height) { // const bool main_points_only = false
        if (!points.size())
            return;
        const qreal h = height;
        path.moveTo(tf(points[0],h));
        for (int i = 1; i <= num_points*3; i++) {
//            if (main_points_only)
//                i += 2;
            path.lineTo(tf(points[i],h));
        }
    }

    //transform
    static inline QPointF tf(const QPointF& p, const qreal height) {
        return QPointF(p.x(), height - p.y());
    }
    // inverse transform
    static inline QPointF tf_1(const QPointF& p, const qreal height) {
        return tf(p, height);
    }
};

inline QDataStream &operator<<(QDataStream &out, const Track &track) {
    out << track.points << track.num_points << track.width << track.signs << track.max_time;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Track &track) {
    in >> track.points >> track.num_points >> track.width >> track.signs >> track.max_time;
    track.prepare_track();
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const Track::Sign::TrafficLightInfo &tl_info) {
    out << tl_info.time_range.first << tl_info.time_range.second << tl_info.trigger_distance;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Track::Sign::TrafficLightInfo &tl_info) {
    in >> tl_info.time_range.first >> tl_info.time_range.second >> tl_info.trigger_distance;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const Track::Sign::SteeringInfo &s_info) {
    out << s_info.duration << s_info.intensity << s_info.fade_in << s_info.fade_out << s_info.left;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Track::Sign::SteeringInfo &s_info) {
    in >> s_info.duration >> s_info.intensity >> s_info.fade_in >> s_info.fade_out >> s_info.left;
    return in;
}



inline QDataStream &operator<<(QDataStream &out, const Track::Sign &sign) {
    out << (int) sign.type << sign.at_length << sign.traffic_light_info << sign.steering_info;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Track::Sign &sign) {
    int type;
    in >> type >> sign.at_length >> sign.traffic_light_info >> sign.steering_info;
    sign.type = (Track::Sign::Type) type;
    return in;
}

#endif // TRACK_H
