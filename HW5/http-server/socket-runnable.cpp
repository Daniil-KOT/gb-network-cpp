#include "socket-runnable.h"
#include "server-control.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QThread>
#include <sstream>


SocketRunnable::SocketRunnable(int handle, ServerControl* controller)
    : descriptor_{ handle }
    , controller_{ controller }
{
}

void SocketRunnable::run()
{
    QTcpSocket* socket = new QTcpSocket();
    socket->setSocketDescriptor(descriptor_);

    socket->waitForReadyRead();
    // qDebug() << socket->readAll();

    const auto line = QString(socket->readLine())
                          .split(QRegularExpression("[ \r\n][ \r\n]*"));

    if (line[0] == "POST")
    {
        if (line[1] == "/server/control/exit")
        {
            controller_->stop();
        }
    }
    else if (line[0] == "GET")
    {
        if (line[1] == "/info")
        {
            createInfoBody();
            createOkHeader();
            write(socket);
        }
        else
        {
            createNotFoundBody();
            createNotFoundHeader();
            write(socket);
        }
    }

    if (socket->bytesToWrite() == 0)
        disconnect(socket);
}

void SocketRunnable::write(QTcpSocket* socket)
{
    QString response = header_.statusLine_;
    response += header_.date_;
    response += header_.contentLength_;
    response += header_.contentType_;
    response += header_.connectionStatus_;
    response += responseBody_;

    qDebug() << "\n\n" << response;

    socket->write(response.toStdString().c_str());
    socket->waitForBytesWritten();
}

void SocketRunnable::createOkHeader()
{
    header_.statusLine_ = "HTTP/1.1 200 Ok\r\n";
    header_.date_ =
        "Date: " + QDateTime::currentDateTimeUtc().toString() + "\r\n";

    std::stringstream ss;
    ss << responseBody_.length();
    header_.contentLength_ =
        "Content-Length: " + QString(ss.str().c_str()) + "\r\n";

    header_.contentType_      = "Content-Type: text/html\r\n";
    header_.connectionStatus_ = "Connection: Closed\r\n\r\n";
}

void SocketRunnable::createNotFoundHeader()
{
    header_.statusLine_ = "HTTP/1.1 404 Not Found\r\n";
    header_.date_ =
        "Date: " + QDateTime::currentDateTimeUtc().toString() + "\r\n";

    std::stringstream ss;
    ss << responseBody_.length();
    header_.contentLength_ =
        "Content-Length: " + QString(ss.str().c_str()) + "\r\n";

    header_.contentType_      = "Content-Type: text/html\r\n";
    header_.connectionStatus_ = "Connection: Closed\r\n\r\n";
}

void SocketRunnable::createInfoBody()
{
    responseBody_ = "<html>"
                    "<body>"
                    "<h1>Info:</h1>"
                    "<p>Simple HTTP server</p>"
                    "<p>Available pages:</p> "
                    "<p>/server/control/exit</p>"
                    "<p>/info</p>"
                    "</body>"
                    "</html>";
}

void SocketRunnable::createNotFoundBody()
{
    responseBody_ = "<html>"
                    "<head>"
                    "<title>404 Not Found</title>"
                    "</head>"
                    "<body>"
                    "<h1>Not Found</h1>"
                    "<p>The requested URL was not found on this server.</p>"
                    "<p>Available pages: /server/control/exit</p>"
                    "<p>/info</p>"
                    "</body>"
                    "</html>";
}

void SocketRunnable::disconnect(QTcpSocket* socket)
{
    socket->disconnectFromHost();
    socket->close();
    socket->deleteLater();
}
