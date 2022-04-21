#pragma once

#include <QTcpServer>
#include <QThreadPool>
#include <memory>
#include <vector>

class HttpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit HttpServer(QObject* parent = nullptr, int port = 8080);

    ~HttpServer() override;

    void incomingConnection(qintptr handle) override;

private slots:
    void stop_server();

private:
    std::unique_ptr<QThreadPool> threads_;
    int                          port_;
};
