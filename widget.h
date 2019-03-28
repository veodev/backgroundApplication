#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTime>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTcpServer>

namespace Ui
{
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget* parent = nullptr);
    ~Widget();

private:
    void readMessageFromBuffer();
    void sendIdentification();
    void sendConfirmationPathEncoder(QByteArray& message);
    void trainingPcTcpSocketStateChanged(QAbstractSocket::SocketState state);
    void trainingPcTcpReadyRead();
    void checkApp();
#ifdef ANDROID
    void keepScreenOn(bool on);
#endif


private slots:
    void onNewConnection();
    void onTcpSocketStateChanged(QAbstractSocket::SocketState state);
    void onTcpReadyRead();
    void on_pushButton_released();


private:
    Ui::Widget* ui;
    QTcpSocket* _trainingPcTcpSocket;
    QTcpSocket* _tcpSocket;
    QUdpSocket* _udpSocket;
    QTcpServer* _tcpServer;
    QByteArray _readBuffer;
    QTimer _checkTimer;
};

#endif  // WIDGET_H
