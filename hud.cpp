#include "stdafx.h"
#include "hud.h"

//const std::array<QFont, 3> ConsumptionDisplay::fonts = { { QFont{ "Eurostile", 18, QFont::Bold }, QFont{ "Eurostile", 12, QFont::Bold }, QFont{ "Eurostile", 8, QFont::Bold } } };
//const QFontMetrics ConsumptionDisplay::font_metrics[3] = { QFontMetrics(fonts[0]), QFontMetrics(fonts[1]), QFontMetrics(fonts[2]) };

void Speedometer::draw(QPainter& painter, const QPointF pos, const qreal kmh)
{
	// angle calculations
	const qreal start_angle = 0.5 * (M_PI + gap); // angle for min_speed (everything's on top!)
	const qreal angle_delta = (2 * M_PI - gap) / speed_steps;

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
		painter.drawLine(pos + (radius - 10) * vec, pos + radius * vec);
		QString str;
		QTextStream(&str) << min_speed + i * speed_delta;
		draw_centered_text(painter, fm, str, pos + (radius - 19 - fabs(cos(angle))*0.4*fm.width(str)) * vec);
	}

	// draw secondary ticks
	painter.setPen(QPen(Qt::black, 2));
	for (int i = 0; i < speed_steps; i++) {
		const qreal angle = start_angle + 0.5 * angle_delta + i * angle_delta;
		const QPointF vec(cos(angle), sin(angle));
		painter.drawLine(pos + (radius - 8) * vec, pos + radius * vec);
	}

	// draw caption
	painter.setFont(QFont("Arial", 12));
	fm = painter.fontMetrics();
	draw_centered_text(painter, fm, caption, pos + QPointF(0, -0.5 * radius));

	// draw needle
	//painter.setPen(QPen(QBrush(QColor(201,14,14)), 6));
	painter.setPen(QPen(Qt::black, 1));
	painter.setPen(Qt::NoPen);
	painter.setBrush(QBrush(QColor(201, 14, 14)));
	const qreal angle = start_angle + (kmh - min_speed) * angle_delta / speed_delta;
	const QPointF vec(cos(angle), sin(angle));
	const QPointF vecP(vec.y(), -vec.x()); // perpendicular vector
	const QPointF v1 = pos - 0.1*radius*vec;
	const QPointF v2 = pos + (radius - 30)*vec;
	const QPointF points[4] = { v1 - 4 * vecP, v1 + 4 * vecP, v2 + vecP, v2 - vecP };
	painter.drawConvexPolygon(points, 4);
	//painter.drawLine(pos-0.1*radius*vec, pos+(radius-20)*vec);
	//painter.setPen(QPen(Qt::black, 1));
	painter.setBrush(Qt::black);
	painter.drawEllipse(pos, 12, 12);

}

void ConsumptionDisplay::draw(QPainter& painter, QPointF pos, qreal consumption, bool draw_number)
{
	QString number; number.sprintf(number_format, consumption); //QString::number(consumption, 'f', 1);

	const qreal l2_x_offset = -3; // x-offset of 2nd part of legend
	const qreal l2_y_offset = 2; // y-offset of 2nd part of legend (100km)
	const qreal y_gap = 0; // gap between number and legend
	//const qreal border_x = 0;
	//const qreal border_y = 0; // border for the bounding rect
	const qreal rect_y_offset = 3;
	const QSizeF rect_size(45, 33);

	const qreal legend_height = font_heights[1] + std::max(l2_y_offset, 0.);
	const qreal height = font_heights[0] + y_gap + legend_height; // total height

	const qreal number_width = draw_number ? font_metrics[0].width(number) : 0;
	const int legend_widths[2] = { font_metrics[1].width(legend[0]), font_metrics[2].width(legend[1]) };
	const qreal legend_width = legend_widths[0] + legend_widths[1] + l2_x_offset;
	//const qreal width = std::max(number_width, legend_width); // total width

	painter.setBrush(QBrush(QColor(180, 180, 180)));
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
