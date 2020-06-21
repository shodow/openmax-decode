#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dialog.h"
#include "camera.h"
#include <QMainWindow>
#include <OMXH264Player.h>
#include "OMXAudio.h"
#include "GLYuvWidget.h"
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
    void on_pushButton_3_clicked();

public:
    QString m_url;
    OMXH264Player *m_player;
//    OMXAudio *m_audio;
    Dialog *dialog;
    QImage *img;
    GLYuvWidget *m_pGLYuvWidget;
private:
    Ui::MainWindow *ui;

    std::thread *td;

    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event);
};

#endif // MAINWINDOW_H
