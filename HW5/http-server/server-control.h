#pragma once

#include <QObject>

class ServerControl : public QObject
{
    Q_OBJECT

public:
    explicit ServerControl(QObject* parent = nullptr);

    void stop();

signals:
    void stop_server();
};
