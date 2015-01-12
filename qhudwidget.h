#ifndef QHUDWIDGET_H
#define QHUDWIDGET_H

#include <QWidget>

struct HUD;

class QHudWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QHudWidget(QWidget *parent = 0);

    void update_hud(HUD* hud, const qreal rpm, const qreal kmh, const qreal liters_used);

private:
    void draw(QPainter& painter);

    virtual void paintEvent(QPaintEvent *) override;
    HUD* hud = nullptr;
    qreal rpm, kmh, liters_used;


signals:

public slots:

};

#endif // QHUDWIDGET_H
