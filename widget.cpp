#include "widget.h"
#include "ui_widget.h"
#include <iostream>

#include <QDebug>
#include <QtEndian>
#include <QProcess>
#include <QMovie>
#ifdef ANDROID
#include <QtAndroid>
#include <QtAndroidExtras>
#endif

Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , _tcpSocket(nullptr)
    , _udpSocket(nullptr)
    , _tcpServer(nullptr)
{
    ui->setupUi(this);
#ifdef ANDROID
    keepScreenOn(true);
#endif
    _tcpServer = new QTcpServer(this);
    connect(_tcpServer, &QTcpServer::newConnection, this, &Widget::onNewConnection);
    //    _tcpServer->listen(QHostAddress("192.168.100.100"), 43000);
    _tcpServer->listen(QHostAddress("127.0.0.1"), 43000);

    _trainingPcTcpSocket = new QTcpSocket(this);
    connect(_trainingPcTcpSocket, &QTcpSocket::stateChanged, this, &Widget::trainingPcTcpSocketStateChanged);
    connect(_trainingPcTcpSocket, &QTcpSocket::readyRead, this, &Widget::trainingPcTcpReadyRead);
    _trainingPcTcpSocket->connectToHost(QString("127.0.0.1"), 49004);

    connect(&_checkTimer, &QTimer::timeout, this, &Widget::checkApp);
    _checkTimer.start(1000);

    //    QTimer::singleShot(1000, this, &Widget::on_pushButton_released);

    QMovie* movie = new QMovie(":/images/loading.gif");
    ui->label_2->setMovie(movie);
    movie->start();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::readMessageFromBuffer()
{
    quint8 id = 0;
    quint8 source = 0;
    quint16 messageSize = 0;
    quint16 umuCommand = 0;

    quint8 tactCount = 0;

    QByteArray message;

    while (true) {
        if (_readBuffer.size() >= 6) {
            id = static_cast<quint8>(_readBuffer.at(0));
            source = static_cast<quint8>(_readBuffer.at(1));
            messageSize = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(_readBuffer.mid(2, sizeof(quint16)).data()));
            if (_readBuffer.size() >= (messageSize + 2)) {
                _readBuffer.remove(0, 6);
                qWarning() << "READ: " << QString::number(id, 16) << _readBuffer.left(messageSize).toHex(' ');

                switch (id) {
                case 0x42:
                    tactCount = _readBuffer.at(0);
                    qWarning() << "TACT COUNT: " << tactCount;
                    _readBuffer.remove(0, 1);
                    break;
                case 0x43:
                    umuCommand = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(_readBuffer.left(sizeof(quint16)).data()));
                    if (umuCommand == 0xDE) {
                        //                        qWarning() << "UMU COMMAND: " << QString("0x") + QString::number(umuCommand, 16);
                        sendIdentification();
                    }
                    _readBuffer.remove(0, 2);
                    break;
                case 0x1D:
                    message = _readBuffer.left(5);
                    _readBuffer.remove(0, 5);
                    //                    qWarning() << "SETUP PATH ENCODER: " << message.toHex(' ');
                    sendConfirmationPathEncoder(message);
                    break;
                default:
                    _readBuffer.remove(0, messageSize);
                    break;
                }
            }
            else {
                break;
            }
        }
        else {
            break;
        }
    }
}

void Widget::sendIdentification()
{
    QByteArray message;
    message.append(char(0xDF));
    message.append(char(0x04));
    message.append(char(0x08));
    message.append(char(0x0));
    message.append(char(0x0));
    message.append(char(0x0));
    message.append(char(0xFF));
    message.append(char(0xFF));
    message.append(char(0xFF));
    message.append(char(0xFF));
    message.append(char(0xFF));
    message.append(char(0xFF));
    message.append(char(0xFF));
    message.append(char(0xFF));
    //    _udpSocket->writeDatagram(message, QHostAddress("192.168.100.1"), 43001);
    _udpSocket->writeDatagram(message, QHostAddress("127.0.0.1"), 43001);

    message.clear();
    message.append(char(0xDB));
    message.append(char(0x04));
    message.append(char(0x02));
    message.append(char(0x0));
    message.append(char(0x0));
    message.append(char(0x0));
    message.append(char(0xFF));
    message.append(char(0xFF));
    //    _udpSocket->writeDatagram(message, QHostAddress("192.168.100.1"), 43001);
    _udpSocket->writeDatagram(message, QHostAddress("127.0.0.1"), 43001);
    _udpSocket->flush();
}

void Widget::sendConfirmationPathEncoder(QByteArray& message)
{
    QByteArray finalMessage;
    finalMessage.append(char(0x1D));
    finalMessage.append(char(0x04));
    finalMessage.append(char(0x05));
    finalMessage.append(char(0x0));
    finalMessage.append(char(0x0));
    finalMessage.append(char(0x0));
    finalMessage.append(message);
    //    _udpSocket->writeDatagram(finalMessage, QHostAddress("192.168.100.1"), 43001);
    _udpSocket->writeDatagram(finalMessage, QHostAddress("127.0.0.1"), 43001);
    _udpSocket->flush();
    qWarning() << "SEND PATH ENCODER CONFIRMATION";
}

void Widget::onTcpSocketStateChanged(QAbstractSocket::SocketState state)
{
    qWarning() << "TCP STATE: " << state;
}

void Widget::onNewConnection()
{
    qWarning() << "NEW CONNECTION";
    if (_tcpSocket != nullptr) {
        disconnect(_tcpSocket, &QTcpSocket::stateChanged, this, &Widget::onTcpSocketStateChanged);
        disconnect(_tcpSocket, &QTcpSocket::readyRead, this, &Widget::onTcpReadyRead);
        _tcpSocket->close();
        delete _tcpSocket;
        _tcpSocket = nullptr;
    }
    _tcpSocket = _tcpServer->nextPendingConnection();
    qWarning() << "Socket descriptor: " << _tcpSocket->socketDescriptor();
    qWarning() << "Socket peer port: " << _tcpSocket->peerPort();
    qWarning() << "Socket peer address: " << _tcpSocket->peerAddress();


    connect(_tcpSocket, &QTcpSocket::stateChanged, this, &Widget::onTcpSocketStateChanged);
    connect(_tcpSocket, &QTcpSocket::readyRead, this, &Widget::onTcpReadyRead);

    if (_udpSocket != nullptr) {
        _udpSocket->close();
        delete _udpSocket;
        _udpSocket = nullptr;
    }
    _udpSocket = new QUdpSocket(this);
    qWarning() << "IS BIND:" << _udpSocket->bind(43001);
}

void Widget::onTcpReadyRead()
{
    _readBuffer += _tcpSocket->readAll();
    readMessageFromBuffer();
}

void Widget::trainingPcTcpSocketStateChanged(QAbstractSocket::SocketState state)
{
    qWarning() << "TRAINING PC TCP SOCKET STATE: " << state;
}

void Widget::trainingPcTcpReadyRead() {}

void Widget::checkApp()
{
    qWarning() << QTime::currentTime().toString("hh:mm:ss");
}

#ifdef ANDROID
void Widget::keepScreenOn(bool on)
{
    QtAndroid::runOnAndroidThread([on] {
        QAndroidJniObject activity = QtAndroid::androidActivity();
        if (activity.isValid()) {
            QAndroidJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
            if (window.isValid()) {
                const int FLAG_KEEP_SCREEN_ON = 128;
                if (on) {
                    window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
                }
                else {
                    window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
                }
            }
        }
        QAndroidJniEnvironment env;
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
    });
}
#endif

void Widget::on_pushButton_released()
{
#ifdef ANDROID
    QAndroidJniObject::callStaticMethod<void>("com/radioavionica/BackgroundApplication/MyService", "startApplication", "(Landroid/content/Context;)V", QtAndroid::androidContext().object());
#endif
}
