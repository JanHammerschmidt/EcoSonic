#include "stdafx.h"
#include "hudwindow.h"
#include "ui_hudwindow.h"

HUDWindow::HUDWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HUDWindow)
{
    ui->setupUi(this);
}

HUDWindow::~HUDWindow()
{
    delete ui;
}

QHudWidget& HUDWindow::hud_widget() {
    return *ui->hud_widget;
}
