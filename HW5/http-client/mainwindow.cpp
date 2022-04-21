#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QUrl>
#include <sstream>


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    manager_ = new QNetworkAccessManager();
    connect(manager_,
            &QNetworkAccessManager::finished,
            this,
            &MainWindow::receive_response);
    // socket_ = new QTcpSocket();
}

MainWindow::~MainWindow()
{
    delete url_;
    delete ui;
}

void MainWindow::receive_response(QNetworkReply* reply)
{
    if (reply->error())
    {
        QString error = "ERROR:\n" + reply->errorString();
        ui->textBrowser->append(error);
    }
    else
        ui->textBrowser->append(reply->readAll());
}

void MainWindow::on_connect_clicked()
{
    url_ = new QUrl("http://" + ui->url->text());
}

void MainWindow::on_GET_clicked()
{
    ui->textBrowser->clear();
    QNetworkRequest request;
    request.setUrl(*url_);
    manager_->get(request);
}

void MainWindow::on_POST_clicked()
{
    ui->textBrowser->clear();
    QNetworkRequest request;
    request.setUrl(*url_);
    manager_->post(request, "");
}

void MainWindow::on_url_returnPressed()
{
    url_ = new QUrl("http://" + ui->url->text());
}
