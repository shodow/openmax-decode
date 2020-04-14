#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <OMXH264Player.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int func();
private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
    OMXH264Player *m_player;
};

#endif // MAINWINDOW_H
