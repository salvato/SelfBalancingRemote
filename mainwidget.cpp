#include "mainwidget.h"
#include "plot2d.h"
#include <QDebug>
#include <QThread>
#include <GLwidget.h>
#include <QPushButton>
#include <QSettings>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    // Widgets
    , pGLWidget(nullptr)
    , pPlotVal(nullptr)
{
}


MainWidget::~MainWidget()
{
}

