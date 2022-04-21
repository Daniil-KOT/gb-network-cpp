#include "server-control.h"

ServerControl::ServerControl(QObject* parent)
    : QObject{ parent }
{
}

void ServerControl::stop()
{
    emit stop_server();
}
