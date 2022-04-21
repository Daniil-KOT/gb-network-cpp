#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QTcpSocket>

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

void MainWindow::on_pushButton_clicked()
{
    socket_->connectToHost(ui->url->displayText(),
                           ui->port->displayText().toInt());
}
