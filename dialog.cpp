#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    setAutoFillBackground(true);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::showImg(QImage img)
{
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(img.scaled(this->size())));
    this->setPalette(palette);
}
