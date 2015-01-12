#ifndef HUD_H
#define HUD_H

#include <type_traits>
#include <QTextStream>
#include <QTime>
#include <QPainter>
#include <math.h>

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

struct ConsumptionDisplay {
    ConsumptionDisplay(QString legend1 = "L/", QString legend2 = "100km", const char* const number_format = "%04.1f")
        : number_format(number_format)
    {
        legend[0] = legend1;
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

struct TimeDisplay {
    static void draw(QPainter& painter, QPointF pos, int max_time, qreal time_elapsed, bool track_started, bool draw_remaining = true)
    {
        Q_ASSERT(draw_remaining == true);
        if (track_started || draw_remaining) {
            QTime t(0,0);
            t = t.addSecs(std::max(0.0, track_started ? max_time - time_elapsed : max_time));
            QString ts = t.toString("m:ss");
            painter.setFont(QFont{"Eurostile", 18, QFont::Bold});
            painter.setPen(QPen(Qt::black));
            painter.drawText(pos, "Remaining Time: " + ts);
        }
    }
};

struct HUD {
    ConsumptionDisplay consumption_display;
    ConsumptionDisplay trip_consumption{"dl/", "Trip", "%04.2f"};
    Speedometer speedometer;
    RevCounter rev_counter;
    TimeDisplay time_display;
    qreal l_100km = 0;
    bool draw_consumption = true;
    void draw(QPainter& painter, const qreal width, const qreal rpm, const qreal kmh, const qreal liters_used) { //, int max_time, qreal time_elapsed, bool track_started) {
        const qreal gap = 15;
        const qreal mid = width / 2.;
        rev_counter.draw(painter, QPointF(mid - rev_counter.radius - gap, rev_counter.radius + gap), 0.01 * rpm);
        speedometer.draw(painter, QPointF(mid + speedometer.radius + gap, speedometer.radius + gap), kmh);
        consumption_display.draw(painter, QPointF(mid, 180), l_100km, kmh >= 5);
        if (draw_consumption) {
            trip_consumption.draw(painter, QPointF(mid, 25), liters_used * 10);
        }
    }
    const qreal height = 230;
};

#endif
