#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <thread>
#include <QTime>
#include <QDebug>
#include <unistd.h>

#include <stdio.h>

#define __STDC_CONSTANT_MACROS
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include <libswresample/swresample.h>
}

//'1': Use H.264 Bitstream Filter
#define USE_H264BSF 1

int func(void *param)
{
    MainWindow *pthis = static_cast<MainWindow*> (param);
    AVFormatContext *ifmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    int videoindex=-1,audioindex=-1;

    // Audio
    AVCodecContext *pCodecCtx;
    AVCodec			*pCodec;
    uint8_t			*out_buffer;
    AVFrame			*pFrame;
    int64_t in_channel_layout;
    struct SwrContext *au_convert_ctx;
    FILE *pFile=NULL;

    qDebug() << pthis->m_url;

    av_register_all();
    //Network
    avformat_network_init();
    //Input
    if ((ret = avformat_open_input(&ifmt_ctx, pthis->m_url.toUtf8(), 0, 0)) < 0) {
        printf( "Could not open input file.");
        return -1;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf( "Failed to retrieve input stream information");
        return -1;
    }

    for(i=0; i<ifmt_ctx->nb_streams; i++) {
        if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoindex=i;
        }else if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            audioindex=i;
        }
    }
    //Dump Format------------------
    printf("\nInput Video===========================\n");
    av_dump_format(ifmt_ctx, 0, pthis->m_url.toUtf8(), 0);
    printf("\n======================================\n");


    // 初始化音频解码器
    // Get a pointer to the codec context for the audio stream
    pCodecCtx=ifmt_ctx->streams[audioindex]->codec;

    // Find the decoder for the audio stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL){
        printf("Codec not found.\n");
        return -1;
    }

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
        printf("Could not open codec.\n");
        return -1;
    }

    //Out Audio Param
    uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
    //nb_samples: AAC-1024 MP3-1152
    int out_nb_samples=pCodecCtx->frame_size;
    AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
    int out_sample_rate=44100;
    int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
    //Out Buffer Size
    int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);

    out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
    pFrame=av_frame_alloc();

    //FIX:Some Codec's Context Information is missing
    in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
    //Swr

    au_convert_ctx = swr_alloc();
    au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,
        in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);
    swr_init(au_convert_ctx);



    /*
    FIX: H.264 in some container format (FLV, MP4, MKV etc.) need
    "h264_mp4toannexb" bitstream filter (BSF)
      *Add SPS,PPS in front of IDR frame
      *Add start code ("0,0,0,1") in front of NALU
    H.264 in some container (MPEG2TS) don't need this BSF.
    */
    int tmpBuffOff = 0;
    unsigned char* tmpBuff = new unsigned char[81920 * 50];
    unsigned char* tmpBuff2 = new unsigned char[81920];
    memset(tmpBuff, 0, 81920 * 50);
    memset(tmpBuff2, 0, 81920);
//    FILE *fp = fopen("h264", "wb");
#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");
#endif
    QTime time;
    pthis->m_player->startPlay();
    while(av_read_frame(ifmt_ctx, &pkt)>=0){
        if(pkt.stream_index==videoindex){
//            time.start();
#if USE_H264BSF
            av_bitstream_filter_filter(h264bsfc, ifmt_ctx->streams[videoindex]->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
            memcpy(tmpBuff + tmpBuffOff, pkt.data, pkt.size);
            tmpBuffOff += pkt.size;
//            fwrite(pkt.data,1,pkt.size,fp_video);
            // 数据长度不够81920的等待之后的数据组成81920长度的数据
            // 数据长度超过81920的数据切割成多个包
            if (tmpBuffOff > 81920)
            {
                int tmp = tmpBuffOff;
                int offset = 0;
                while(tmp > 81920)
                {
                    // 推到解码队列中等待解码
                    pthis->m_player->pushData(tmpBuff + offset, 81920, 0);
//                    fwrite(tmpBuff + offset, 1, 81920, fp);
                    offset += 81920;
                    tmp -= 81920;
                }
                if (tmp > 0)
                {
                    memmove(tmpBuff2, tmpBuff + offset, tmp);
                    memset(tmpBuff, 0, 81920 * 50);
                    memmove(tmpBuff, tmpBuff2, tmp);
                    tmpBuffOff = tmp;
                }
            }
        }else if(pkt.stream_index==audioindex){
            /*
            AAC in some container format (FLV, MP4, MKV etc.) need to add 7 Bytes
            ADTS Header in front of AVPacket data manually.
            Other Audio Codec (MP3...) works well.
            */

            int got_picture = -1;
            ret = avcodec_decode_audio4(pCodecCtx, pFrame, &got_picture, &pkt);
            if ( ret < 0 ) {
                qDebug("Error in decoding audio frame.\n");
            }
            if ( got_picture > 0 ){
                swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE,
                            (const uint8_t **)pFrame->data, pFrame->nb_samples);
                // 推入到音频解码队列中等待解码音频数据
                pthis->m_player->pushData(out_buffer, out_buffer_size, 1);
            }
            else {
                qDebug() << "got_picture: " << got_picture;
            }
        }
        av_free_packet(&pkt);
    }
    pthis->m_player->endPlay();
#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif

//    fclose(fp_video);
//    fclose(fp_audio);
    fclose(pFile);

    avformat_close_input(&ifmt_ctx);

    if (ret < 0 && ret != AVERROR_EOF) {
        printf( "Error occurred.\n");
        return -1;
    }
    return 0;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground);
    m_player = new OMXH264Player();
//    m_audio = new OMXAudio();
//    m_pGLYuvWidget = new GLYuvWidget();
//    dialog = new Dialog();
    setWindowOpacity(0);

//    m_url = "http://192.168.1.140:9090/videoplay/test1080p.mp4";
//    td = new std::thread(func, this);
//    dialog->showFullScreen();
//    connect(m_player, SIGNAL(sendImg(uchar*,uint,uint)),
//            dialog, SLOT(slotShowYuv(uchar*,uint,uint)));
    on_pushButton_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
//    m_url = "http://192.168.1.140:9090/videoplay/" + ui->lineEdit->text();
    m_url = "http://192.168.3.14:8890/7.mp4";
    td = new std::thread(func, this);
    this->hide();
}

void MainWindow::on_pushButton_2_clicked()
{
//    dialog->showFullScreen();
//    connect(m_player, SIGNAL(sendImg(uchar*,uint,uint)),
//            dialog, SLOT(slotShowYuv(uchar*,uint,uint)));
}

void MainWindow::on_pushButton_3_clicked()
{
//    m_pGLYuvWidget->show();
//    connect(m_player, SIGNAL(sendImg(uchar*,uint,uint)),
    //            m_pGLYuvWidget, SLOT(slotShowYuv(uchar*,uint,uint)));
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    // QColor最后一个参数代表alpha通道，一般用作透明度
    painter.fillRect(rect(), QColor(0, 0, 0, 0));
}
