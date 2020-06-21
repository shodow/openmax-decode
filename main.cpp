#include "mainwindow.h"
#include <QApplication>
#include <QImage>
#include "form.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<QImage>("QImage");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
//    Form f;
//    f.show();

    return a.exec();
}
