#include "mainwidget.h"

#include <QApplication>
#include <QIcon>

int
main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":10_DOF.png"));
    MainWidget w;
    w.show();
    return a.exec();
}
