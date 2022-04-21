#include "http-server.h"
#include <QCoreApplication>


int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    HttpServer server;

    return a.exec();
}
