#ifndef KEYBOARDINPUT_H
#define KEYBOARDINPUT_H

struct KeyboardInput : public QObject
{
    void init(QMainWindow* main_window) {
        main_window->installEventFilter(this);
    }

    double throttle() { return keys_pressed.contains(Qt::Key_Up) ? 1 : 0; }
    double breaking() { return keys_pressed.contains(Qt::Key_Down) ? 1 : 0; }
    bool vol_up() { return keys_pressed.contains(Qt::Key_Plus) ? true : false; }
    bool vol_down() { return keys_pressed.contains(Qt::Key_Minus) ? true : false; }
    bool steer_right() { return keys_pressed.contains(Qt::Key_Left); }
    bool steer_left() { return keys_pressed.contains(Qt::Key_Right); }

    int gear_change() {
        const int ret = gears;
        gears = 0;
        return ret;
    }
    bool toggle_clutch() {
        const bool ret = clutch;
        clutch = false;
        return ret;
    }
    bool show_arrow() {
        const bool ret = t_arrow;
        t_arrow = false;
        return ret;
    }

    bool show_control_window() {
        const bool ret = control_window;
        control_window = false;
        return ret;
    }
    bool pitch_toggle() {
        const bool ret = t_pitch_toggle;
        t_pitch_toggle = false;
        return ret;
    }

    bool connect() {
        const bool ret = t_connect;
        t_connect = false;
        return ret;
    }

    int get_sound_modus() { return sound_modus; }

    bool update() {
        const bool ret = changed;
        changed = false;
        return ret;
    }


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
                case Qt::Key_Space: clutch = true; break;
                case Qt::Key_1: sound_modus = 1; break;
                case Qt::Key_2: sound_modus = 2; break;
                case Qt::Key_3: sound_modus = 3; break;
                case Qt::Key_0: sound_modus = 0; break;
                case Qt::Key_T: control_window = true; break;
                case Qt::Key_U: t_pitch_toggle = true; break;
                case Qt::Key_D: t_arrow = true; break;
                case Qt::Key_C: t_connect = true; break;
                default: keys_pressed.insert(key);
            }
        }
        else if (e->type() == QEvent::KeyRelease) {
            keys_pressed.remove(key);
        }
        return false;
    }
public:
    QSet<int> keys_pressed;
protected:
    bool changed = false;
    int gears = 0;
    bool clutch = false;
    int sound_modus = 0;
    bool control_window = false;
    bool t_pitch_toggle = false;
    bool t_arrow = false;
    bool t_connect = false;
};

#endif // KEYBOARDINPUT_H
