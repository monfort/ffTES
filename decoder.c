#include "decoder.h"

#define MIN(a,b) ((a)<(b)?(a):(b))

DecoderState *decoderInit(const char* file)
{
	int err, framesInBestSteam = -1;
	unsigned int i;
	DecoderState *state = NULL;
	AVCodec *decoder;

	// Allocate decoder state
	if (!(state = (DecoderState*)malloc(sizeof(DecoderState))))
	{
		printf("Error allocating deocder state\n");
		goto error;
	}
	memset(state, 0, sizeof(DecoderState));

	// Init ffmpeg
	av_register_all();

	// Open input
	err = av_open_input_file(&state->srcFormatCtx, file, NULL, 0, NULL);
	if (err)
	{
		printf("Error openning input file: %s\n", file);
		goto error;
	}

    // Get file info so we can find audio stream properties
    if ((err = av_find_stream_info(state->srcFormatCtx)) < 0)
    {
		printf("Error finding streams in: %s\n", file);
        goto error;
    }

    // Find audio stream to decode (we'll use the first stream we find)
    for (i = 0; i < state->srcFormatCtx->nb_streams; i++)
        if (state->srcFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && 
			state->srcFormatCtx->streams[i]->codec_info_nb_frames > framesInBestSteam)
        {
			framesInBestSteam = state->srcFormatCtx->streams[i]->codec_info_nb_frames;
            state->decodeCtx = state->srcFormatCtx->streams[i]->codec;
			state->streamIndex = i;
        }

    if (!state->decodeCtx)
    {
		printf("Error finding audio stream in: %s\n", file);
		goto error;
    }

    // Find decoder for this file and open it
    if (!(decoder = avcodec_find_decoder(state->decodeCtx->codec_id)) || avcodec_open(state->decodeCtx, decoder) < 0)
    {
		printf("Error finding decoder for: %s\n", file);
        goto error;
    }

    // If for some reason the source stream's sample rate or channel isn't valid then fail decoding
    if (!state->decodeCtx->sample_rate || !state->decodeCtx->channels || state->decodeCtx->channels > 2)
    {
		printf("Invalid information in input stream for file: %s\n", file);
        goto error;
    }

	// Setup resampler so cause input is probably not in the format we want
	state->resampleCtx = av_audio_resample_init(1, state->decodeCtx->channels, 22050, state->decodeCtx->sample_rate, AV_SAMPLE_FMT_U8, state->decodeCtx->sample_fmt, 16, 10, 0, 0.8); 

	return state;

error:
	if (state)
		decoderFree(state);

	return NULL;
}

void decoderFree(DecoderState *state)
{
    if (state->pkt.data)
        av_free_packet(&state->pkt);

    if (state->decodeCtx)
        avcodec_close(state->decodeCtx);

    if (state->srcFormatCtx)
        av_close_input_file(state->srcFormatCtx);

	if (state->resampleCtx)
		audio_resample_close(state->resampleCtx);

	free(state);
}

int deocoderGet(DecoderState *state, unsigned char *output, int len)
{
	int bytesWritten = 0;
	int decodedSampleSize = av_get_bits_per_sample_fmt(state->decodeCtx->sample_fmt)/8;

	// Copy any remaining resampled (decoded) data to output
	if (state->resampledBuffLen)
	{
		bytesWritten = MIN(len, state->resampledBuffLen);
		memcpy(output, state->resampledBuff + state->resampledBuffPos, bytesWritten);
		state->resampledBuffLen -= bytesWritten;
		state->resampledBuffPos += bytesWritten;
		len -= bytesWritten;
		output += bytesWritten;
	}

	// Decode more data as long as there's room in the output
	while (len)
	{
		// If we're in the middle of a frame continue decoding it
        while (state->frameSize && len)
        {
			int usedBytes;
			int dataToCopy;
			DECLARE_ALIGNED(16,uint8_t,decodeBuff)[AVCODEC_MAX_AUDIO_FRAME_SIZE];
			int decodeBuffLen = sizeof(decodeBuff);

            // Decode current position in source frame
			state->resampledBuffLen = 0;
			state->resampledBuffPos = 0;
			usedBytes = avcodec_decode_audio3(state->decodeCtx, (int16_t*)decodeBuff, &decodeBuffLen,(AVPacket*) state);
            // Finished decoding source frame
            if (usedBytes <= 0)
                break;
            // Move forward in source frame
            state->frameSize -= usedBytes;
            state->frameData += usedBytes;

			// If nothing was decoded (yes this can happen!) then continue
			if (decodeBuffLen == 0)
				continue;

			// Resample the decoded data (to 22HHz 8bit mono)
			state->resampledBuffLen = audio_resample(state->resampleCtx, (int16_t*)state->resampledBuff, (int16_t*)decodeBuff, decodeBuffLen / (state->decodeCtx->channels * decodedSampleSize));

			// Copy decoded data to the output
			dataToCopy = MIN(state->resampledBuffLen, len);
			memcpy(output, state->resampledBuff, dataToCopy);
			state->resampledBuffLen -= dataToCopy;
			state->resampledBuffPos += dataToCopy;
			len -= dataToCopy;
			output += dataToCopy;
			bytesWritten += dataToCopy;
		}
		
		// Reset decoding state before reading next frame
        if (state->pkt.data)
            av_free_packet(&state->pkt);
		state->frameData = NULL;
		state->frameSize = 0;

		// Read next frame from source file
		if (av_read_frame(state->srcFormatCtx, &state->pkt) < 0)
			break; // We reached the end of the data (or encountered other error)

        // If the frame doesn't belong to the audio stream then skip it
        if (state->pkt.stream_index != state->streamIndex)
            continue;

        // Init frame decoding state so we'll start decoding this frame in the next loop iteration
        state->frameSize = state->pkt.size;
        state->frameData = state->pkt.data;
 	}

	return bytesWritten;
}
