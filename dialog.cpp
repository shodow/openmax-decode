#include "dialog.h"
#include "ui_dialog.h"
#include <QTime>
#include <QDebug>
Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    videoW(0),
    videoH(0)
{
    ui->setupUi(this);
    setAutoFillBackground(true);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::slotShowYuv(uchar *ptr, uint width, uint height)
{
    static int i = 0;
    i += 1;
    yuvPtr = ptr;
    if (videoW != width || videoH != height)
    {
        int yuvSize = width * height * 3 /2;
        yuvBuffer = (uint8_t *)malloc(yuvSize);
        //为每帧图像分配内存
        pFrame = av_frame_alloc();
        pFrameRGB = av_frame_alloc();
        int numBytes = avpicture_get_size(AV_PIX_FMT_RGB32,
                                          width,height);
        rgbBuffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
        avpicture_fill((AVPicture *) pFrameRGB, rgbBuffer,
                       AV_PIX_FMT_RGB32,width, height);
        //特别注意 img_convert_ctx 该在做H264流媒体解码时候，发现sws_getContext /sws_scale内存泄露问题，
        //注意sws_getContext只能调用一次，在初始化时候调用即可，另外调用完后，在析构函数中使用sws_free_Context，将它的内存释放。
        //设置图像转换上下文
        img_convert_ctx = sws_getContext(width, height,
                                         AV_PIX_FMT_YUV420P,
                                         width, height,
                                         AV_PIX_FMT_RGB32,
                                         SWS_BICUBIC,
                                         NULL, NULL, NULL);
    }
    videoW = width;
    videoH = height;

    update();
}

void Dialog::paintEvent(QPaintEvent *event)
{
    if (videoW >0 && videoH > 0)
    {
        QTime time;
        time.start();
        avpicture_fill((AVPicture *) pFrame, (uint8_t *)yuvPtr,
                       AV_PIX_FMT_YUV420P, videoW, videoH);//这里的长度和高度跟之前保持一致
        //转换图像格式，将解压出来的YUV420P的图像转换为RGB的图像
        sws_scale(img_convert_ctx,
                  (uint8_t const * const *) pFrame->data,
                  pFrame->linesize, 0, videoH, pFrameRGB->data,
                  pFrameRGB->linesize);
        //把这个RGB数据 用QImage加载
        QImage tmpImg((uchar *)rgbBuffer, videoW, videoH, QImage::Format_RGB32);

        QPalette palette;
        palette.setBrush(QPalette::Window, QBrush(tmpImg.scaled(this->size())));
        this->setPalette(palette);
        qDebug() << "YUV2RGB use time: " << time.elapsed() << "ms";
    }
}
