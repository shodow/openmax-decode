#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "OMXAudio.h"
#include <QDebug>

    /* For the RPi name can be "hdmi" or "local" */
    void OMXAudio::setOutputDevice(OMX_HANDLETYPE handle, const char *name) {
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

void OMXAudio::setPCMMode(OMX_HANDLETYPE handle, int startPortNumber) {
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

void OMXAudio::set_audio_render_input_format(COMPONENT_T *audioRender) {
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

OMX_ERRORTYPE OMXAudio::read_into_buffer_and_empty(uchar *pcmData, int len, COMPONENT_T *audioRender,
                                         OMX_BUFFERHEADERTYPE *buff_header) {
	OMX_ERRORTYPE r;
	int buff_size = buff_header->nAllocLen;
	int nread = len;
//    qDebug() << buff_header->nAllocLen << " " << len;
	memcpy(buff_header->pBuffer, pcmData, len);
//	printf("Read %d\n", nread);
	buff_header->nFilledLen = nread;
	// *toread -= nread;
	// if (*toread <= 0) {
	// 	printf("Setting EOS on input\n");
	// 	buff_header->nFlags |= OMX_BUFFERFLAG_EOS;
	// }
	r = OMX_EmptyThisBuffer(ilclient_get_handle(audioRender), buff_header);
	if (r != OMX_ErrorNone) {
		// fprintf(stderr, "Empty buffer error %s\n", err2str(r));
	}
	return r;
}

int OMXAudio::init() {
	int i;
	char *componentName;
	int err;
	ILCLIENT_T *handle;

	componentName = "audio_render";

	bcm_host_init();
	handle = ilclient_init();
	if (handle == NULL) {
		fprintf(stderr, "IL client init failed\n");
		exit(1);
	}
	if (OMX_Init() != OMX_ErrorNone) {
		ilclient_destroy(handle);
		fprintf(stderr, "OMX init failed\n");
		exit(1);
	}

    err = ilclient_create_component(handle, &audioRender, componentName, (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS));
	if (err == -1) {
		fprintf(stderr, "Component create failed\n");
		exit(1);
	}

	err = ilclient_change_component_state(audioRender, OMX_StateIdle);
	if (err < 0) {
		fprintf(stderr, "Couldn't change state to Idle\n");
		exit(1);
	}

	// must be before we enable buffers
	set_audio_render_input_format(audioRender);
	setOutputDevice(ilclient_get_handle(audioRender), "local");
	// input port
	ilclient_enable_port_buffers(audioRender, 100, NULL, NULL, NULL);
	ilclient_enable_port(audioRender, 100);
	err = ilclient_change_component_state(audioRender, OMX_StateExecuting);
	if (err < 0) {
		fprintf(stderr, "Couldn't change state to Executing\n");
		exit(1);
	}
}

void OMXAudio::playAudio(uchar *pcmData, int len)
{
	// now work through the file
	OMX_ERRORTYPE r;
    OMX_BUFFERHEADERTYPE *buff_header;
	// do we have an input buffer we can fill and empty?
	buff_header = ilclient_get_input_buffer(audioRender, 100, 1 /* block */);
	if (buff_header != NULL) {
		read_into_buffer_and_empty(pcmData, len, audioRender, buff_header);
	}
}

OMXAudio::OMXAudio(QObject *parent)
{
    init();
}

OMXAudio::~OMXAudio()
{

}
