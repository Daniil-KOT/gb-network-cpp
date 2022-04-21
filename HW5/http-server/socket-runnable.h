#pragma once

#include "response-header.h"

#include <QDebug>
#include <QRunnable>
#include <QTcpSocket>


class ServerControl;

class SocketRunnable : public QRunnable
{
public:
    SocketRunnable(int handle, ServerControl* controller);

    void run() override;

private:
    void write(QTcpSocket* socket);

    void createOkHeader();

    void createNotFoundHeader();

    void createInfoBody();

    void createNotFoundBody();

    void disconnect(QTcpSocket* socket);

private:
    int            descriptor_;
    ServerControl* controller_;
    ResponseHeader header_;
    QString        responseBody_;
};
