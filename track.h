#ifndef TRACK_H
#define TRACK_H

#include <QPainterPath>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

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
            __length
        };
        enum TrafficLightState {
            Red = 0,
            Yellow,
            Green
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
        bool draw(QPainter& painter, QPainterPath& path) {
            QRectF pos, pole_pos;
            if (!get_position(pos, pole_pos, path))
                return false;
            images.pole_image.render(&painter, pole_pos);
            Track::SignImage& img = type == TrafficLight ? images.traffic_light_images[traffic_light_state]
                                                 : images.sign_images[type];
            img.is_svg ? img.svg->render(&painter, pos) : painter.drawImage(pos, *img.img);
            return true;
        }

        Type type = Stop;
        qreal at_length = 0;
        TrafficLightState traffic_light_state = Red;
    };
    void sort_signs() {
        std::sort(signs.begin(), signs.end());
    }

    QVector<QPointF> points;
    int num_points = 0;
    int width = 1000;
    QVector<Sign> signs;
    struct Images {
        QVector<SignImage> sign_images;
        QSvgRenderer pole_image;
        QSize pole_size;
        SignImage traffic_light_images[3];
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
            traffic_light_images[Sign::Yellow].load("media/signs/traffic_yellow.svg", "", 0.07);
            traffic_light_images[Sign::Green].load("media/signs/traffic_green.svg", "", 0.07);
            sign_images[Sign::TrafficLight].name = "Traffic Light";
        }
    };
    static Images images;

    inline bool save(const QString filename = QDir::homePath()+"/track.bin");
    inline bool load(const QString filename = QDir::homePath()+"/track.bin");
    void get_path(QPainterPath& path, const qreal height) {
        const qreal h = height;
        path.moveTo(tf(points[0],h));
        for (int i = 0; i < num_points; i++) {
            const int j = i*3+1;
            path.cubicTo(tf(points[j],h), tf(points[j+1],h), tf(points[j+2],h));
        }
    }
    void get_path_points(QPainterPath& path, const qreal height) { // const bool main_points_only = false
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
    out << track.points << track.num_points << track.width << track.signs;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Track &track) {
    in >> track.points >> track.num_points >> track.width >> track.signs;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const Track::Sign &sign) {
    out << (int) sign.type << sign.at_length;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, Track::Sign &sign) {
    int type;
    in >> type >> sign.at_length;
    sign.type = (Track::Sign::Type) type;
    return in;
}


bool Track::save(const QString filename) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QDataStream out(&file);
    out << *this;
    return true;
}
bool Track::load(const QString filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QDataStream in(&file);
    in >> *this;
    return true;
}

#endif // TRACK_H
