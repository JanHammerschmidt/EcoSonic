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
        connect(this, &QTcpServer::newConnection, this, &TrackerServer::new_connection);
    }

protected slots:
    void new_connection()
    {
        qDebug() << "incoming connection..";
        new_conn = true;
        pauseAccepting();
        if (socket != nullptr) {
            qDebug() << "WARNING: existing socket found!";
            qDebug() << "..ignoring!";
//            socket->abort();
//            socket->close();
//            socket->deleteLater();
        }
        socket = nextPendingConnection();
        connect(socket, &QTcpSocket::disconnected, this, &TrackerServer::disconnect);
    }

    void disconnect()
    {
        qDebug() << "disconnect";
        if (socket != nullptr) {
            socket->deleteLater();
            socket = nullptr;
        }
        resumeAccepting();
    }
public slots:
    void gaze_event(double x, double y) {
        //qDebug() << x << y;
        if (socket != nullptr && socket->isWritable()) {
            if (new_conn) {
                new_conn = false;
                qDebug() << "sending...";
            }
            //qDebug() << "write";
            char data[sizeof(double)*2];
            *((double*)&data[0]) = x;
            *((double*)&data[sizeof(double)]) = y;
            socket->write(data, sizeof(data));
        }
    }

protected:
    QTcpSocket* socket = nullptr;
    bool new_conn = false;
};

#endif // TRACKERSERVER_H
