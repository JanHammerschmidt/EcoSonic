#ifndef KEYBOARDINPUT_H
#define KEYBOARDINPUT_H

struct KeyboardInput : public QObject
{
    void init(QMainWindow* main_window) {
        main_window->installEventFilter(this);
    }

    bool is_key_down(Qt::Key key) {
        return keys_down.contains(key);
    }

    bool get_key_press(Qt::Key key) {
        if (keys_pressed.contains(key)) {
            keys_pressed.remove(key);
            return true;
        }
        return false;
    }

    double throttle() { return is_key_down(Qt::Key_Up) ? 1 : 0; }
    double breaking() { return is_key_down(Qt::Key_Down) ? 1 : 0; }
    bool vol_up() { return is_key_down(Qt::Key_Plus) ? true : false; }
    bool vol_down() { return is_key_down(Qt::Key_Minus) ? true : false; }
    bool steer_right() { return is_key_down(Qt::Key_Left); }
    bool steer_left() { return is_key_down(Qt::Key_Right); }

    int gear_change() {
        const int ret = gears;
        gears = 0;
        return ret;
    }

    bool toggle_clutch() { return get_key_press(Qt::Key_Space); }
    bool show_arrow() { return get_key_press(Qt::Key_D); }
    bool show_control_window() { return get_key_press(Qt::Key_T); }
    bool pitch_toggle() { return get_key_press(Qt::Key_U); }
    bool connect() { return get_key_press(Qt::Key_C); }

    bool update() {
        const bool ret = changed;
        changed = false;
        return ret;
    }

    bool toggle_show_eye_tracking_point() { return get_key_press(Qt::Key_E); }


protected:
    virtual bool eventFilter(QObject *obj, QEvent *e) {
        Q_UNUSED(obj);
//        if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) {
//            QKeyEvent *key = static_cast<QKeyEvent *>(e);
//            printf("%s: %d\n", (e->type() == QEvent::KeyPress) ? "down" : "up", key->key());
//        }
        int const key = static_cast<QKeyEvent *>(e)->key();
        if (key == Qt::Key_Up || key == Qt::Key_Down)
            changed = true;

        if (e->type() == QEvent::KeyPress) {
            switch (key) {
                case Qt::Key_W: gears += 1; break;
                case Qt::Key_S: gears -= 1; break;
                default: keys_pressed.insert(key); keys_down.insert(key);
            }
        }
        else if (e->type() == QEvent::KeyRelease) {
            keys_down.remove(key);
        }
        return false;
    }
public:


protected:
    QSet<int> keys_down;
    QSet<int> keys_pressed;
    bool changed = false;
    int gears = 0;
};

#endif // KEYBOARDINPUT_H
