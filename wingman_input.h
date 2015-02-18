#ifndef WINGMAN_INPUT_H
#define WINGMAN_INPUT_H

#ifdef NO_HID
struct WingmanInput
{
	bool update() { return false; }
	double gas() { return 0; }
	double brake() { return 0; }
	bool update_buttons() { return false; }
	bool left_click() { return false; }
	bool right_click() { return false; }
	bool update_wheel() { return false; }
	bool steering_left() { return false; }
	bool steering_right() { return false; }
	bool wheel_neutral() { return false; }
	double wheel() { return 0; }


	bool valid() { return false; }
};
#else
#include <HID.h>

// WingMan Formula GP
// 9: geht bei gas von 127=>0, bremse: 127=>255
// 10&11: bei gas: 255=>0, bremse: 255=>65535
// 8: lenkrad von links (0) bis rechts (255)
//4-7: Knöpfe (0 / 1), links hinten, rechts hinten, links vorne, rechts vorne
class WingmanInput
{
public:
    WingmanInput() {
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
            wheel_element = dev->GetElement(8);
            left_button_element = dev->GetElement(6);
            right_button_element = dev->GetElement(7);
            left_back_button_element = dev->GetElement(4);
            right_back_button_element = dev->GetElement(5);
        }
    }

    bool update() {
        const int old_value = value;
        value = element->GetValue();
        return value != old_value;
    }
    bool update_wheel() {
        const int old_value = wheel_value;
        wheel_value = wheel_element->GetValue();
//        if (wheel_value != old_value)
//            printf("wheel: %i\n", wheel_value);
        return wheel_value != old_value;
    }
    bool update_buttons() {
        bool update = false;
        bool old_val = left_button_cur;
        left_button_cur = left_button_element->GetValue();
        if (left_button_cur && old_val != left_button_cur) {
            left_button_click = true;
            update = true;
        }
        old_val = right_button_cur;
        right_button_cur = right_button_element->GetValue();
        if (right_button_cur && old_val != right_button_cur) {
            right_button_click = true;
            update = true;
        }
        return update;
    }

    bool update_back_buttons() {
        bool update = false;
        bool old_val = left_right_cur;
        left_right_cur = left_back_button_element->GetValue() && right_back_button_element->GetValue();
        if (left_right_cur && old_val != left_right_cur) {
            left_right_click_ = true;
            update = true;
        }
        return update;
    }

    const int neutral_range = 7;
    double gas() { return value < 255 ? (255-value)/255. : 0; }
    double brake() { return value > 255 ? (value-255)/65280. : 0; }
    double wheel() {
        /*printf("%.3f\n", (wheel_value - 128) / 128.);*/
        int val = wheel_value - 128;
        if (abs(val) <= neutral_range)
            return 0;
        //val += (val > 0) ? -neutral_range : neutral_range;
        //qDebug() << val << (double) (val) / (128-neutral_range);
        return pow((double) (val) / (128-neutral_range), 3);
    }
    bool wheel_neutral() { /*qDebug() << wheel_value; */return wheel_value >= 121 && wheel_value <= 135; }
//    bool steering_left() { return wheel() < -0.5; }
//    bool steering_right() { return wheel() > 0.5; }
    bool left_click() {
        const bool ret = left_button_click;
        left_button_click = false;
        return ret;
    }
    bool right_click() {
        const bool ret = right_button_click;
        right_button_click = false;
        return ret;
    }
    bool left_right_click() {
        const bool ret = left_right_click_;
        left_right_click_ = false;
        return ret;
    }

    bool valid() {
        return !!dev && !!element && !!wheel_element;
    }
protected:
    HIDManager hid;
    IHIDevice* dev = NULL;
    IHIDElement* element; // this is the element for gas & braking
    IHIDElement* wheel_element;
    IHIDElement* left_button_element;
    IHIDElement* right_button_element;    
    IHIDElement* left_back_button_element;
    IHIDElement* right_back_button_element;
    int value = 255; // this is for gas & braking
    int wheel_value = 128;
    bool left_button_cur = false;
    bool right_button_cur = false;
    bool left_button_click = false;
    bool right_button_click = false;
    bool left_right_cur = false;
    bool left_right_click_ = false;
};

#endif // NO_HID

#endif // WINGMAN_INPUT_H
