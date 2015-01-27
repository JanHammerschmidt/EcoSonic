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
        //connect(&socket, SIGNAL(disconnected()), this, SLOT(disconnect()));
        connect(this, &QTcpServer::newConnection, this, &TrackerServer::new_connection);
    }


protected:

//    virtual void incomingConnection(qintptr socketDescriptor) override
//    {
//        qDebug() << "incoming connection";
//        //pauseAccepting();
//        socket.setSocketDescriptor(socketDescriptor);

//        //QTcpServer::incomingConnection(socketDescriptor);
//    }

protected slots:
    void new_connection()
    {
        qDebug() << "incoming connection..";
        pauseAccepting();
        if (socket != nullptr) {
            qDebug() << "WARNING: existing socket found!";
            socket->deleteLater();
            socket->abort();
            socket->close();
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
        //socket.abort();
    }
public slots:
    void gaze_event(double x, double y) {
        //qDebug() << x << y;
        if (socket != nullptr && socket->isWritable()) {
            //qDebug() << "write";
            char data[sizeof(double)*2];
            *((double*)&data[0]) = x;
            *((double*)&data[sizeof(double)]) = y;
            socket->write(data, sizeof(data));
        }
    }

protected:
    QTcpSocket* socket = nullptr;
    //QTcpSocket socket;
};


#endif // TRACKERSERVER_H
