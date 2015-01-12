#ifndef HUDWINDOW_H
#define HUDWINDOW_H

#include <QMainWindow>
#include "qhudwidget.h"

namespace Ui {
class HUDWindow;
}

class HUDWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit HUDWindow(QWidget *parent = 0);
    ~HUDWindow();

    QHudWidget& hud_widget();

private:

    Ui::HUDWindow *ui;
};

#endif // HUDWINDOW_H
