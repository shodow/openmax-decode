#include "mainwindow.h"
#include <QApplication>
#include <QImage>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QImage>("QImage");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
