#ifndef KEYBOARDINPUT_H
#define KEYBOARDINPUT_H

struct KeyboardInput : public QObject
{
    void init(QMainWindow* main_window) {
        main_window->installEventFilter(this);
    }

    double throttle() { return keys_pressed.contains(Qt::Key_Up) ? 1 : 0; }
    double breaking() { return keys_pressed.contains(Qt::Key_Down) ? 1 : 0; }
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
                default: keys_pressed.insert(key);
            }
        }
        else if (e->type() == QEvent::KeyRelease) {
            keys_pressed.remove(key);
        }
        return false;
    }
    QSet<int> keys_pressed;
    bool changed = false;
    int gears = 0;
    bool clutch = false;
};

#endif // KEYBOARDINPUT_H
