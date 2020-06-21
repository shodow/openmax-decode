#ifndef OMXAUDIO_H
#define OMXAUDIO_H

#include <QObject>
#include "bcm_host.h"
extern "C"{
    #include "ilclient.h"
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libavdevice/avdevice.h"
}

class OMXAudio : public QObject
{
    Q_OBJECT
public:
    OMXAudio(QObject *parent = nullptr);
    ~OMXAudio();
    int init();
    void playAudio(uchar *pcmData, int len);
protected:
    void setOutputDevice(OMX_HANDLETYPE handle, const char *name);
    void setPCMMode(OMX_HANDLETYPE handle, int startPortNumber);
    void set_audio_render_input_format(COMPONENT_T *audioRender);
    OMX_ERRORTYPE read_into_buffer_and_empty(uchar *pcmData, int len, COMPONENT_T *audioRender,
                                             OMX_BUFFERHEADERTYPE *buff_header);
private:
    COMPONENT_T *audioRender;
};
#endif // OMXAUDIO_H
