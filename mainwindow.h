#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dialog.h"
#include <QMainWindow>
#include <OMXH264Player.h>
#include <thread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
//    int func();
private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();
public:
    OMXH264Player *m_player;
    Dialog *dialog;
    QImage *img;
private:
    Ui::MainWindow *ui;

    std::thread *td;
};

#endif // MAINWINDOW_H
