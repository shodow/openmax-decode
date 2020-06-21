#include "camera.h"
#include <QDebug>

#define OMX_INIT_STRUCTURE(x) \
  memset (&(x), 0, sizeof (x)); \
  (x).nSize = sizeof (x); \
  (x).nVersion.nVersion = OMX_VERSION; \
  (x).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (x).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (x).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (x).nVersion.s.nStep = OMX_VERSION_STEP

#define VIDEO_FRAMERATE 30
#define VIDEO_BITRATE 17000000
#define VIDEO_IDR_PERIOD 0 //Disabled
#define VIDEO_SEI OMX_FALSE
#define VIDEO_EEDE OMX_FALSE
#define VIDEO_EEDE_LOSS_RATE 0
#define VIDEO_QP OMX_FALSE
#define VIDEO_QP_I 0 //1 .. 51, 0 means off
#define VIDEO_QP_P 0 //1 .. 51, 0 means off
#define VIDEO_PROFILE OMX_VIDEO_AVCProfileHigh
#define VIDEO_INLINE_HEADERS OMX_FALSE

//Some settings doesn't work well
#define CAM_WIDTH 1920
#define CAM_HEIGHT 1080
#define CAM_SHARPNESS 0 //-100 .. 100
#define CAM_CONTRAST 0 //-100 .. 100
#define CAM_BRIGHTNESS 50 //0 .. 100
#define CAM_SATURATION 0 //-100 .. 100
#define CAM_SHUTTER_SPEED_AUTO OMX_TRUE
#define CAM_SHUTTER_SPEED 1.0/8.0
#define CAM_ISO_AUTO OMX_TRUE
#define CAM_ISO 100 //100 .. 800
#define CAM_EXPOSURE OMX_ExposureControlAuto
#define CAM_EXPOSURE_COMPENSATION 0 //-24 .. 24
#define CAM_MIRROR OMX_MirrorNone
#define CAM_ROTATION 0 //0 90 180 270
#define CAM_COLOR_ENABLE OMX_FALSE
#define CAM_COLOR_U 128 //0 .. 255
#define CAM_COLOR_V 128 //0 .. 255
#define CAM_NOISE_REDUCTION OMX_TRUE
#define CAM_FRAME_STABILIZATION OMX_FALSE
#define CAM_METERING OMX_MeteringModeAverage
#define CAM_WHITE_BALANCE OMX_WhiteBalControlAuto
//The gains are used if the white balance is set to off
#define CAM_WHITE_BALANCE_RED_GAIN 1000 //0 ..
#define CAM_WHITE_BALANCE_BLUE_GAIN 1000 //0 ..
#define CAM_IMAGE_FILTER OMX_ImageFilterNone
#define CAM_ROI_TOP 0 //0 .. 100
#define CAM_ROI_LEFT 0 //0 .. 100
#define CAM_ROI_WIDTH 100 //0 .. 100
#define CAM_ROI_HEIGHT 100 //0 .. 100
#define CAM_DRC OMX_DynRangeExpOff

CCamera::CCamera()
{
   memset(list, 0, sizeof(list));
   memset(tunnel, 0, sizeof(tunnel));

   if((client = ilclient_init()) == NULL)
   {
      qDebug() << "ilclient_init error";
   }

   if(OMX_Init() != OMX_ErrorNone)
   {
      qDebug() << "OMX_Init error";
      ilclient_destroy(client);
   }

   // create camera
   if(ilclient_create_component(client, &camera, "camera", ILCLIENT_DISABLE_ALL_PORTS) != 0)
   {
      qDebug() << "camera error";
      status = -14;
   }
   list[0] = camera;


   // create video_decode
   if(ilclient_create_component(client, &video_decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS) != 0)
   {
      qDebug() << "video_decode error";
      status = -14;
   }
   list[1] = video_decode;

   // create video_render
   if(status == 0 && ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
   {
      qDebug() << "video_render error";
      status = -14;
   }
   list[2] = video_render;

   // create clock
   if(status == 0 && ilclient_create_component(client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0)
   {
      qDebug() << "clock error";
      status = -14;
   }
   list[3] = clock;

   memset(&cstate, 0, sizeof(cstate));
   cstate.nSize = sizeof(cstate);
   cstate.nVersion.nVersion = OMX_VERSION;
   cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
   cstate.nWaitMask = 1;
   if(clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
   {
      qDebug() << "clock OMX_SetParameter error";
      status = -13;
   }

   // create video_scheduler
   if(status == 0 && ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0)
   {
      qDebug() << "video_scheduler error";
      status = -14;
   }
   list[4] = video_scheduler;

   set_tunnel(tunnel, camera, 71, video_decode, 130);
   set_tunnel(tunnel+1, video_decode, 131, video_scheduler, 10);
   set_tunnel(tunnel+2, video_scheduler, 11, video_render, 90);
   set_tunnel(tunnel+3, clock, 80, video_scheduler, 12);

   // setup clock tunnel first
   if(status == 0 && ilclient_setup_tunnel(tunnel+3, 0, 0) != 0)
   {
      qDebug() << "ilclient_setup_tunnel error";
      status = -15;
   }
   else
      ilclient_change_component_state(clock, OMX_StateExecuting);


   ilclient_change_component_state(camera, OMX_StateIdle);

   // ilclient_change_component_state(video_decode, OMX_StateExecuting);
   OMX_ERRORTYPE error;

   OMX_CONFIG_REQUESTCALLBACKTYPE cbs_st;
   OMX_INIT_STRUCTURE (cbs_st);
   cbs_st.nPortIndex = OMX_ALL;
   cbs_st.nIndex = OMX_IndexParamCameraDeviceNumber;
   cbs_st.bEnable = OMX_TRUE;
   if ((error = OMX_SetConfig (ILC_GET_HANDLE(camera), OMX_IndexConfigRequestCallback,
                               &cbs_st))){
//      fprintf (stderr, "error: OMX_SetConfig: %s\n", dump_OMX_ERRORTYPE (error));
      exit (1);
   }

   OMX_PARAM_U32TYPE dev_st;
   OMX_INIT_STRUCTURE (dev_st);
   dev_st.nPortIndex = OMX_ALL;
   //ID for the camera device
   dev_st.nU32 = 0;
   if ((error = OMX_SetParameter (ILC_GET_HANDLE(camera), OMX_IndexParamCameraDeviceNumber, &dev_st)))
   {
//      fprintf (stderr, "error: OMX_SetParameter: %s\n",
//      dump_OMX_ERRORTYPE (error));
      exit (1);
   }

   //Configure camera port definition
   OMX_PARAM_PORTDEFINITIONTYPE port_st;
   OMX_INIT_STRUCTURE (port_st);
   port_st.nPortIndex = 71;
   if ((error = OMX_GetParameter (ILC_GET_HANDLE(camera), OMX_IndexParamPortDefinition, &port_st))){
      exit (1);
   }

   port_st.format.video.nFrameWidth = CAM_WIDTH;
   port_st.format.video.nFrameHeight = CAM_HEIGHT;
   port_st.format.video.nStride = CAM_WIDTH;
   port_st.format.video.xFramerate = VIDEO_FRAMERATE << 16;
   port_st.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
   port_st.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
   if ((error = OMX_SetParameter (ILC_GET_HANDLE(camera), OMX_IndexParamPortDefinition, &port_st))){
      exit (1);
   }

   //Preview port
   port_st.nPortIndex = 70;
   if ((error = OMX_SetParameter (ILC_GET_HANDLE(camera), OMX_IndexParamPortDefinition, &port_st))){

      exit (1);
   }

//   printf ("configuring %s framerate\n", camera.name);
   OMX_CONFIG_FRAMERATETYPE framerate_st;
   OMX_INIT_STRUCTURE (framerate_st);
   framerate_st.nPortIndex = 71;
   framerate_st.xEncodeFramerate = port_st.format.video.xFramerate;
   if ((error = OMX_SetConfig (ILC_GET_HANDLE(camera), OMX_IndexConfigVideoFramerate, &framerate_st))){
//      fprintf (stderr, "error: OMX_SetConfig: %s\n", dump_OMX_ERRORTYPE (error));
      exit (1);
   }

   //Preview port
   framerate_st.nPortIndex = 70;
   if ((error = OMX_SetConfig (ILC_GET_HANDLE(camera), OMX_IndexConfigVideoFramerate, &framerate_st))){
//      fprintf (stderr, "error: OMX_SetConfig: %s\n", dump_OMX_ERRORTYPE (error));
      exit (1);
   }

}

CCamera::~CCamera()
{

   ilclient_disable_tunnel(tunnel);
   ilclient_disable_tunnel(tunnel+1);
   ilclient_disable_tunnel(tunnel+2);
   ilclient_disable_tunnel(tunnel+3);
   // ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
   ilclient_teardown_tunnels(tunnel);

   ilclient_state_transition(list, OMX_StateIdle);
   ilclient_state_transition(list, OMX_StateLoaded);

   ilclient_cleanup_components(list);

   OMX_Deinit();

   ilclient_destroy(client);
}

void CCamera::play(char *filename)
{

   OMX_BUFFERHEADERTYPE *buf;
   int port_settings_changed = 0;
   int first_packet = 1;

   while((buf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL)
   {
      // feed data and wait until we get port settings changed
      unsigned char *dest = buf->pBuffer;

//      data_len += fread(dest, 1, buf->nAllocLen-data_len, in);

      if(port_settings_changed == 0 &&
         ((data_len > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
          (data_len == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                    ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
      {
         port_settings_changed = 1;

         if(ilclient_setup_tunnel(tunnel, 0, 0) != 0)
         {
            status = -7;
            break;
         }

         if(ilclient_setup_tunnel(tunnel+1, 0, 0) != 0)
         {
            status = -7;
            break;
         }

         ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

         // now setup tunnel to video_render
         if(ilclient_setup_tunnel(tunnel+2, 0, 1000) != 0)
         {
            status = -12;
            break;
         }

         ilclient_change_component_state(video_render, OMX_StateExecuting);
      }
      if(!data_len)
         break;

      buf->nFilledLen = data_len;
      data_len = 0;

      buf->nOffset = 0;
      if(first_packet)
      {
         buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
         first_packet = 0;
      }
      else
         buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

      if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
      {
         status = -6;
         break;
      }
   }

   buf->nFilledLen = 0;
   buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

   if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
      status = -20;

   // wait for EOS from render
   ilclient_wait_for_event(video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0,
                           ILCLIENT_BUFFER_FLAG_EOS, -1);

   // need to flush the renderer to allow video_decode to disable its input port
   ilclient_flush_tunnels(tunnel, 0);
}
