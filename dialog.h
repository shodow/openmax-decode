#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QImage>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();
public slots:
    void showImg(QImage img);
private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
