#pragma once

#include <QObject>

#ifdef WIN32
#include <windows.h>
#include "eyex/EyeX.h"
#else
#include <QTimer>
#include <random>
#endif


struct TobiiTracker : public QObject
{

    Q_OBJECT

signals:

    void gaze_event(double x, double y);

public:

#ifndef WIN32

    TobiiTracker() {
        connect(&timer, &QTimer::timeout, this, &TobiiTracker::timer_timeout);
    }

    bool StartTracker() {
        timer.start();
        return false;
    }
    bool StopTracker() { return false; }

    QTimer timer;

protected slots:

    void timer_timeout() {
        static std::mt19937_64 rng(std::random_device{}());
        static std::uniform_real_distribution<double> xy(0, 500);
        emit gaze_event(xy(rng), xy(rng));
    }

#else

public:

    TobiiTracker();

    /*
     * Initializes g_hGlobalInteractorSnapshot with an interactor that has the Gaze Point behavior.
     */
    BOOL InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext);

    /*
     * Callback function invoked when a snapshot has been committed.
     */
    static void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param);


    /*
     * Callback function invoked when the status of the connection to the EyeX Engine has changed.
     */
    static void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam);


    /*
     * Handles an event from the Gaze Point data stream.
     */
    void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior);


    /*
     * Callback function invoked when an event has been received from the EyeX Engine.
     */
    static void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam);


    bool StartTracker();

    bool StopTracker();

    TX_CONTEXTHANDLE hContext = TX_EMPTY_HANDLE;


#endif // WIN32

};

