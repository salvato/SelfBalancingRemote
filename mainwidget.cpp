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
{
    initLayout();
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

    buttonStartStop->setEnabled(true);
    buttonHide3D->setEnabled(true);
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
    editHostName         = new QLineEdit("192.168.1.123", this);
    pGLWidget = new GLWidget(this);
    createPlot();
    QHBoxLayout *firstButtonRow = new QHBoxLayout;
    firstButtonRow->addWidget(buttonStartStop);
    firstButtonRow->addWidget(buttonHide3D);
    firstButtonRow->addWidget(buttonAccCalibration);
    firstButtonRow->addWidget(buttonGyroCalibration);
    firstButtonRow->addWidget(buttonMagCalibration);
    firstButtonRow->addWidget(buttonShowPidOutput);

    QHBoxLayout *firstRow = new QHBoxLayout;
    firstRow->addWidget(pGLWidget);
    firstRow->addWidget(pPlotVal);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(firstRow);
    mainLayout->addLayout(firstButtonRow);
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
    if(command.at(0) == 'q') {
        //        QStringList tokens = command.split(' ');
        //        tokens.removeFirst();
        //        if(tokens.count() == 8) {
        //            int iSensorNumber = tokens.at(0).toInt();
        //            if(iSensorNumber >= boxes.count()) {
        //                for(int i=boxes.count(); i<=iSensorNumber; i++) {
        //                    boxes.append(new Shimmer3Box());
        //                }
        //            }
        //            if(iSensorNumber < boxes.count()) {
        //                Shimmer3Box* pBox = boxes[iSensorNumber];
        //                pBox->x      = tokens.at(1).toDouble();
        //                pBox->y      = tokens.at(2).toDouble();
        //                pBox->z      = tokens.at(3).toDouble();
        //                pBox->pos[0] = tokens.at(4).toDouble();
        //                pBox->pos[1] = tokens.at(5).toDouble();
        //                pBox->pos[2] = tokens.at(6).toDouble();
        //                pBox->angle  = tokens.at(7).toDouble();
        //                updateWidgets();
        //            }
        //        }

        //    } else if(command.contains(QString("depth"))) {
        //        QStringList tokens = command.split(' ');
        //        tokens.removeFirst();
        //        double depth = tokens.at(0).toInt()/100.0;// Now in meters
        //        //      qDebug() << "Depth= " << depth;
        //        pDepth->setValue(tokens.at(0).toInt());
        //        QString sDepth;
        //        sDepth.sprintf("%.2f", depth);
        //        pDepthEdit->setText(sDepth);

        //    } else if(command.contains(QString("alive"))) {
        //        watchDogTimer.start(watchDogTime);
    }
}

