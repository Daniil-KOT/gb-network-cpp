#pragma once

#include <QObject>
#include <memory>

class QTcpSocket;
class HttpServer;

class ClientConnection : public QObject
{
    Q_OBJECT

public:
    explicit ClientConnection(QTcpSocket* socket, QObject* parent = nullptr);

    ~ClientConnection();

    void start();

private slots:

    void ready_read();

signals:

    void server_stopped();

private:
    const std::unique_ptr<QTcpSocket> socket_;
    bool*                             enabled_;
};
