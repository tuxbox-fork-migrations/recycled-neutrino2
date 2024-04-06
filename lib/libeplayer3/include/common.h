#ifndef COMMON_H_
#define COMMON_H_

#include "container.h"
#include "output.h"
#include "manager.h"
#include "playback.h"
#include <pthread.h>


typedef struct Context_s 
{
	PlaybackHandler_t* playback;
	ContainerHandler_t* container;
	OutputHandler_t* output;
	ManagerHandler_t* manager;
} Context_t;

typedef struct Data_s
{
	uint8_t* buffer[AV_NUM_DATA_POINTERS];
	unsigned int size;
	uint32_t width;
	uint32_t height;
	AVRational a;
	uint64_t vpts;
	uint64_t apts;
	uint32_t rate;
}Data_t;

// video stream type 
typedef enum {
	VIDEO_STREAMTYPE_MPEG2 		= 0,
	VIDEO_STREAMTYPE_MPEG4_H264 	= 1,
	VIDEO_STREAMTYPE_MPEG4_H263 	= 2,
	VIDEO_STREAMTYPE_VC1 		= 3,
	VIDEO_STREAMTYPE_MPEG4_Part2 	= 4,
	VIDEO_STREAMTYPE_VC1_SM 	= 5,
	VIDEO_STREAMTYPE_MPEG1 		= 6,
	VIDEO_STREAMTYPE_H265_HEVC 	= 7,
	VIDEO_STREAMTYPE_VB8 		= 8,
	VIDEO_STREAMTYPE_VB9 		= 9,
	VIDEO_STREAMTYPE_XVID 		= 10,
	VIDEO_STREAMTYPE_DIVX311 	= 13,
	VIDEO_STREAMTYPE_DIVX4 		= 14,
	VIDEO_STREAMTYPE_DIVX5 		= 15,
	VIDEO_STREAMTYPE_AVS 		= 16,
	VIDEO_STREAMTYPE_VB6 		= 18,
	VIDEO_STREAMTYPE_SPARK 		= 21,
	VIDEO_STREAMTYPE_MJPEG 		= 30,
	VIDEO_STREAMTYPE_RV30 		= 31, 
	VIDEO_STREAMTYPE_RV40 		= 32,
	VIDEO_STREAMTYPE_AVS2 		= 40
}VIDEO_FORMAT;

// audio stream type
typedef enum {
	AUDIO_STREAMTYPE_AC3 		= 0,
	AUDIO_STREAMTYPE_MPEG 		= 1,
	AUDIO_STREAMTYPE_DTS 		= 2,
	AUDIO_STREAMTYPE_LPCMDVD 	= 6,
	AUDIO_STREAMTYPE_AAC 		= 8,
	AUDIO_STREAMTYPE_AAC_HE 	= 9,
	AUDIO_STREAMTYPE_MP3 		= 0xa,
	AUDIO_STREAMTYPE_AAC_PLUS 	= 0xb,
	AUDIO_STREAMTYPE_DTSHD 		= 0x10,
	AUDIO_STREAMTYPE_WMA 		= 0x20,
	AUDIO_STREAMTYPE_WMA_PRO 	= 0x21,
#if defined (PLATFORM_DREAMBOX)
	AUDIO_STREAMTYPE_EAC3 		= 7,
#else
	AUDIO_STREAMTYPE_EAC3 		= 0x22,
#endif
	AUDIO_STREAMTYPE_AMR 		= 0x23,
	AUDIO_STREAMTYPE_OPUS 		= 0x24,
	AUDIO_STREAMTYPE_VORBIS 	= 0x25,
	AUDIO_STREAMTYPE_RAW 		= 0x30
}AUDIO_FORMAT;

#endif

