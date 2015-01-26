#ifndef TRACKERSERVER_H
#define TRACKERSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include "gaze.h"


class TrackerServer : public QTcpServer
{
    Q_OBJECT

public:
    TrackerServer() : QTcpServer() {
        connect(&socket, SIGNAL(disconnected()), this, SLOT(disconnect()));
    }


protected:

    virtual void incomingConnection(qintptr socketDescriptor) override
    {
        qDebug() << "incoming connection";
        //pauseAccepting();
        socket.setSocketDescriptor(socketDescriptor);

        //QTcpServer::incomingConnection(socketDescriptor);
    }

protected slots:
    void disconnect() {
        qDebug() << "disconnect";
        socket.abort();
        //resumeAccepting();
    }
public slots:
    void gaze_event(double x, double y) {
        qDebug() << x << y;
        if (socket.isWritable()) {
            qDebug() << "write";
            char data[sizeof(double)*2];
            *((double*)&data[0]) = x;
            *((double*)&data[sizeof(double)]) = y;
            socket.write(data, sizeof(data));
        }
    }

protected:
    QTcpSocket socket;
};


#endif // TRACKERSERVER_H
