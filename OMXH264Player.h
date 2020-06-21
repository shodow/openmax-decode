#ifndef OMXH264PLAYER_H
#define OMXH264PLAYER_H
 
#include <QObject>
#include "bcm_host.h"
#include <QImage>
#include <thread>
#include <QQueue>
extern "C"{
    #include "ilclient.h"
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libavdevice/avdevice.h"
}

#include <mutex>

class OMXH264Player: public QObject
{
   Q_OBJECT
public:
    struct DataStruct{
        unsigned char* data; // 解码数据,h264或PCM
        int size; // 数据长度
        int type; // 数据类型 0-视频 1-音频
    };
signals:
    void sendImg(uchar*,uint,uint);
public:
    OMXH264Player(QObject *parent = 0);
    ~OMXH264Player();

    ///
    /// \brief initAudio
    ///
    void initAudio();

    ///
    /// \brief set_audio_render_input_format
    /// \param audioRender
    ///
    void set_audio_render_input_format(COMPONENT_T *audioRender);

    ///
    /// \brief setOutputDevice
    /// \param handle
    /// \param name
    ///
    void setOutputDevice(OMX_HANDLETYPE handle, const char *name);

    ///
    /// \brief setPCMMode
    /// \param handle
    /// \param startPortNumber
    ///
    void setPCMMode(OMX_HANDLETYPE handle, int startPortNumber);

    ///
    /// \brief draw
    /// \param image
    /// \param size
    ///
    void draw(unsigned char * image, int size);

    ///
    /// \brief playAudio
    /// \param pcmData
    /// \param len
    ///
    void playAudio(uchar *pcmData, int len);

    ///
    /// \brief initBufferQueue
    /// \param input_port
    /// \return
    ///
    int initBufferQueue(int input_port);

    ///
    /// \brief getBuffer
    /// \return
    ///
    OMX_BUFFERHEADERTYPE * getBuffer();
    void clear();

    ///
    /// \brief startPlay
    ///
    void startPlay();

    ///
    /// \brief endPlay
    ///
    void endPlay();

    ///
    /// \brief pushData
    /// \param image
    /// \param size
    /// \param type
    ///
    void pushData(unsigned char * image, int size, int type);

    ///
    /// \brief popData
    /// \return
    ///
    bool popData();
public slots:
//    void playbackTest();
 
public:
    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
    COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *video_render = NULL, *clock = NULL;
    COMPONENT_T *audio_render = NULL;
    COMPONENT_T *list[5];
    TUNNEL_T tunnel[4];
    ILCLIENT_T *client;
    FILE *in;
    int status = 0;
    OMX_BUFFERHEADERTYPE *omxbuf;
    int first_packet;
    int port_settings_changed = 0;

    bool m_run;
    std::thread *t1;
    QQueue<DataStruct> m_dataQueue;
    std::mutex m_dataLock;

    std::mutex m_bufferLock;
    QQueue<OMX_BUFFERHEADERTYPE *> m_videoBuffers;
};
 
#endif // OMXH264PLAYER_H
