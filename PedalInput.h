#ifndef PEDALINPUT_H
#define PEDALINPUT_H

#include <HID.h>

// WingMan Formula GP
// 9: geht bei gas von 127=>0, bremse: 127=>255
// 10&11: bei gas: 255=>0, bremse: 255=>65535
class PedalInput2
{
public:
    PedalInput2() {
        const std::string wingman = "WingMan Formula GP";
        hid.ScanDevices();
        int c = hid.GetDeviceCount();
        for (int i = 0; i < c; i++) {
            IHIDevice* d = hid.GetDevice(i);
            const std::string name = d->GetDeviceName();
            if (name.find(wingman) != std::string::npos) {
                dev = d;
                break;
            }
        }
        printf("%s %s\n", wingman.c_str(), dev ? "found" : "not found");
        if (dev) {
            element = dev->GetElement(10);
        }
    }

    bool update() {
        const int old_value = value;
        value = element->GetValue();
        return value != old_value;
    }
    double gas() { return value < 255 ? (255-value)/255. : 0; }
    double brake() { return value > 255 ? (value-255)/65280. : 0; }

    bool valid() {
        return !!dev && !!element;
    }
protected:
    HIDManager hid;
    IHIDevice* dev = NULL;
    IHIDElement* element;
    int value = 255;
};

#endif // PEDALINPUT_H
