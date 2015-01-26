#include <QCoreApplication>
#include "TrackerServer.h"
#include "gaze.h"

TrackerServer server;
TobiiTracker tracker;

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

    QObject::connect(&tracker, &TobiiTracker::gaze_event, &server, &TrackerServer::gaze_event);

    qDebug() << "starting server...";
    server.listen(QHostAddress::Any, 7767);

    qDebug() << "starting tracker...";
    tracker.StartTracker();
    const int ret = a.exec();
    qDebug() << "shutdown";
    tracker.StopTracker();
    return ret;
}
