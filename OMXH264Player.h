#ifndef OMXH264PLAYER_H
#define OMXH264PLAYER_H
 
#include <QObject>
#include "bcm_host.h"
#include <QImage>
extern "C"{
    #include "ilclient.h"
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libavdevice/avdevice.h"
}
 
class OMXH264Player: public QObject
{
   Q_OBJECT
signals:
    void sendImg(uchar*,uint,uint);
public:
    OMXH264Player(QObject *parent = 0);
    ~OMXH264Player();
    void draw( unsigned char * image, int size);
    void clear();
public slots:
    void playbackTest();
 
public:
    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    COMPONENT_T *video_decode;
    COMPONENT_T *list[2];
    // TUNNEL_T tunnel[1];
    ILCLIENT_T *client;
    int port_settings_changed;
    int first_packet;
    OMX_BUFFERHEADERTYPE *omxbuf;
    FILE *fd;
    int width, height;


};
 
#endif // OMXH264PLAYER_H
