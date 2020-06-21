#include "OMXH264Player.h"
#include <QDebug>
#include <QTime>

void emptyBufferDoneCallback(void *data, COMPONENT_T *comp)
{
    qDebug() << "emptyBufferDoneCallback";
}

void run(void *param)
{
    OMXH264Player *pThis = static_cast<OMXH264Player*>(param);
    bool ret = false;
    while (pThis->m_run)
    {
        ret = pThis->popData();
//        if (!ret)
//        {

//            usleep(1000 * 10);
//        }
    }
    while (pThis->popData());
    qDebug() << "run over";
}

OMXH264Player::OMXH264Player(QObject *parent):
    QObject(parent),
    video_decode(NULL),
    port_settings_changed(0),
    first_packet(1),
    omxbuf(NULL),
    m_run(false),
    t1(nullptr)
{
    memset(list, 0, sizeof(list));
    memset(tunnel, 0, sizeof(tunnel));

    if((client = ilclient_init()) == NULL)
    {
       qDebug() << "ilclient_init error";
       fclose(in);
    }

    if(OMX_Init() != OMX_ErrorNone)
    {
       qDebug() << "OMX_Init error";
       ilclient_destroy(client);
       fclose(in);
    }

    // create video_decode
    if(ilclient_create_component(client, &video_decode, "video_decode",
                                 (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS)) != 0)
    {
       qDebug() << "video_decode error";
       status = -14;
    }
    list[0] = video_decode;

    // create video_render
    if(status == 0 && ilclient_create_component(client, &video_render, "video_render",
                                                ILCLIENT_DISABLE_ALL_PORTS) != 0)
    {
       qDebug() << "video_render error";
       status = -14;
    }
    list[1] = video_render;

    // create clock
    if(status == 0 && ilclient_create_component(client, &clock, "clock",
                                                ILCLIENT_DISABLE_ALL_PORTS) != 0)
    {
       qDebug() << "clock error";
       status = -14;
    }
    list[2] = clock;

    memset(&cstate, 0, sizeof(cstate));
    cstate.nSize = sizeof(cstate);
    cstate.nVersion.nVersion = OMX_VERSION;
    cstate.eState = OMX_TIME_ClockStateRunning;
//    cstate.nStartTime = 100000;
//    cstate.nOffset = 10000;
    cstate.nWaitMask = 1;
    if(clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(clock),
                                         OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
    {
       qDebug() << "clock OMX_SetParameter error";
       status = -13;
    }

    // create video_scheduler
    if(status == 0 && ilclient_create_component(client, &video_scheduler, "video_scheduler",
                                                ILCLIENT_DISABLE_ALL_PORTS) != 0)
    {
       qDebug() << "video_scheduler error";
       status = -14;
    }
    list[3] = video_scheduler;

    // create audio_render
    initAudio();
    list[4] = audio_render;

    set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
    set_tunnel(tunnel+1, video_scheduler, 11, video_render, 90);
    set_tunnel(tunnel+2, clock, 80, video_scheduler, 12);
    set_tunnel(tunnel+3, clock, 81, audio_render, 101);

    // setup clock tunnel first
    if(status == 0 && ilclient_setup_tunnel(tunnel+2, 0, 0) != 0)
    {
       qDebug() << "ilclient_setup_tunnel error";
       status = -15;
    }
    else
       ilclient_change_component_state(clock, OMX_StateExecuting);

    // setup clock tunnel first
    if(status == 0 && ilclient_setup_tunnel(tunnel+3, 0, 0) != 0)
    {
       qDebug() << "ilclient_setup_tunnel error";
       status = -15;
    }
    else
       ilclient_change_component_state(clock, OMX_StateExecuting);

    if(status == 0)
       ilclient_change_component_state(video_decode, OMX_StateIdle);

    memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    format.nVersion.nVersion = OMX_VERSION;
    format.nPortIndex = 130;
    format.eCompressionFormat = OMX_VIDEO_CodingAVC;

    if (OMX_SetParameter(ILC_GET_HANDLE(video_decode),
                         OMX_IndexParamVideoPortFormat, &format) != OMX_ErrorNone)
    {
       qDebug() << "video_decode OMX_SetParameter error";
    }



    if (ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) != 0)
    {
       qDebug() << "video_decode ilclient_enable_port_buffers error";
    }

    initBufferQueue(130);

    ilclient_change_component_state(video_decode, OMX_StateExecuting);
//    ilclient_set_empty_buffer_done_callback(client, emptyBufferDoneCallback, this);
}
 
OMXH264Player::~OMXH264Player(){
    qDebug()<<"OMXH264Player destructor";
}

void OMXH264Player::setPCMMode(OMX_HANDLETYPE handle, int startPortNumber) {
    OMX_AUDIO_PARAM_PCMMODETYPE sPCMMode;
    OMX_ERRORTYPE err;
    memset(&sPCMMode, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
    sPCMMode.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    sPCMMode.nVersion.nVersion = OMX_VERSION;
    sPCMMode.nPortIndex = startPortNumber;
    err = OMX_GetParameter(handle, OMX_IndexParamAudioPcm, &sPCMMode);
    printf("Sampling rate %d, channels %d\n", sPCMMode.nSamplingRate, sPCMMode.nChannels);
    sPCMMode.nSamplingRate = 44100;
    sPCMMode.nChannels = 2;
    err = OMX_SetParameter(handle, OMX_IndexParamAudioPcm, &sPCMMode);
    if(err != OMX_ErrorNone){
        fprintf(stderr, "PCM mode unsupported\n");
        return;
    } else {
        fprintf(stderr, "PCM mode supported\n");
        fprintf(stderr, "PCM sampling rate %d\n", sPCMMode.nSamplingRate);
        fprintf(stderr, "PCM nChannels %d\n", sPCMMode.nChannels);
    }
}

void OMXH264Player::set_audio_render_input_format(COMPONENT_T *audioRender) {
    // set input audio format
    printf("Setting audio render format\n");
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioPortFormat;
    //setHeader(&audioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
    memset(&audioPortFormat, 0, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
    audioPortFormat.nSize = sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE);
    audioPortFormat.nVersion.nVersion = OMX_VERSION;
    audioPortFormat.nPortIndex = 100;
    OMX_GetParameter(ilclient_get_handle(audioRender), OMX_IndexParamAudioPortFormat, &audioPortFormat);
    audioPortFormat.eEncoding = OMX_AUDIO_CodingPCM;
    //audioPortFormat.eEncoding = OMX_AUDIO_CodingMP3;
    OMX_SetParameter(ilclient_get_handle(audioRender), OMX_IndexParamAudioPortFormat, &audioPortFormat);
    setPCMMode(ilclient_get_handle(audioRender), 100);
}

/* For the RPi name can be "hdmi" or "local" */
void OMXH264Player::setOutputDevice(OMX_HANDLETYPE handle, const char *name) {
    OMX_ERRORTYPE err;
    OMX_CONFIG_BRCMAUDIODESTINATIONTYPE arDest;
    if (name && strlen(name) < sizeof(arDest.sName)) {
        memset(&arDest, 0, sizeof(OMX_CONFIG_BRCMAUDIODESTINATIONTYPE));
        arDest.nSize = sizeof(OMX_CONFIG_BRCMAUDIODESTINATIONTYPE);
        arDest.nVersion.nVersion = OMX_VERSION;
        strcpy((char *)arDest.sName, name);
        err = OMX_SetParameter(handle, OMX_IndexConfigBrcmAudioDestination, &arDest);
        if (err != OMX_ErrorNone) {
            fprintf(stderr, "Error on setting audio destination\n");
            exit(1);
        }
    }
}

void OMXH264Player::initAudio()
{
    int err;
    err = ilclient_create_component(client, &audio_render, "audio_render", (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS));
    if (err == -1) {
        fprintf(stderr, "Component create failed\n");
        exit(1);
    }

    err = ilclient_change_component_state(audio_render, OMX_StateIdle);
    if (err < 0) {
        fprintf(stderr, "Couldn't change state to Idle\n");
        exit(1);
    }

    // must be before we enable buffers
    set_audio_render_input_format(audio_render);
    setOutputDevice(ilclient_get_handle(audio_render), "local");
    // input port
    ilclient_enable_port_buffers(audio_render, 100, NULL, NULL, NULL);
    ilclient_enable_port(audio_render, 100);
    err = ilclient_change_component_state(audio_render, OMX_StateExecuting);
    if (err < 0) {
        fprintf(stderr, "Couldn't change state to Executing\n");
        exit(1);
    }
}

void OMXH264Player::draw(unsigned char * image, int size){
    QTime time;
    time.start();
    if(size > 81920 || size < 1){
        qDebug()<<"write size larger than omxbuf size, largest 81920";
        return ;
    }

    OMX_BUFFERHEADERTYPE *buf = getBuffer();
//    OMX_BUFFERHEADERTYPE *buf = ilclient_get_input_buffer(video_decode, 130, 1);
    if (NULL == buf)
    {
        qDebug()<<"ilclient_get_input_buffer NULL!";
        return ;
    }

//    qDebug() << "ilclient_get_input_buffer use time: " << time.elapsed() << "ms";
//    time.restart();
    buf->pBuffer = image;
    if(port_settings_changed == 0 &&
       ((size > 0
         && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
        (size == 0
         && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                  ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
    {
       port_settings_changed = 1;

       if(ilclient_setup_tunnel(tunnel, 0, 0) != 0)
       {
          status = -7;
//          qDebug() << "total use time: " << time.elapsed() << "ms";
//          qDebug()<<"ilclient_setup_tunnel error!";
          m_videoBuffers.push_back(buf);
          return ;
       }

       ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

       // now setup tunnel to video_render
       if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0)
       {
          status = -12;
//          qDebug() << "total use time: " << time.elapsed() << "ms";
//          qDebug()<<"ilclient_setup_tunnel +1 error!";
          m_videoBuffers.push_back(buf);
          return ;
       }

       ilclient_change_component_state(video_render, OMX_StateExecuting);
    }
//    qDebug() << "port_settings_changed use time: " << time.elapsed() << "ms";
//    time.restart();

    buf->nFilledLen = size;

    buf->nOffset = 0;
    if(first_packet)
    {
       buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
       first_packet = 0;
    }
    else
       buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

//    qDebug() << "set info use time: " << time.elapsed() << "ms";
//    time.restart();
    if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
    {
       status = -6;
//       qDebug() << "total use time: " << time.elapsed() << "ms";
//       qDebug()<<"OMX_EmptyThisBuffer error!";
       m_videoBuffers.push_back(buf);
       return ;
    }
    qDebug() << " use time: " << time.elapsed() << "ms";
    m_videoBuffers.push_back(buf);
    return ;
}

void OMXH264Player::playAudio(uchar *pcmData, int len)
{
    OMX_BUFFERHEADERTYPE *buff_header;
    buff_header = ilclient_get_input_buffer(audio_render, 100, 1 /* block */);
    if (buff_header != NULL) {
        OMX_ERRORTYPE r;
        int buff_size = buff_header->nAllocLen;
        int nread = len;
    //    qDebug() << buff_header->nAllocLen << " " << len;
        memcpy(buff_header->pBuffer, pcmData, len);
        buff_header->nFilledLen = nread;
        r = OMX_EmptyThisBuffer(ilclient_get_handle(audio_render), buff_header);
        if (r != OMX_ErrorNone) {
            qDebug() << "" ;
        }
    }
}

int OMXH264Player::initBufferQueue(int input_port)
{
    OMX_ERRORTYPE omx_err = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE portFormat;
    portFormat.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    portFormat.nVersion.nVersion = OMX_VERSION;
    portFormat.nPortIndex = input_port;

    omx_err = OMX_GetParameter(ilclient_get_handle(video_decode), OMX_IndexParamPortDefinition, &portFormat);
    if(omx_err != OMX_ErrorNone)
    {
        qDebug() << "OMX_GetParameter error: " << omx_err;
        return omx_err;
    }
    qDebug() << "nBufferCountActual" << portFormat.nBufferCountActual << " portFormat.nBufferSize: " << portFormat.nBufferSize;
    for (size_t i = 0; i < portFormat.nBufferCountActual; i++)
    {
        OMX_BUFFERHEADERTYPE *buffer = NULL;
        omx_err = OMX_AllocateBuffer(ilclient_get_handle(video_decode), &buffer, input_port, NULL, portFormat.nBufferSize);
        if(omx_err != OMX_ErrorNone)
        {
            qDebug() << "OMX_AllocateBuffer error: " << QString::number(omx_err, 16);
            return omx_err;
        }
        buffer->nInputPortIndex = input_port;
        buffer->nFilledLen      = 0;
        buffer->nOffset         = 0;
        buffer->pAppPrivate     = (void*)i;
        m_videoBuffers.push_back(buffer);
    }
    return omx_err;
}

OMX_BUFFERHEADERTYPE *OMXH264Player::getBuffer()
{
    OMX_BUFFERHEADERTYPE *buffer = NULL;
    m_bufferLock.lock();
    if (!m_videoBuffers.isEmpty())
    {
        buffer = m_videoBuffers.front();
        m_videoBuffers.pop_front();
    }
    m_bufferLock.unlock();
    return buffer;
}
 
//void OMXH264Player::playbackTest(){
//    unsigned char* buffer = (unsigned char*)malloc(81920);
//    memset(buffer, 0, 81920);
//    FILE *in;
//    if((in = fopen("cuc_ieschool.h264", "rb")) == NULL)
//        return;
//    int read = 0;
//    while((read = fread(buffer, 1, 2000, in))>0){
//        draw(buffer, read);
//    }
//    clear();
//}
 
void OMXH264Player::clear(){
    if((omxbuf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL){
        omxbuf->nFilledLen = 0;
        omxbuf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;
        if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), omxbuf) != OMX_ErrorNone) 
            qDebug()<<"EmptyThisBuffer error";
 
        // wait for EOS from render (End of Stream Buffer Flag)
        // ilclient_wait_for_event(video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0, ILCLIENT_BUFFER_FLAG_EOS, -1);
 
        // need to flush the renderer to allow video_decode to disable its input port
        // ilclient_flush_tunnels(tunnel, 0);
    }
 
    // ilclient_disable_tunnel(tunnel);
    // ilclient_disable_tunnel(tunnel+1);
    // ilclient_disable_tunnel(tunnel+2);
    ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
    ilclient_disable_port_buffers(video_decode, 131, NULL, NULL, NULL);
    // ilclient_teardown_tunnels(tunnel);
 
 
    ilclient_state_transition(list, OMX_StateIdle);
    ilclient_state_transition(list, OMX_StateLoaded);
 
    ilclient_cleanup_components(list);
 
    OMX_Deinit();
 
    ilclient_destroy(client);
}

void OMXH264Player::startPlay()
{
    if (!m_run && t1 == nullptr)
    {
        m_run = true;
        t1 = new std::thread(run, this);
    }
}

void OMXH264Player::endPlay()
{
    if (m_run && t1 != nullptr)
    {
        m_run = false;
        t1->join();
        delete t1;
        t1 = nullptr;
        ilclient_wait_for_event(video_render,  OMX_EventBufferFlag,  90, 0, OMX_BUFFERFLAG_EOS, 0, ILCLIENT_BUFFER_FLAG_EOS, 10000);
    }
}

void OMXH264Player::pushData(unsigned char *image, int size, int type)
{
    if (size <= 0 || image == nullptr || (type != 0 && type != 1))
    {
//        qDebug() << size << type;
        return;
    }
    DataStruct data;
    data.size = size;
    data.type = type;
    data.data = new unsigned char[size];
    memmove(data.data, image, size);
    m_dataLock.lock();
    m_dataQueue.push_back(data);
    m_dataLock.unlock();
}

bool OMXH264Player::popData()
{
//    qDebug() << m_queue.size();
    m_dataLock.lock();
    if (!m_dataQueue.empty())
    {
        auto data = m_dataQueue.first();
        if (data.type == 0) // video
        {
            draw(data.data, data.size);
        }
        else if(data.type == 1) // audio
        {
            playAudio(data.data, data.size);
        }
        m_dataQueue.pop_front();
        delete data.data;
        data.data = nullptr;
        m_dataLock.unlock();
        return true;
    }
    m_dataLock.unlock();
    return false;
}


