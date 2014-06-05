#include "mainwindow.h"
#include <QApplication>
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
    MainWindow w;
    w.show();

    return a.exec();
}
