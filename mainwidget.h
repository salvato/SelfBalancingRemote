#pragma once

#include <QWidget>
#include <QTcpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <QByteArray>

QT_FORWARD_DECLARE_CLASS(GLWidget)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(Plot2D)

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();


public slots:
    void onConnectToClient();
    void handleLookup(QHostInfo hostInfo);
    void displayError(QAbstractSocket::SocketError socketError);
    void onServerConnected();
    void onServerDisconnected();
    void onNewDataAvailable();

protected:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void initLayout();
    void createButtons();
    void createPlot();
    void executeCommand(QString command);


private:
    QTcpSocket tcpClient;
    QHostAddress serverAddress;
    QByteArray message;
    QString receivedCommand;
    int bytesWritten;
    int bytesReceived;


    GLWidget* pGLWidget;
    Plot2D*   pPlotVal;

    QPushButton* buttonAccCalibration;
    QPushButton* buttonGyroCalibration;
    QPushButton* buttonMagCalibration;
    QPushButton* buttonShowPidOutput;
    QPushButton* buttonStartStop;
    QPushButton* buttonHide3D;
    QPushButton* buttonConnect;

    QLineEdit*   editHostName;


    float q0, q1, q2, q3;

};
