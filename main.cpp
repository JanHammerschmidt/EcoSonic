#include "stdafx.h"
#include "mainwindow.h"
#include "hudwindow.h"
#include <QApplication>
#include <QDesktopWidget>
//#include <HID.h>
//#include <PedalInput.h>
//#include <gazeapi.h>

//QMainWindow* window = nullptr;
//QRect r2;

//struct Gaze_api : public gtl::GazeApi {
//    Gaze_api(bool verbose = false)
//        : gtl::GazeApi(verbose)
//    { }

//    template<class T> void add_listener(T& listener) {
//        if (!connected) {
//            // Connect to the server in push mode on the default TCP port (6555)
//            if(connect(true)) {
//                printf("connected\n");
//                connected = true;
//                gtl::ServerState state = get_server_state();
//                printf("server state: %i \n", *reinterpret_cast<int*>(&state));
//            } else {
//                printf("failed to connect to server!\n");
//                return;
//            }
//        }
//        gtl::GazeApi::add_listener(listener);
//    }

//    virtual ~Gaze_api() {
//        if (connected) {
//            disconnect();
//            printf("disconnected\n");
//        }
//        connected = false;
//    }

//    bool connected = false;
//};

//Gaze_api gaze_api;

//// --- MyGaze definition
//class MyGaze : public gtl::IGazeListener
//{
//    public:
//        explicit MyGaze(Gaze_api& gaze_api)
//            : gaze_api(gaze_api)
//        {
//            // Enable GazeData notifications
//            gaze_api.add_listener( *this );
//            printf("gaze listener added\n");
//        }

//        ~MyGaze() {
//            //gaze_api.remove_listener( *this );
//        }

//    private:
//        // IGazeListener
//        void on_gaze_data( gtl::GazeData const & gaze_data ) {
//            //printf("gaze: %i ", gaze_data.state);
//            if( gaze_data.state & gtl::GazeData::GD_STATE_TRACKING_GAZE )
//            {
//                window->move(r2.topLeft() + QPoint(gaze_data.avg.x, gaze_data.avg.y) - QPoint(window->width() / 2, window->height() / 2));
//                //printf("gaze! ");
//            }
//        }
//    Gaze_api& gaze_api;
//};

////class MyStateListener : public gtl::IConnectionStateListener {
////public:
////    MyStateListener() {
////        if( m_api.connect( true ) )
////        {
////            printf("state changer connected\n");
////            m_api.add_listener( *this );
////        }
////    }

////    virtual void on_connection_state_changed( bool is_connected ) override {
////        printf("%s\n", is_connected ? "connected" : "disconnected");
////    }
////    gtl::GazeApi m_api;
////};

//class MyStateListener : public gtl::ITrackerStateListener
//{
//public:
//    explicit MyStateListener(Gaze_api& gaze_api)
//        : gaze_api(gaze_api)
//    {
//        gaze_api.add_listener( *this );
//        printf("state listener added\n");
//    }
//    virtual ~MyStateListener() {
////        if (gaze_api.connected)
////            gaze_api.remove_listener(*this);
//    }

//    virtual void on_tracker_connection_changed( int tracker_state ) override {
//        printf("tracker state changed: %i\n", tracker_state);
//    }

//    /** A notification call back indicating that main screen index has changed.
//     *  This is only relevant for multiscreen setups. Implementing classes should
//     *  update themselves accordingly if needed.
//     *  Register for updates through GazeApi::add_listener(ITrackerStateListener & listener).
//     *
//     *  \param[in] screen the new screen state
//     */
//    virtual void on_screen_state_changed( gtl::Screen const & ) override {
//        printf("screen state changed\n");
//    }
//protected:
//    Gaze_api& gaze_api;

//};

//#include <boost/algorithm/clamp.hpp>

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

//    QDesktopWidget d;
//    int c = d.screenCount();
//    if (c == 2) {
//        r2 = d.screenGeometry(1);
//    }

//    QMainWindow w;
//    window = &w;
//    w.show();
//    MyGaze g(gaze_api);
//    MyStateListener sl(gaze_api);
//    return a.exec();

#ifdef WIN32
	QDir::setCurrent("C:/d/Dropbox/Eigene Dateien/phd/code/car_simulator");
#else
    QDir::setCurrent("/Users/jhammers/Dropbox/Eigene Dateien/phd/code/car_simulator/");
#endif

    MainWindow w;
    //HUDWindow hw;

    QDesktopWidget d;
    int c = d.screenCount();
    if (c == 2) {
        //QRect r = d.availableGeometry();
        //QRect r1 = d.availableGeometry(0);
        QRect r2 = d.availableGeometry(1);
        //w.move(r2.center() - QPoint(w.width() / 2, 0)); //, w.height() / 2));
        //hw.move(r1.center() - QPoint(hw.width() / 2, 0)); //, hw.height() / 2));
    }

    w.show();
    //hw.show();

    return a.exec();
}
