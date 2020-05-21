#pragma once

#include <QWidget>
#include <QTcpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <QByteArray>
#include <QTimer>


QT_FORWARD_DECLARE_CLASS(GLWidget)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(Plot2D)
QT_FORWARD_DECLARE_CLASS(QUdpSocket)
QT_FORWARD_DECLARE_CLASS(QStatusBar)


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
    void readPendingDatagrams();
    void onButtonManualPushed();
    void onStartMovePushed();
    void onSetPIDPushed();
    void onTimeToUpdateWidgets();

protected:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void initLayout();
    void restoreSettings();
    void saveSettings();
    void createUi();
    void createPlot();
    void executeCommand(QString command);
    void setDisableUI(bool bDisable);
    void askConfiguration();

private:
    QTcpSocket   tcpClient;
    QHostAddress serverAddress;
    QByteArray   message;
    QString      receivedCommand;

    QUdpSocket*  pUdpSocket;
    int          udpPort;

    GLWidget* pGLWidget;
    Plot2D*   pPlotVal;

    QHBoxLayout* firstButtonRow;
    QHBoxLayout* secondButtonRow;
    QHBoxLayout* thirdButtonRow;

    QPushButton* buttonManualControl;

    QPushButton* buttonConnect;
    QLabel*      labelHost;
    QLineEdit*   editHostName;

    QPushButton* buttonMove;
    QLabel*      labelSpeedL;
    QLineEdit*   editMoveSpeedL;
    QLabel*      labelSpeedR;
    QLineEdit*   editMoveSpeedR;

    QPushButton* buttonSetPid;
    QLabel*      labelKp;
    QLineEdit*   editKp;
    QLabel*      labelKi;
    QLineEdit*   editKi;
    QLabel*      labelKd;
    QLineEdit*   editKd;
    QLabel*      labelSetpoint;
    QLineEdit*   editSetpoint;

    QStatusBar*  statusBar;

    bool bPIDInControl;

    float q0, q1, q2, q3;
    QTimer timerUpdate;
};
