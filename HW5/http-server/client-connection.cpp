#include "client-connection.h"

#include "http-server.h"
#include <QDateTime>
#include <QRegularExpression>
#include <QtNetwork/QTcpSocket>


ClientConnection::ClientConnection(QTcpSocket* socket, QObject* parent)
    : QObject{ parent }
    , socket_{ socket }
{
    connect(socket_.get(),
            &QAbstractSocket::disconnected,
            this,
            &QObject::deleteLater);
    connect(socket_.get(),
            &QIODevice::readyRead,
            this,
            &ClientConnection::ready_read);
}

void ClientConnection::ready_read()
{
    const auto lines = QString(socket_->readLine())
                           .split(QRegularExpression("[ \r\n][ \r\n]*"));

    qDebug() << lines;

    if (lines[0] == "POST")
    {
        if (lines[1] == "/server/control/exit")
        {
            for (int i = 0; i < lines.size(); ++i)
                qDebug() << lines[i];

            qDebug() << "\nEXIT!!!";
            *enabled_ = false;
            emit server_stopped();
            return;
        }
    }
}

ClientConnection::~ClientConnection() = default;
