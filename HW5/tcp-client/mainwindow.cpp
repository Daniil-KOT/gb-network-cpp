#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QTcpSocket>
#include <QDataStream>
#include <QRegularExpression>
#include <sstream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    socket_ = new QTcpSocket();
}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::create_get_header()
{
    auto url = ui->url->text().split(QRegularExpression("[/]"));

    if (url.size() > 1)
    {
        header_.statusLine_ = "GET /" + url[1] + " HTTP/1.1\r\n";
        header_.host_ = "Host: " + url[0] + "\r\n";
    }
    else
    {
        header_.statusLine_ = "GET / HTTP/1.1\r\n";
        header_.host_ = "Host: " + ui->url->text() + "\r\n";
    }
    header_.userAgent_ = "User-Agent: Simple HTTP Client v0.1\r\n";
    header_.contentType_ = "";
    header_.contentLength_ = "";
    header_.acceptLanguage_ = "Accept-Language: en-en\r\n";
    header_.acceptEncoding_ = "Accept-Encoding: gzip, deflate\r\n";
    header_.connection_ = "Connection: Keep-Alive\r\n\r\n";
}

void MainWindow::create_post_header()
{
    auto url = ui->url->text().split(QRegularExpression("[/]"));

    if (url.size() > 1)
    {
        header_.statusLine_ = "POST ";
        for (int i = 1; i < url.size(); ++i)
        {
            header_.statusLine_ += "/" + url[i];
        }
        header_.statusLine_ += " HTTP/1.1\r\n";
        header_.host_ = "Host: " + url[0] + "\r\n";
    }
    else
    {
        header_.statusLine_ = "POST / HTTP/1.1\r\n";
        header_.host_ = "Host: " + ui->url->text() + "\r\n";
    }
    header_.userAgent_ = "User-Agent: Simple HTTP Client v0.1\r\n";
    header_.contentType_ = "Content-Type: text/html\r\n";

    std::stringstream ss;
    ss << content_.length();
    header_.contentLength_ = "Content Length: " + QString(ss.str().c_str()) + "\r\n";
    header_.acceptLanguage_ = "Accept-Language: en-en\r\n";
    header_.acceptEncoding_ = "Accept-Encoding: gzip, deflate\r\n";
    header_.connection_ = "Connection: Keep-Alive\r\n\r\n";
}

void MainWindow::send_query()
{
    QString query = header_.statusLine_;
    query += header_.userAgent_;

    if (!header_.contentType_.isEmpty())
        query += header_.contentType_;

    if (!header_.contentLength_.isEmpty())
        query += header_.contentLength_;

    query += header_.acceptLanguage_;
    query += header_.acceptEncoding_;

    qDebug() << "\n\n" << query;

    socket_->write(query.toStdString().c_str());
    socket_->waitForBytesWritten();
    ui->textBrowser->clear();
    receive_response();
}

void MainWindow::receive_response()
{
    socket_->waitForReadyRead();
    ui->textBrowser->append(socket_->readAll());
}

void MainWindow::on_connect_clicked()
{
    auto url = ui->url->text().split(QRegularExpression("[/]"));
    qDebug() << url;
    socket_->connectToHost(url[0],
                           ui->port->text().toInt());
}

void MainWindow::on_GET_clicked()
{
    content_ = "";
    create_get_header();
    send_query();
}

void MainWindow::on_POST_clicked()
{
    content_ = "Exit!";
    create_post_header();
    send_query();
}
