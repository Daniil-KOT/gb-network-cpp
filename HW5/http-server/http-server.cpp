#include "http-server.h"
#include "client-connection.h"
#include "server-control.h"
#include "socket-runnable.h"


#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>


HttpServer::HttpServer(QObject* parent, int port)
    : QTcpServer{ parent }
    , port_{ port }
{
    if (listen(QHostAddress::Any, port_))
    {
        qDebug() << "Listening..";
    }
    else
    {
        qDebug() << "Error on listening!";
    }
    threads_ = std::make_unique<QThreadPool>(this);
    threads_->setMaxThreadCount(20);
}

HttpServer::~HttpServer() = default;

void HttpServer::incomingConnection(qintptr handle)
{
    ServerControl*  controller = new ServerControl();
    SocketRunnable* socket     = new SocketRunnable(handle, controller);

    threads_->start(socket);

    connect(controller,
            &ServerControl::stop_server,
            this,
            &HttpServer::stop_server);
}

void HttpServer::stop_server()
{
    qDebug() << "Stopping server!";
    close();
}
