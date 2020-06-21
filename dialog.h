#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QImage>
extern "C"{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libavdevice/avdevice.h"
}
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
    void slotShowYuv(uchar *ptr, uint width, uint height);
    void showRgb();
private:
    Ui::Dialog *ui;
    uint videoW,videoH;
    uchar *yuvPtr = nullptr;
    uint8_t *yuvBuffer;
    AVFrame *pFrame ;
    AVFrame *pFrameRGB;
    uint8_t * rgbBuffer;
    SwsContext *img_convert_ctx;
    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event);
};

#endif // DIALOG_H
