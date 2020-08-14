#include "opus.h"
#include "opus_types.h"
#include "opus_private.h"
#include "opus_memory.h"
#include "voice_opus.h"
#include "hal_trace.h"

static uint8_t is_voice_opus_initialized = 0;
static uint8_t is_voice_opus_encoder_initialized = 0;
static uint8_t is_voice_opus_decoder_initialized = 0;

static OpusEncoder* voice_opus_enc = NULL;
static OpusDecoder* voice_opus_dec = NULL;

static VOICE_OPUS_CONFIG_T	voice_opus_config =
			{
				VOICE_OPUS_HEAP_SIZE,
				VOICE_OPUS_CHANNEL_COUNT,
				VOICE_OPUS_COMPLEXITY,
				VOICE_OPUS_PACKET_LOSS_PERC,
				VOICE_SIZE_PER_SAMPLE,
				VOICE_OPUS_APP,
				VOICE_OPUS_BANDWIDTH,
				VOICE_OPUS_BITRATE,
				VOICE_OPUS_SAMPLE_RATE,
				VOICE_SIGNAL_TYPE,
				VOICE_FRAME_PERIOD,
				
				VOICE_OPUS_USE_VBR,
				VOICE_OPUS_CONSTRAINT_USE_VBR,
				VOICE_OPUS_USE_INBANDFEC,
				VOICE_OPUS_USE_DTX,
			};

static uint8_t* voice_opus_heap;
static int voice_opus_init_encoder(uint16_t sampleRate);
static int voice_opus_deinit_encoder(void);
static int voice_opus_init_decoder(uint16_t sampleRate);
static int voice_opus_deinit_decoder(void);

static int voice_opus_init_encoder(uint16_t sampleRate)
{
	if (is_voice_opus_encoder_initialized)
	{
		return -1;
	}
		
    int err;
    voice_opus_enc = opus_encoder_create(sampleRate, voice_opus_config.channelCnt, voice_opus_config.appType, &err);
    if (err != OPUS_OK)
    {
        return -2;
    }
    opus_encoder_ctl(voice_opus_enc, OPUS_SET_BITRATE(voice_opus_config.bitRate));
    opus_encoder_ctl(voice_opus_enc, OPUS_SET_BANDWIDTH(voice_opus_config.bandWidth));
    opus_encoder_ctl(voice_opus_enc, OPUS_SET_VBR(voice_opus_config.isUseVbr));
    opus_encoder_ctl(voice_opus_enc, OPUS_SET_VBR_CONSTRAINT(voice_opus_config.isConstraintUseVbr));
    opus_encoder_ctl(voice_opus_enc, OPUS_SET_COMPLEXITY(voice_opus_config.complexity));
    opus_encoder_ctl(voice_opus_enc, OPUS_SET_INBAND_FEC(voice_opus_config.isUseInBandFec));
    opus_encoder_ctl(voice_opus_enc, OPUS_SET_FORCE_CHANNELS(voice_opus_config.channelCnt));
    opus_encoder_ctl(voice_opus_enc, OPUS_SET_DTX(voice_opus_config.isUseDtx));
    opus_encoder_ctl(voice_opus_enc, OPUS_SET_PACKET_LOSS_PERC(voice_opus_config.packetLossPercentage));
    opus_encoder_ctl(voice_opus_enc, OPUS_SET_SIGNAL(voice_opus_config.signalType));
	opus_encoder_ctl(voice_opus_enc, OPUS_SET_EXPERT_FRAME_DURATION(voice_opus_config.periodPerFrame));

	is_voice_opus_encoder_initialized = 1;

    return 0;
}

uint32_t voice_opus_encode(uint8_t *bitstream, uint8_t *speech, uint32_t sampleCount, uint8_t isReset)
{
	if (isReset)
	{
		voice_opus_deinit_encoder();
		int8_t ret = voice_opus_init_encoder(voice_opus_config.sampleRate);
		if (0 != ret)
		{
			return ret;
		}
	}
	uint32_t outputBytes = opus_encode(voice_opus_enc, (opus_int16*)bitstream, sampleCount, speech, sampleCount*4);
	return outputBytes;
}

static int voice_opus_init_decoder(uint16_t sampleRate)
{
	if (is_voice_opus_decoder_initialized)
	{
		return -1;
	}
	
    int err;
    voice_opus_dec = opus_decoder_create(sampleRate, voice_opus_config.channelCnt, &err);
    if (err != OPUS_OK)
    {
        return -2;
    }
	opus_int32 skip;
		
    opus_decoder_ctl(voice_opus_dec, OPUS_SET_BITRATE(voice_opus_config.bitRate));
    opus_decoder_ctl(voice_opus_dec, OPUS_SET_BANDWIDTH(voice_opus_config.bandWidth));
    opus_decoder_ctl(voice_opus_dec, OPUS_SET_VBR(voice_opus_config.isUseVbr));
    opus_decoder_ctl(voice_opus_dec, OPUS_SET_VBR_CONSTRAINT(voice_opus_config.isConstraintUseVbr));
    opus_decoder_ctl(voice_opus_dec, OPUS_SET_COMPLEXITY(voice_opus_config.complexity));
    opus_decoder_ctl(voice_opus_dec, OPUS_SET_INBAND_FEC(voice_opus_config.isUseInBandFec));
    opus_decoder_ctl(voice_opus_dec, OPUS_SET_FORCE_CHANNELS(voice_opus_config.channelCnt));
    opus_decoder_ctl(voice_opus_dec, OPUS_SET_DTX(voice_opus_config.isUseDtx));
    opus_decoder_ctl(voice_opus_dec, OPUS_SET_PACKET_LOSS_PERC(voice_opus_config.packetLossPercentage));
    opus_decoder_ctl(voice_opus_dec, OPUS_SET_SIGNAL(voice_opus_config.signalType));
	opus_decoder_ctl(voice_opus_dec, OPUS_SET_EXPERT_FRAME_DURATION(voice_opus_config.periodPerFrame));

	opus_decoder_ctl(voice_opus_dec, OPUS_GET_LOOKAHEAD(&skip));
	is_voice_opus_decoder_initialized = 1;

    return 0;
}

uint32_t voice_opus_decode(uint8_t *speech, uint32_t speechLen, uint8_t *bitstream, uint32_t sampleCount, uint8_t isReset)
{
	if (isReset)
	{
		voice_opus_deinit_decoder();
		int8_t ret = voice_opus_init_decoder(voice_opus_config.sampleRate);
		if (0 != ret)
		{
			return ret;
		}
	}
	uint32_t outputPcmCount = opus_decode(voice_opus_dec, speech, speechLen, (opus_int16*)bitstream, sampleCount, 0);
	return outputPcmCount;
}

int voice_opus_init(VOICE_OPUS_CONFIG_T* pConfig, uint8_t* ptr)
{
    if (!is_voice_opus_initialized) {
		voice_opus_config = *pConfig;
		voice_opus_heap = ptr;
		TRACE("voice_opus_heap 0x%x", (uint32_t)voice_opus_heap);
        rt_system_heap_init(ptr,  (void *)((uint32_t)ptr + (voice_opus_config.heapSize-1)));

        is_voice_opus_initialized = 1;
    }

   	return 0;
}

int voice_opus_deinit(void)
{
    if (is_voice_opus_initialized) {

		voice_opus_heap = NULL;
        is_voice_opus_initialized = 0;
    }

   	return 0;
}


static int voice_opus_deinit_encoder(void)
{
    if (is_voice_opus_encoder_initialized) {
		
		//opus_encoder_destroy(voice_opus_enc);
        is_voice_opus_encoder_initialized = 0;
    }
    return 0;
}

static int voice_opus_deinit_decoder(void)
{
    if (is_voice_opus_decoder_initialized) {
		
		//opus_decoder_destroy(voice_opus_dec);
        is_voice_opus_decoder_initialized = 0;
    }
    return 0;
}

