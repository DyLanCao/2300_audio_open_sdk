#include "voice_opus.h"
#include "string.h"
#include "app_wl_smartvoice_encode.h"
#include "app_audio.h"

typedef VOICE_OPUS_CONFIG_T wl_smartvoice_opus_cfg_t;


static wl_smartvoice_opus_cfg_t opus_cfg = {
    WL_SMARTVOICE_OPUS_CONFIG_HEAP_SIZE,
    WL_SMARTVOICE_OPUS_CONFIG_CHANNEL_COUNT,
    WL_SMARTVOICE_OPUS_CONFIG_COMPLEXITY,
    WL_SMARTVOICE_OPUS_CONFIG_PACKET_LOSS_PERC,
    WL_SMARTVOICE_OPUS_CONFIG_SIZE_PER_SAMPLE,
    WL_SMARTVOICE_OPUS_CONFIG_APP,
    WL_SMARTVOICE_OPUS_CONFIG_BANDWIDTH,
    WL_SMARTVOICE_OPUS_CONFIG_BITRATE,
    WL_SMARTVOICE_OPUS_CONFIG_SAMPLE_RATE,
    WL_SMARTVOICE_OPUS_CONFIG_SIGNAL_TYPE,
    WL_SMARTVOICE_OPUS_CONFIG_FRAME_PERIOD,
    
    WL_SMARTVOICE_OPUS_CONFIG_USE_VBR,
    WL_SMARTVOICE_OPUS_CONFIG_CONSTRAINT_USE_VBR,
    WL_SMARTVOICE_OPUS_CONFIG_USE_INBANDFEC,
    WL_SMARTVOICE_OPUS_CONFIG_USE_DTX,
};

static int isOpusEncodeReset = false;
static int isOpusDecodeReset = false;

static bool isOpusInitDone = false;

void wl_smartvoice_opus_init(uint8_t* opus_heap)
{
    memset(opus_heap, 0x00, WL_SMARTVOICE_OPUS_CONFIG_HEAP_SIZE);
    voice_opus_init(&opus_cfg, opus_heap);
    isOpusEncodeReset = true;
    isOpusDecodeReset = true;

    isOpusInitDone = true;
}

void wl_smartvoice_opus_deinit(void)
{
    voice_opus_deinit();
    isOpusEncodeReset = false;
    isOpusDecodeReset = false;

    isOpusInitDone = false;
}

int wl_smartvoice_opus_decode(uint8_t* in, uint32_t in_len,uint8_t* out, uint32_t out_len)
{
    int size;

    if (!isOpusInitDone) {
        return -1;
    }

    size = voice_opus_decode(in, in_len, out, out_len/2, isOpusDecodeReset);

    if (isOpusDecodeReset) {
        isOpusDecodeReset = false;
    }

    return size;
}

int wl_smartvoice_opus_encode(uint8_t* in, uint8_t* out, uint32_t len)
{
    int size;

    if (!isOpusInitDone) {
        return -1;
    }
    
    // 初始化之后的第一次编码，isOpusEncodeReset为true，以后都为false，直到再次初始化opus
    size = voice_opus_encode(in, out, len/2, isOpusEncodeReset);

    if (isOpusEncodeReset) {
        isOpusEncodeReset = false;
    }

    return size;
}

POSSIBLY_UNUSED void wl_smartvoice_sbc_init(void)
{

}

POSSIBLY_UNUSED void wl_smartvoice_sbc_deinit(void)
{
    
}

POSSIBLY_UNUSED int wl_smartvoice_sbc_decode(uint8_t* in, uint32_t in_len,uint8_t* out, uint32_t out_len)
{
    return 0;
}

POSSIBLY_UNUSED int wl_smartvoice_sbc_encode(uint8_t* in, uint32_t in_len,uint8_t* out, uint32_t out_len)
{
    return 0;
}