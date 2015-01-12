#include "mainwindow.h"
#include "hudwindow.h"
#include <QApplication>
#include <QDesktopWidget>
//#include <HID.h>
//#include <PedalInput.h>

int main(int argc, char *argv[])
{
//    PedalInput pedal_input;
//    if (pedal_input.valid())
//        printf("found!\n");
//    else {
//        printf("not found!\n");
//        return 0;
//    }
//    double brake = 0, gas = 0;
//    while (true) {
//        pedal_input.update();
//        double const new_brake = pedal_input.brake();
//        double const new_gas = pedal_input.gas();
//        if (new_brake != brake) {
//            printf("brake: %.3f\n", new_brake);
//            brake = new_brake;
//        }
//        if (new_gas != gas) {
//            printf("gas: %.3f\n", new_gas);
//            gas = new_gas;
//        }
//        QThread::msleep(100);
//    }

//    return 0;
//    HIDManager hid;
//    hid.ScanDevices();
//    int c = hid.GetDeviceCount();
//    for (int i = 0; i < c; i++) {
//        IHIDevice* d = hid.GetDevice(i);
//        const std::string name = d->GetDeviceName();
//        printf("%i: %s\n", i, name.c_str());
//    }

//    IHIDevice* dev = hid.GetDevice(3);
//    c = dev->GetElementCount();
//    std::vector<int> state(c);
//    for (int& i : state) {
//        try {
//            i = dev->GetElement(i)->GetValue();
//        } catch (...)
//            {}
//    }

//    while (true) {
//        for (int i = 0; i < c; i++) {
//            if (i != 9)
//                continue;
//            int v;
//            try {
//                v = dev->GetElement(i)->GetValue();
//            } catch (...)
//                {}
//            if (v != state[i]) {
//                state[i] = v;
//                printf("%i: %i\n", i, v);
//            }
//        }
//        QThread::msleep(100);
//    }

//    return 0;

    QApplication a(argc, argv);
    QDir::setCurrent("../../../../car_simulator");

    MainWindow w;
    //HUDWindow hw;

    QDesktopWidget d;
    int c = d.screenCount();
    if (c == 2) {
        //QRect r = d.availableGeometry();
        //QRect r1 = d.availableGeometry(0);
        QRect r2 = d.availableGeometry(1);
        w.move(r2.center() - QPoint(w.width() / 2, 0)); //, w.height() / 2));
        //hw.move(r1.center() - QPoint(hw.width() / 2, 0)); //, hw.height() / 2));
    }

    w.show();
    //hw.show();

    return a.exec();
}
