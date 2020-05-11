#include "mainwidget.h"
#include "plot2d.h"
#include "GLwidget.h"

#include <QDebug>
#include <QThread>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>


//==============================================================
// Orders (Sent)            Meaning
//--------------------------------------------------------------
//      G           Go Controling the Robot
//      S           Stop Controlling the Robot and Halt
//      P           Send PID Values (Kp, Ki, Kd)
//      C           Ask Robot Configuration
//      M           Start Moving (at a given speed Left & Rigth)
//      H           Stop Moving
//==============================================================


//==============================================================
// Commands (Received)          Meaning
//--------------------------------------------------------------
//      q               Quaternion Value
//      p               PID Values (time, input & output)
//      c               Robot Configuration Values
//==============================================================


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , pUdpSocket(nullptr)
    , udpPort(37755)
    // Widgets
    , pGLWidget(nullptr)
    , pPlotVal(nullptr)
    // Status
    , bRunInProgress(false)
    , bShowPidInProgress(false)
    , bMoveInProgress(false)
{
    initLayout();

    pUdpSocket = new QUdpSocket(this);
    if(!pUdpSocket->bind(QHostAddress::Any, udpPort)) {
        qDebug() << "Unable to bind";
        exit(EXIT_FAILURE);
    }

    // Network events
    connect(pUdpSocket, SIGNAL(readyRead()),
            this, SLOT(readPendingDatagrams()));
    connect(&tcpClient, SIGNAL(connected()),
            this, SLOT(onServerConnected()));
    connect(&tcpClient, SIGNAL(disconnected()),
            this, SLOT(onServerDisconnected()));
    connect(&tcpClient, SIGNAL(readyRead()),
            this, SLOT(onNewDataAvailable()));
    connect(&tcpClient, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));

    // Timer Event for Updating the Widgets
    connect(&timerUpdate, SIGNAL(timeout()),
            this, SLOT(onTimeToUpdateWidgets()));
}


MainWidget::~MainWidget() {
}


void
MainWidget::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event)
    if(pUdpSocket)
        delete pUdpSocket;
}


void
MainWidget::keyPressEvent(QKeyEvent *e) {
  if(e->key() == Qt::Key_Escape)
    return;
  else
    QWidget::keyPressEvent(e);
}


void
MainWidget::createUi() {
    buttonStartStop       = new QPushButton("Start",     this);
    buttonAccCalibration  = new QPushButton("Acc. Cal.", this);
    buttonGyroCalibration = new QPushButton("Gyro Cal.", this);
    buttonMagCalibration  = new QPushButton("Mag. Cal.", this);
    buttonShowPidOutput   = new QPushButton("Show PID",  this);
    buttonAskConf         = new QPushButton("Get Conf.", this);
    buttonConnect         = new QPushButton("Connect",   this);
    buttonMove            = new QPushButton("Move",      this);
    buttonSetPid          = new QPushButton("Set PID",   this);

    labelHost = new QLabel("Hostname", this);
    editHostName = new QLineEdit("raspberrypi.local", this);

    labelSpeedL = new QLabel("L Speed", this);
    labelSpeedR = new QLabel("R Speed", this);
    editMoveSpeedL = new QLineEdit("0.0", this);
    editMoveSpeedR = new QLineEdit("0.0", this);

    labelKp = new QLabel("Kp", this);
    labelKi = new QLabel("Ki", this);
    labelKd = new QLabel("Kd", this);
    editKp = new QLineEdit("1.0", this);
    editKi = new QLineEdit("0.0", this);
    editKd = new QLineEdit("0.0", this);

    pGLWidget = new GLWidget(this);
    createPlot();

    connect(buttonStartStop, SIGNAL(clicked()),
            this, SLOT(onStartStopPushed()));
    connect(buttonAccCalibration, SIGNAL(clicked()),
            this, SLOT(onStartAccCalibration()));
    connect(buttonGyroCalibration, SIGNAL(clicked()),
            this, SLOT(onStartGyroCalibration()));
    connect(buttonMagCalibration, SIGNAL(clicked()),
            this, SLOT(onStartMagCalibration()));
    connect(buttonShowPidOutput, SIGNAL(clicked(bool)),
            this, SLOT(onShowPidOutput()));
    connect(buttonAskConf, SIGNAL(clicked()),
            this, SLOT(onAskConfPushed()));
    connect(buttonConnect, SIGNAL(clicked()),
            this, SLOT(onConnectToClient()));
    connect(buttonMove, SIGNAL(clicked()),
            this, SLOT(onStartMovePushed()));
    connect(buttonSetPid, SIGNAL(clicked()),
            this, SLOT(onSetPID()));

    setDisableUI(true);
}


void
MainWidget::setDisableUI(bool bDisable) {
    buttonStartStop->setDisabled(bDisable);
    buttonAskConf->setDisabled(bDisable);
    buttonAccCalibration->setDisabled(bDisable);
    buttonGyroCalibration->setDisabled(bDisable);
    buttonMagCalibration->setDisabled(bDisable);
    buttonShowPidOutput->setDisabled(bDisable);
    buttonMove->setDisabled(bDisable);
    buttonSetPid->setDisabled(bDisable);

    editKp->setDisabled(bDisable);
    editKi->setDisabled(bDisable);
    editKd->setDisabled(bDisable);

    editMoveSpeedL->setDisabled(bDisable);
    editMoveSpeedR->setDisabled(bDisable);
}


void
MainWidget::createPlot() {
    pPlotVal = new Plot2D(this, "Plot");

    pPlotVal->NewDataSet(1, 1, QColor(255,   0,   0), Plot2D::ipoint, "X");
    pPlotVal->NewDataSet(2, 1, QColor(  0, 255,   0), Plot2D::ipoint, "Y");
    pPlotVal->NewDataSet(3, 1, QColor(  0,   0, 255), Plot2D::ipoint, "Z");
    pPlotVal->NewDataSet(4, 1, QColor(255, 255, 255), Plot2D::ipoint, "PID-In");
    pPlotVal->NewDataSet(5, 1, QColor(255, 255,  64), Plot2D::ipoint, "PID-Out");

    pPlotVal->SetShowTitle(1, true);
    pPlotVal->SetShowTitle(2, true);
    pPlotVal->SetShowTitle(3, true);
    pPlotVal->SetShowTitle(4, true);
    pPlotVal->SetShowTitle(5, true);

    pPlotVal->SetLimits(-1.0, 1.0, -1.0, 1.0, true, true, false, false);

    pPlotVal->SetShowDataSet(4, true);
    pPlotVal->SetShowDataSet(5, true);
}


void
MainWidget::initLayout() {
    createUi();

    firstButtonRow = new QHBoxLayout;
    secondButtonRow = new QHBoxLayout;
    thirdButtonRow = new QHBoxLayout;

    firstButtonRow->addWidget(buttonStartStop);
    firstButtonRow->addWidget(buttonAskConf);
    firstButtonRow->addWidget(buttonAccCalibration);
    firstButtonRow->addWidget(buttonGyroCalibration);
    firstButtonRow->addWidget(buttonMagCalibration);
    firstButtonRow->addWidget(buttonShowPidOutput);

    secondButtonRow->addWidget(labelHost);
    secondButtonRow->addWidget(editHostName);
    secondButtonRow->addWidget(buttonConnect);

    thirdButtonRow->addWidget(labelSpeedL);
    thirdButtonRow->addWidget(editMoveSpeedL);
    thirdButtonRow->addWidget(labelSpeedR);
    thirdButtonRow->addWidget(editMoveSpeedR);
    thirdButtonRow->addWidget(buttonMove);

    thirdButtonRow->addWidget(labelKp);
    thirdButtonRow->addWidget(editKp);
    thirdButtonRow->addWidget(labelKi);
    thirdButtonRow->addWidget(editKi);
    thirdButtonRow->addWidget(labelKd);
    thirdButtonRow->addWidget(editKd);
    thirdButtonRow->addWidget(buttonSetPid);

    QHBoxLayout *firstRow = new QHBoxLayout;
    firstRow->addWidget(pGLWidget);
    firstRow->addWidget(pPlotVal);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(firstRow);
    mainLayout->addLayout(firstButtonRow);
    mainLayout->addLayout(secondButtonRow);
    mainLayout->addLayout(thirdButtonRow);

    setLayout(mainLayout);
}


void
MainWidget::onConnectToClient() {
    if(buttonConnect->text() == tr("Connect")) {
        buttonConnect->setDisabled(true);
        editHostName->setDisabled(true);
        QHostInfo::lookupHost(editHostName->text(), this, SLOT(handleLookup(QHostInfo)));
    } else {//pButtonConnect->text() == tr("Disconnect")
        tcpClient.close();
    }
}


void
MainWidget::handleLookup(QHostInfo hostInfo) {
    // Handle the results.
    if(hostInfo.error() == QHostInfo::NoError) {
        if(!hostInfo.addresses().isEmpty()) {
            serverAddress = hostInfo.addresses().at(0);
            qDebug() << QString("Connecting to: %1").arg(hostInfo.hostName());
            tcpClient.connectToHost(serverAddress, 43210);
        }
        else {
            qDebug() << QString(hostInfo.errorString());
            buttonConnect->setEnabled(true);
            editHostName->setEnabled(true);
        }
    } else {
        qDebug() << QString(hostInfo.errorString());
        buttonConnect->setEnabled(true);
        editHostName->setEnabled(true);
    }
}


void
MainWidget::displayError(QAbstractSocket::SocketError socketError) {
    if(socketError == QTcpSocket::RemoteHostClosedError) {
        qDebug() << QString("The remote host has closed the connection");
        tcpClient.close();
        return;
    }
    qDebug() << QString(tcpClient.errorString());
    tcpClient.close();
    buttonConnect->setEnabled(true);
    editHostName->setEnabled(true);
}


void
MainWidget::onServerConnected() {
    qDebug() << QString("Connected");

    buttonConnect->setText("Disconnect");
    buttonConnect->setEnabled(true);

    setDisableUI(false);

    pPlotVal->ClearDataSet(4);
    pPlotVal->ClearDataSet(5);
    timerUpdate.start(100);
}


void
MainWidget::onServerDisconnected() {
    timerUpdate.stop();
    setDisableUI(true);
    buttonConnect->setText("Connect");
    editHostName->setEnabled(true);
}


void
MainWidget::onNewDataAvailable() {
    receivedCommand += tcpClient.readAll();
    QString sNewCommand;
    int iPos;
    iPos = receivedCommand.indexOf("#");
    while(iPos != -1) {
        sNewCommand = receivedCommand.left(iPos);
        executeCommand(sNewCommand);
        receivedCommand = receivedCommand.mid(iPos+1);
        iPos = receivedCommand.indexOf("#");
    }
}


void
MainWidget::readPendingDatagrams() {
    while(pUdpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = pUdpSocket->receiveDatagram();
        QString sReceived = QString(datagram.data());
        QString sNewCommand;
        int iPos;
        iPos = sReceived.indexOf("#");
        while(iPos != -1) {
            sNewCommand = sReceived.left(iPos);
            executeCommand(sNewCommand);
            sReceived = sReceived.mid(iPos+1);
            iPos = sReceived.indexOf("#");
        }
    }
}


void
MainWidget::onTimeToUpdateWidgets() {
    pGLWidget->update();
    pPlotVal->UpdatePlot();
}


void
MainWidget::executeCommand(QString command) {
    QStringList tokens = command.split(' ');
    tokens.removeFirst();
    char cmd = command.at(0).toLatin1();
    if(cmd == 'q') { // It is a Quaternion !
        if(tokens.count() == 4) {
            q0 = tokens.at(0).toDouble();
            q1 = tokens.at(1).toDouble();
            q2 = tokens.at(2).toDouble();
            q3 = tokens.at(3).toDouble();
            pGLWidget->setRotation(q0, q1, q2, q3);
        }
    }
    else if(cmd == 'p') { // PID Input & Output values
        if(tokens.count() == 3) {
            double x = tokens.at(0).toDouble();
            double input = tokens.at(1).toDouble();
            double output = tokens.at(2).toDouble();
            pPlotVal->NewPoint(4, x, double(input));
            pPlotVal->NewPoint(5, x, double(output));
        }
    }
//    else if(cmd == 'c') { // Robot Configuration Values
//        if(tokens.count() == 3) {
//        }
//    }
}


void
MainWidget::onStartStopPushed() {
    if(bRunInProgress) {
        if(tcpClient.isOpen()) {
            message.clear();
            message.append("S#"); // Stop !
            tcpClient.write(message);
            bRunInProgress = false;
            buttonStartStop->setText("Start");
            buttonAccCalibration->setDisabled(true);
            buttonGyroCalibration->setDisabled(true);
            buttonMagCalibration->setDisabled(true);
            buttonShowPidOutput->setDisabled(true);
        }
    }
    else {
        if(tcpClient.isOpen()) {
            message.clear();
            message.append("G#"); // Go !
            tcpClient.write(message);
            bRunInProgress = true;
            buttonStartStop->setText("Stop");
            buttonAccCalibration->setEnabled(true);
            buttonGyroCalibration->setEnabled(true);
            buttonMagCalibration->setEnabled(true);
            buttonShowPidOutput->setEnabled(true);
        }
    }
}


void
MainWidget::onStartAccCalibration() {
}


void
MainWidget::onStartGyroCalibration() {
}


void
MainWidget::onStartMagCalibration() {
}


void
MainWidget::onAskConfPushed() {
    if(tcpClient.isOpen()) {
        message.clear();
        message.append("C#"); // Ask Robot Configuration
        tcpClient.write(message);
    }
}


void
MainWidget::onShowPidOutput() {
    if(bShowPidInProgress) {
        if(tcpClient.isOpen()) {
            message.clear();
            message.append("N#"); // Stop Sending PID Values
            tcpClient.write(message);
            bShowPidInProgress = false;
            buttonShowPidOutput->setText("Show PID");
            buttonAccCalibration->setEnabled(true);
            buttonGyroCalibration->setEnabled(true);
            buttonMagCalibration->setEnabled(true);
        }
    }
    else {
        if(tcpClient.isOpen()) {
            message.clear();
            message.append("P#"); // Send PID Values
            tcpClient.write(message);
            bShowPidInProgress = true;

            buttonShowPidOutput->setText("Hide Pid Out");
            buttonAccCalibration->setDisabled(true);
            buttonGyroCalibration->setDisabled(true);
            buttonMagCalibration->setDisabled(true);

            pPlotVal->SetShowDataSet(1, false);
            pPlotVal->SetShowDataSet(2, false);
            pPlotVal->SetShowDataSet(3, false);
            pPlotVal->SetShowDataSet(4, true);
            pPlotVal->SetShowDataSet(5, true);
        }
    }
}


void
MainWidget::onStartMovePushed() {
    if(bMoveInProgress) {
        if(tcpClient.isOpen()) {
            message.clear();
            message.append("H#"); // Halt Move !
            tcpClient.write(message);
            bMoveInProgress = false;
            buttonMove->setText("Move");
            buttonAccCalibration->setEnabled(true);
            buttonGyroCalibration->setEnabled(true);
            buttonMagCalibration->setEnabled(true);
            buttonShowPidOutput->setEnabled(true);
        }
    }
    else {
        if(tcpClient.isOpen()) {
            QString sMessage = QString("M %1 %2#")
                    .arg(editMoveSpeedL->text(), editMoveSpeedR->text()); // Start Moving
            tcpClient.write(sMessage.toLatin1());
            bMoveInProgress = true;
            buttonMove->setText("Halt");
            buttonAccCalibration->setDisabled(true);
            buttonGyroCalibration->setDisabled(true);
            buttonMagCalibration->setDisabled(true);
            buttonShowPidOutput->setDisabled(true);
        }
    }
}


void
MainWidget::onSetPID() {
    if(tcpClient.isOpen()) {
        QString sMessage = QString("C %1 %2 %3#")
                .arg(editKp->text(),
                     editKi->text(),
                     editKd->text());
        tcpClient.write(sMessage.toLatin1());
    }
}
