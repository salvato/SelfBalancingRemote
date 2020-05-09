#include "mainwidget.h"
#include "plot2d.h"
#include "GLwidget.h"

#include <QDebug>
#include <QThread>
#include <QPushButton>
#include <QLineEdit>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    // Widgets
    , pGLWidget(nullptr)
    , pPlotVal(nullptr)
    , bRunInProgress(false)
    , bShowPidInProgress(false)
{
    initLayout();

    // Network events
    connect(&tcpClient, SIGNAL(connected()),
            this, SLOT(onServerConnected()));
    connect(&tcpClient, SIGNAL(disconnected()),
            this, SLOT(onServerDisconnected()));
    connect(&tcpClient, SIGNAL(readyRead()),
            this, SLOT(onNewDataAvailable()));
    connect(&tcpClient, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
}


MainWidget::~MainWidget() {
}


void
MainWidget::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event)
}


void
MainWidget::keyPressEvent(QKeyEvent *e) {
  if(e->key() == Qt::Key_Escape)
    close();
  else
    QWidget::keyPressEvent(e);
}


void
MainWidget::createButtons() {
    buttonStartStop       = new QPushButton("Start",     this);
    buttonAccCalibration  = new QPushButton("Acc. Cal.", this);
    buttonGyroCalibration = new QPushButton("Gyro Cal.", this);
    buttonMagCalibration  = new QPushButton("Mag. Cal.", this);
    buttonShowPidOutput   = new QPushButton("Show PID",  this);
    buttonHide3D          = new QPushButton("Hide3D",    this);
    buttonConnect         = new QPushButton("Connect",   this);

    buttonStartStop->setEnabled(false);
    buttonHide3D->setEnabled(false);
    buttonAccCalibration->setEnabled(false);
    buttonGyroCalibration->setEnabled(false);
    buttonMagCalibration->setEnabled(false);
    buttonShowPidOutput->setEnabled(false);

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
    connect(buttonHide3D, SIGNAL(clicked()),
            this, SLOT(onHide3DPushed()));
    connect(buttonConnect, SIGNAL(clicked()),
            this, SLOT(onConnectToClient()));
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
}


void
MainWidget::initLayout() {
    createButtons();
    editHostName = new QLineEdit("raspberrypi.local", this);
    pGLWidget = new GLWidget(this);
    createPlot();
    firstButtonRow = new QHBoxLayout;
    secondButtonRow = new QHBoxLayout;

    firstButtonRow->addWidget(buttonStartStop);
    firstButtonRow->addWidget(buttonHide3D);
    firstButtonRow->addWidget(buttonAccCalibration);
    firstButtonRow->addWidget(buttonGyroCalibration);
    firstButtonRow->addWidget(buttonMagCalibration);
    firstButtonRow->addWidget(buttonShowPidOutput);

    secondButtonRow->addWidget(editHostName);
    secondButtonRow->addWidget(buttonConnect);

    QHBoxLayout *firstRow = new QHBoxLayout;
    firstRow->addWidget(pGLWidget);
    firstRow->addWidget(pPlotVal);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(firstRow);
    mainLayout->addLayout(firstButtonRow);
    mainLayout->addLayout(secondButtonRow);
    setLayout(mainLayout);
}


void
MainWidget::onConnectToClient() {
    if(buttonConnect->text() == tr("Connect")) {
        buttonConnect->setEnabled(false);
        editHostName->setEnabled(false);
        QHostInfo::lookupHost(editHostName->text(), this, SLOT(handleLookup(QHostInfo)));
    } else {//pButtonConnect->text() == tr("Disconnect")
        tcpClient.close();
    }
}


void
MainWidget::handleLookup(QHostInfo hostInfo) {
    // Handle the results.
    if(hostInfo.error() == QHostInfo::NoError) {
        serverAddress = hostInfo.addresses().first();
        qDebug() << QString("Connecting to: %1").arg(hostInfo.hostName());
        bytesWritten = 0;
        bytesReceived = 0;
        tcpClient.connectToHost(serverAddress, 43210);
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

    buttonStartStop->setEnabled(true);
    buttonHide3D->setEnabled(true);
    buttonAccCalibration->setEnabled(true);
    buttonGyroCalibration->setEnabled(true);
    buttonMagCalibration->setEnabled(true);
    buttonShowPidOutput->setEnabled(true);
}


void
MainWidget::onServerDisconnected() {
    qDebug() << QString("Disconnected");
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
        //qDebug() << QString(sNewCommand + " Received");
        executeCommand(sNewCommand);
        receivedCommand = receivedCommand.mid(iPos+1);
        iPos = receivedCommand.indexOf("#");
    }
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
            pGLWidget->update();
        }
    } else if(cmd == 'p') { // PID Input & Output values
        if(tokens.count() == 3) {
            double x = tokens.at(0).toDouble();
            double input = tokens.at(1).toDouble();
            double output = tokens.at(2).toDouble();
            pPlotVal->NewPoint(4, x, double(input));
            pPlotVal->NewPoint(5, x, double(output));
            pPlotVal->UpdatePlot();
        }
//    } else if(command.contains(QString("alive"))) {
//        watchDogTimer.start(watchDogTime);
    }
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
            buttonAccCalibration->setEnabled(false);
            buttonGyroCalibration->setEnabled(false);
            buttonMagCalibration->setEnabled(false);
            buttonShowPidOutput->setEnabled(false);
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
MainWidget::onShowPidOutput() {
    if(bShowPidInProgress) {
        if(tcpClient.isOpen()) {
            message.clear();
            message.append("N#"); // Stop PID!
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
            message.append("P#"); // Show PID
            tcpClient.write(message);
            bShowPidInProgress = false;

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
