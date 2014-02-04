#ifndef _DECODER_H
#define _DECODER_H

#include <libavformat/avformat.h>


typedef struct _DecoderState {
	AVPacket pkt;
	AVFormatContext *srcFormatCtx;
	AVCodecContext *decodeCtx;
	uint8_t resampledBuff[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	int resampledBuffLen;
	int resampledBuffPos;
	unsigned int streamIndex;
    	int frameSize;
    	uint8_t *frameData;
	ReSampleContext *resampleCtx;
} DecoderState;

void decoderFree(DecoderState *state);
DecoderState *decoderInit(const char* file);
int deocoderGet(DecoderState *state, unsigned char *output, int len);

#endif
