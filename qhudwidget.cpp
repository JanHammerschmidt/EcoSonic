#include "qhudwidget.h"
#include "hud.h"


QHudWidget::QHudWidget(QWidget *parent) :
    QWidget(parent)
{
    setAutoFillBackground(true);
    QPalette p = palette();
    p.setColor(backgroundRole(), Qt::white);
    setPalette(p);

}

void QHudWidget::update_hud(HUD* hud, const qreal rpm, const qreal kmh, const qreal liters_used) {
    this->hud = hud;
    this->rpm = rpm;
    this->kmh = kmh;
    this->liters_used = liters_used;
    update(); // !! only rect ..
}

void QHudWidget::draw(QPainter& painter) {
    if (!hud)
        return;
    hud->draw(painter, width(), rpm, kmh, liters_used);
}

void QHudWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    draw(painter);
}