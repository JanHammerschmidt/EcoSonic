#ifndef FEDI_VOLUME_H
#define FEDI_VOLUME_H

#include <QDialog>

class QCarViz;

namespace Ui {
class FediVolume;
}

class FediVolume : public QDialog
{
    Q_OBJECT

public:
    explicit FediVolume(QWidget *parent = 0);
    ~FediVolume();

    void init(QCarViz* car_viz) { car_viz_ = car_viz; }
    void start();
    static void main_loop(FediVolume* fedi_volume);

private:
    Ui::FediVolume *ui;
    QCarViz* car_viz_ = nullptr;
    bool running_ = false;
};

#endif // FEDI_VOLUME_H
