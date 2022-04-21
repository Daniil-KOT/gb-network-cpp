#pragma once

#include <QMainWindow>
#include "query-header.h"

class QTcpSocket;

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    void create_get_header();

    void create_post_header();

    void send_query();

    void receive_response();

private slots:
    void on_connect_clicked();

    void on_GET_clicked();

    void on_POST_clicked();

private:
    Ui::MainWindow* ui;
    QTcpSocket*     socket_;
    QString query_;
    QString content_;
    QueryHeader header_;
};
