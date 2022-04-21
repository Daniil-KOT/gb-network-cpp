#pragma once

#include <QMainWindow>

class QTcpSocket;
class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

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

private slots:
    void on_connect_clicked();

    void on_GET_clicked();

    void on_POST_clicked();

    void receive_response(QNetworkReply* reply);

    void on_url_returnPressed();

private:
    Ui::MainWindow*        ui;
    QUrl*                  url_;
    QNetworkAccessManager* manager_;
};
