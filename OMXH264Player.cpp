#include "OMXH264Player.h"
#include <QDebug>
#include <QTime>
 
OMXH264Player::OMXH264Player(QObject *parent):
    QObject(parent),
    video_decode(NULL),
    port_settings_changed(0),
    first_packet(1),
    omxbuf(NULL)
{

    fd = fopen("yuv", "wb");
    int err=0;
    qDebug()<<"OMXH264Player constructor start";
    bcm_host_init();
    int status = 0;
    memset(list, 0, sizeof(list));
    // memset(tunnel, 0, sizeof(tunnel));
 
    if((client = ilclient_init()) == NULL){
        qDebug()<<"OMXH264Player ilclient_init error";
        return;
    }
    OMX_ERRORTYPE error;
    if((error = OMX_Init()) != OMX_ErrorNone){
        ilclient_destroy(client);
        qDebug("OMXH264Player OMX_Init error %x",error);
        return;
    }
 
    // create video_decode
    if(ilclient_create_component(client, &video_decode, "video_decode", (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS | ILCLIENT_ENABLE_OUTPUT_BUFFERS)) != 0)
      status = -14;
    list[0] = video_decode;
 
    // create video_render
    // if(status == 0 && ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
    //   status = -14;
    // list[1] = video_render;
 
    // create clock
    // if(status == 0 && ilclient_create_component(client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0)
    //   status = -14;
    // list[2] = clock;
 
    // memset(&cstate, 0, sizeof(cstate));
    // cstate.nSize = sizeof(cstate);
    // cstate.nVersion.nVersion = OMX_VERSION;
    // cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
    // cstate.nWaitMask = 1;
    // if(clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
    //   status = -13;
 
    // create video_scheduler
    // if(status == 0 && ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0)
    //   status = -14;
    // list[3] = video_scheduler;
 
    // set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
    // set_tunnel(tunnel+1, video_scheduler, 11, video_render, 90);
    // set_tunnel(tunnel+2, clock, 80, video_scheduler, 12);
 
    // setup clock tunnel first
    // if(status == 0 && ilclient_setup_tunnel(tunnel+2, 0, 0) != 0)
    //   status = -15;
    // else
    //   ilclient_change_component_state(clock, OMX_StateExecuting);
 

    err = ilclient_change_component_state(video_decode, OMX_StateIdle); // initialization has been completed successfully and the component is ready to to start
    if (err < 0) {
        qDebug()<<"OMXH264Player OMX_StateIdle error, err = "<<err;
    }

    // set input buffer port param
    memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE); // 结构体大小
    format.nVersion.nVersion = OMX_VERSION;                // omx版本
    format.nPortIndex = 130;                               // 端口
    format.eCompressionFormat = OMX_VIDEO_CodingAVC;       // H.264/AVC
    if (status == 0 && OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone)
    {
        qDebug() << "OMX_SetParameter success";
    }else 
    {
        qDebug() << "OMX_SetParameter error, status = "<< status;
    }
 
    if(status == 0 &&
      ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0)
    {
        qDebug()<<"OMXH264Player input constructor success";
    }else{
        qDebug()<<"OMXH264Player input constructor error, status = "<<status;
    }
    ilclient_enable_port(video_decode, 130);

    err = ilclient_change_component_state(video_decode, OMX_StateExecuting); // accepted the start command and is processing data (if data is available)
    if (err < 0) {
        qDebug()<<"OMXH264Player OMX_StateExecuting error, err = "<<err;
    }

    // buff_header = ilclient_get_input_buffer(video_decode, 130, 1 /* block */);
    // if (buff_header != NULL) {

    //     r = OMX_EmptyThisBuffer(ilclient_get_handle(video_decode), buff_header);
    //     if (r != OMX_ErrorNone) {
    //         qDebug() << "Empty buffer error " << r;
    //     }

    //     if (toread <= 0) {
    //      printf("Rewinding\n");
    //      // wind back to start and repeat
    //      fp = freopen(IMG, "r", fp);
    //      toread = get_file_size(IMG);
    //      }
    // }



    // set output buffer port param
    // memset(&format2, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    // format2.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE); // 结构体大小
    // format2.nVersion.nVersion = OMX_VERSION;                // omx版本
    // format2.nPortIndex = 131;                               // 端口
    // format2.eColorFormat = OMX_COLOR_Format24bitRGB888;     // 24bitRGB888 Red 24:16, Green 15:8, Blue 7:0
 
    // if(status == 0 &&
    //   OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format2) == OMX_ErrorNone &&
    //   ilclient_enable_port_buffers(video_decode, 131, NULL, NULL, NULL) == 0)
    // {
    //     qDebug()<<"OMXH264Player output constructor success";
    // }else{
    //     qDebug()<<"OMXH264Player output constructor error, status = "<<status;
    // }

    
}
 
OMXH264Player::~OMXH264Player(){
    qDebug()<<"OMXH264Player destructor";
}

void OMXH264Player::draw( unsigned char * image, int size){
    QTime time;
    time.start();
//    qDebug()<<"write "<<size;
    if(size > 81920){
        qDebug()<<"write size larger than omxbuf size, largest 81920";
        return;
    }
    unsigned int data_len = 0;
    omxbuf = ilclient_get_input_buffer(video_decode, 130, 1 /* block */);
    if(omxbuf == NULL){
        return;
    }
    unsigned char *dest = omxbuf->pBuffer;
    memcpy(dest, image, size);
    data_len = size;
 
    // if(port_settings_changed == 0 && (data_len > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0)){
    //     port_settings_changed = 1;
    //     if(ilclient_setup_tunnel(tunnel, 0, 0) != 0) return;
    //     ilclient_change_component_state(video_scheduler, OMX_StateExecuting);
    //     // now setup tunnel to video_render
    //     if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0) return ;
    //     ilclient_change_component_state(video_render, OMX_StateExecuting);
    // }
    if(!data_len) return ;
    omxbuf->nFilledLen = data_len;
    omxbuf->nOffset = 0;
    if(first_packet) // 发送的第一个包文件
    {
        // omxbuf->nFlags = OMX_BUFFERFLAG_STARTTIME;
        OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), omxbuf);

        // wait for first input block to set params for output port
        int err = ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                          ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 2000);
        if (err < 0) {
            qDebug("Wait for port settings changed timed out\n");
        } else {
            first_packet = 0;
        }
        err = ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1);
        if (err < 0) {
            qDebug("Wait for remove port settings changed timed out\n");
        } else {
            first_packet = 0;
        }
        if (!first_packet)
        {
            OMX_PARAM_PORTDEFINITIONTYPE portdef;
            qDebug() << "Port settings changed\n";
            // need to setup the input for the resizer with the output of the
            // decoder
            portdef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
            portdef.nVersion.nVersion = OMX_VERSION;
            portdef.nPortIndex = 131;
            OMX_GetParameter(ilclient_get_handle(video_decode), OMX_IndexParamPortDefinition, &portdef);

            portdef.format.video.eColorFormat = OMX_COLOR_Format24bitRGB888;
            OMX_SetParameter(ilclient_get_handle(video_decode), OMX_IndexParamPortDefinition, &portdef);
            unsigned int uWidth = (unsigned int) portdef.format.video.nFrameWidth;
            unsigned int uHeight = (unsigned int) portdef.format.video.nFrameHeight;
            unsigned int uStride = (unsigned int) portdef.format.video.nStride;
            unsigned int uSliceHeight = (unsigned int) portdef.format.video.nSliceHeight;
            qDebug() << QString("Frame width %1, frame height %2, stride %3, slice height %4").arg(uWidth)
                    .arg(uHeight).arg(uStride).arg(uSliceHeight);
            qDebug() << QString("Getting format Compression %1 Color Format: %2")
                        .arg((unsigned int) portdef.format.video.eCompressionFormat)
                        .arg((unsigned int) portdef.format.video.eColorFormat);
            // now enable output port since port params have been set
            ilclient_enable_port_buffers(video_decode, 131, NULL, NULL, NULL);
            ilclient_enable_port(video_decode, 131);
            width = uStride;
            height = uSliceHeight;

            int yuvSize = width * height * 3 /2;
            yuvBuffer = (uint8_t *)malloc(yuvSize);
            //为每帧图像分配内存
            pFrame = av_frame_alloc();
            pFrameRGB = av_frame_alloc();
            int numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, width,height);
            rgbBuffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
            avpicture_fill((AVPicture *) pFrameRGB, rgbBuffer, AV_PIX_FMT_RGB32,width, height);
            //特别注意 img_convert_ctx 该在做H264流媒体解码时候，发现sws_getContext /sws_scale内存泄露问题，
            //注意sws_getContext只能调用一次，在初始化时候调用即可，另外调用完后，在析构函数中使用sws_free_Context，将它的内存释放。
            //设置图像转换上下文
            img_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
        }
    }else {
        // omxbuf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
        OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), omxbuf);

        omxbuf = ilclient_get_output_buffer(video_decode, 131, 0 /* no block */);
        if (omxbuf != NULL) {
            if (OMX_FillThisBuffer(ilclient_get_handle(video_decode), omxbuf) != OMX_ErrorNone) {
                qDebug() << "Fill buffer error";
            }
            else
            {
                qDebug() << omxbuf->nFilledLen;
//                qDebug() << omxbuf->nFlags;
                qDebug() << omxbuf->nOffset;
//                qDebug() << omxbuf->pBuffer;

                //            fwrite(omxbuf->pBuffer + omxbuf->nOffset, omxbuf->nFilledLen, 1, fd);
                unsigned char *yuvdata = omxbuf->pBuffer + omxbuf->nOffset;

                avpicture_fill((AVPicture *) pFrame, (uint8_t *)yuvdata, AV_PIX_FMT_YUV420P, width, height);//这里的长度和高度跟之前保持一致
                //转换图像格式，将解压出来的YUV420P的图像转换为RGB的图像
                sws_scale(img_convert_ctx,
                         (uint8_t const * const *) pFrame->data,
                         pFrame->linesize, 0, height, pFrameRGB->data,
                         pFrameRGB->linesize);
                //把这个RGB数据 用QImage加载
                QImage tmpImg((uchar *)rgbBuffer, width, height, QImage::Format_RGB32);
                qDebug() << "use time:" << time.elapsed() << "ms";
                emit sendImg(tmpImg);
            }
        } else {
            qDebug() << "No filled buffer";
        }
    }
 
    
    //ilclient_wait_for_event(ILC_GET_HANDLE(video_decode), OMX_EventMark, 131, 0, OMX_BUFFERFLAG_TIME_UNKNOWN|OMX_BUFFERFLAG_STARTTIME, 0, ILCLIENT_EMPTY_BUFFER_DONE, -1);
    // qDebug() << "---------------------1";
    // omxOutBuf = ilclient_get_output_buffer(video_decode, 131, 1);
    // qDebug() << "---------------------2";
    // OMX_FillThisBuffer(ILC_GET_HANDLE(video_decode), omxOutBuf);
    // ilclient_wait_for_event(video_decode, OMX_EventMark, 131, 0, 0, 1, ILCLIENT_FILL_BUFFER_DONE, -1);
    
}
 
void OMXH264Player::playbackTest(){
    unsigned char* buffer = (unsigned char*)malloc(81920);
    memset(buffer, 0, 81920);
    FILE *in;
    if((in = fopen("cuc_ieschool.h264", "rb")) == NULL)
        return;
    int read = 0;
    while((read = fread(buffer, 1, 2000, in))>0){
        draw(buffer, read);
    }
    clear();
}
 
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
