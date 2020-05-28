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
#include <QStatusBar>
#include <QIcon>


//==============================================================
// Orders (Sent)            Meaning
//--------------------------------------------------------------
//      G           Set PID Control
//      S           Set Manual Control
//      P           Send PID Values (Kp, Ki, Kd)
//      C           Ask Robot Configuration
//      M           Start Moving (at a given speed Left & Rigth)
//      H           Stop Moving
//      K           Kill Remote Program
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
    , bPIDInControl(false)
{
    setWindowIcon(QIcon(":/10DOF.png"));
    initLayout();
    restoreSettings();

    pUdpSocket = new QUdpSocket(this);
    if(!pUdpSocket->bind(QHostAddress::Any, udpPort)) {
        statusBar->showMessage("Unable to bind... EXITING");
        setDisabled(true);
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

    // Timer Event for Widgets Updating
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
    saveSettings();
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
    buttonClose           = new QPushButton("Close",     this);
    buttonManualControl   = new QPushButton("PID Ctrl",  this);
    buttonConnect         = new QPushButton("Connect",   this);
    buttonMove            = new QPushButton("Move",      this);
    buttonSetPid          = new QPushButton("Set PID",   this);

    labelHost    = new QLabel("Hostname", this);
    editHostName = new QLineEdit("raspberrypi.local", this);

    labelSpeedL    = new QLabel("L Speed", this);
    labelSpeedR    = new QLabel("R Speed", this);
    editMoveSpeedL = new QLineEdit("0.0", this);
    editMoveSpeedR = new QLineEdit("0.0", this);
    editMoveSpeedL->setAlignment(Qt::AlignRight);
    editMoveSpeedR->setAlignment(Qt::AlignRight);

    labelKp       = new QLabel("Kp", this);
    labelKi       = new QLabel("Ki", this);
    labelKd       = new QLabel("Kd", this);
    labelSetpoint = new QLabel("Setpoint", this);
    editKp        = new QLineEdit("1.0", this);
    editKi        = new QLineEdit("0.0", this);
    editKd        = new QLineEdit("0.0", this);
    editSetpoint  = new QLineEdit("0.0", this);

    editKp->setAlignment(Qt::AlignRight);
    editKi->setAlignment(Qt::AlignRight);
    editKd->setAlignment(Qt::AlignRight);
    editSetpoint->setAlignment(Qt::AlignRight);

    statusBar = new QStatusBar(this);

    pGLWidget = new GLWidget(this);
    createPlot();

    connect(buttonClose, SIGNAL(clicked()),
            this, SLOT(onButtonClosePushed()));
    connect(buttonManualControl, SIGNAL(clicked()),
            this, SLOT(onButtonManualPushed()));
    connect(buttonConnect, SIGNAL(clicked()),
            this, SLOT(onConnectToClient()));
    connect(buttonMove, SIGNAL(clicked()),
            this, SLOT(onStartMovePushed()));
    connect(buttonSetPid, SIGNAL(clicked()),
            this, SLOT(onSetPIDPushed()));

    setDisableUI(true);
}


void
MainWidget::restoreSettings() {
    QSettings settings;
    // Tcp Server
    QString sServer = settings.value("tcpServer", "raspberrypi.local").toString();
    editHostName->setText(sServer);
}


void
MainWidget::saveSettings() {
    QSettings settings;
    settings.setValue("tcpServer", editHostName->text());
}


void
MainWidget::setDisableUI(bool bDisable) {
    buttonClose->setDisabled(bDisable);
    buttonManualControl->setDisabled(bDisable);
    buttonMove->setDisabled(bDisable);
    buttonSetPid->setDisabled(bDisable);

    editKp->setDisabled(bDisable);
    editKi->setDisabled(bDisable);
    editKd->setDisabled(bDisable);
    editSetpoint->setDisabled(bDisable);

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
//
    firstButtonRow->addWidget(labelSpeedL);
    firstButtonRow->addWidget(editMoveSpeedL);
    firstButtonRow->addWidget(labelSpeedR);
    firstButtonRow->addWidget(editMoveSpeedR);
    firstButtonRow->addWidget(buttonMove);
//
    firstButtonRow->addWidget(labelKp);
    firstButtonRow->addWidget(editKp);
    firstButtonRow->addWidget(labelKi);
    firstButtonRow->addWidget(editKi);
    firstButtonRow->addWidget(labelKd);
    firstButtonRow->addWidget(editKd);
    firstButtonRow->addWidget(labelSetpoint);
    firstButtonRow->addWidget(editSetpoint);
    firstButtonRow->addWidget(buttonSetPid);

    secondButtonRow = new QHBoxLayout;
    secondButtonRow->addWidget(labelHost);
    secondButtonRow->addWidget(editHostName);
    secondButtonRow->addWidget(buttonConnect);
    secondButtonRow->addWidget(buttonClose);
    secondButtonRow->addWidget(buttonManualControl);

//    thirdButtonRow = new QHBoxLayout;

    QHBoxLayout *firstRow = new QHBoxLayout;
    firstRow->addWidget(pGLWidget);
    firstRow->addWidget(pPlotVal);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(firstRow);
    mainLayout->addLayout(firstButtonRow);
    mainLayout->addLayout(secondButtonRow);
//    mainLayout->addLayout(thirdButtonRow);
    mainLayout->addWidget(statusBar);

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
            statusBar->showMessage(QString("Connecting to: %1").arg(hostInfo.hostName()));
            tcpClient.connectToHost(serverAddress, 43210);
        }
        else {
            statusBar->showMessage(QString(hostInfo.errorString()));
            buttonConnect->setEnabled(true);
            editHostName->setEnabled(true);
        }
    } else {
        statusBar->showMessage(QString(hostInfo.errorString()));
        buttonConnect->setEnabled(true);
        editHostName->setEnabled(true);
    }
}


void
MainWidget::displayError(QAbstractSocket::SocketError socketError) {
    if(socketError == QTcpSocket::RemoteHostClosedError) {
        statusBar->showMessage(QString("The remote host has closed the connection"));
        tcpClient.close();
        return;
    }
    statusBar->showMessage(QString(tcpClient.errorString()));
    tcpClient.close();
    buttonConnect->setEnabled(true);
    editHostName->setEnabled(true);
}


void
MainWidget::onServerConnected() {
    statusBar->showMessage(QString("Connected"));
    askConfiguration(); // Get Current Robot Configuration

    setDisableUI(false);

    buttonConnect->setText("Disconnect");
    buttonConnect->setEnabled(true);

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
        if(tokens.count() > 1) {
            double x = tokens.at(0).toDouble();
            double input = tokens.at(1).toDouble();
            pPlotVal->NewPoint(4, x, double(input));
            if(tokens.count() == 3) {
                double output = tokens.at(2).toDouble();
                pPlotVal->NewPoint(5, x, double(output));
            }
            else
                pPlotVal->ClearDataSet(5);
        }
    }
    else if(cmd == 'c') { // Robot Configuration Values
        if(tokens.count() == 6) {
            editKp->setText(tokens.at(0));
            editKi->setText(tokens.at(1));
            editKd->setText(tokens.at(2));
            //motorSpeedFactorLeft  = tokens.at(3).toDouble();
            //motorSpeedFactorRight = tokens.at(4).toDouble();
            editSetpoint->setText(tokens.at(5));
        }
    }
}


void
MainWidget::onButtonClosePushed() {
    if(tcpClient.isOpen()) {
        message.clear();
        message.append("K#"); // Kill Remote Program
        tcpClient.write(message);
    }
}


void
MainWidget::onButtonManualPushed() {
    if(tcpClient.isOpen()) {
        message.clear();
        if(bPIDInControl) {
            message.append("S#"); // Set Manual Control
            tcpClient.write(message);
            bPIDInControl = false;
            buttonManualControl->setText("PID Ctrl");
            setDisableUI(false);
        }
        else {
            message.append("G#"); // Go !
            tcpClient.write(message);
            bPIDInControl = true;
            buttonManualControl->setText("Manual Control");
            setDisableUI(true);
            buttonSetPid->setEnabled(true);

            editKp->setEnabled(true);
            editKi->setEnabled(true);
            editKd->setEnabled(true);
            buttonManualControl->setEnabled(true);
        }
    }
}


void
MainWidget::askConfiguration() {
    if(tcpClient.isOpen()) {
        message.clear();
        message.append("C#"); // Ask Robot Configuration
        tcpClient.write(message);
    }
}


void
MainWidget::onStartMovePushed() {
    if(tcpClient.isOpen()) {
        QString sMessage = QString("M %1 %2#")
                .arg(editMoveSpeedL->text(), editMoveSpeedR->text()); // Start Moving
        tcpClient.write(sMessage.toLatin1());
    }
}


void
MainWidget::onSetPIDPushed() {
    if(tcpClient.isOpen()) {
        QString sMessage = QString("P %1 %2 %3 %4#")
                .arg(editKp->text(),
                     editKi->text(),
                     editKd->text(),
                     editSetpoint->text());
        tcpClient.write(sMessage.toLatin1());
    }
}
