#include "fedi_volume.h"
#include "ui_fedi_volume.h"
#include "qcarviz.h"
#include <QtConcurrent>

FediVolume::FediVolume(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FediVolume)
{
    ui->setupUi(this);
}

FediVolume::~FediVolume()
{
    delete ui;
}

void FediVolume::start()
{
    running_ = true;
    QtConcurrent::run(main_loop, this);
}

void FediVolume::main_loop(FediVolume* fedi_volume)
{
    for ( ; ; )
    {
        for ( ; ; )
        {
            if (!fedi_volume->running_)
                return;
        }
    }
}
