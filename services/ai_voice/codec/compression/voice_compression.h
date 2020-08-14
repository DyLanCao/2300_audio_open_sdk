#ifndef __VOICE_COMPRESSION_H__
#define __VOICE_COMPRESSION_H__

#include "voice_opus.h"
#include "voice_sbc.h"

#if VOB_ENCODING_ALGORITHM == ENCODING_ALGORITHM_OPUS
#if defined (__SMART_VOICE__)
#define VOB_VOICE_CAPTURE_INTERVAL_IN_MS        (100)
#elif defined (__AMA_VOICE__)
#define VOB_VOICE_CAPTURE_INTERVAL_IN_MS        (20)
#elif defined (__DMA_VOICE__)
#define VOB_VOICE_CAPTURE_INTERVAL_IN_MS        (20)
#elif defined (__TENCENT_VOICE__)
#define VOB_VOICE_CAPTURE_INTERVAL_IN_MS        (20)
#else
#define VOB_VOICE_CAPTURE_INTERVAL_IN_MS        (100)
#endif
#define VOB_VOICE_CAPTURE_FRAME_CNT             (VOB_VOICE_CAPTURE_INTERVAL_IN_MS/VOICE_FRAME_PERIOD_IN_MS)
#define VOB_VOICE_PCM_FRAME_SIZE                (VOICE_OPUS_PCM_DATA_SIZE_PER_FRAME)
#define VOB_VOICE_PCM_CAPTURE_CHUNK_SIZE        (VOB_VOICE_PCM_FRAME_SIZE*VOB_VOICE_CAPTURE_FRAME_CNT)
#define VOB_VOICE_ENCODED_DATA_FRAME_SIZE       (VOICE_OPUS_ENCODED_DATA_SIZE_PER_FRAME)
#elif VOB_ENCODING_ALGORITHM == ENCODING_ALGORITHM_SBC
#define VOB_VOICE_CAPTURE_INTERVAL_IN_MS        (VOICE_SBC_CAPTURE_INTERVAL_IN_MS)
#define VOB_VOICE_CAPTURE_FRAME_CNT             (VOICE_SBC_CAPTURE_INTERVAL_IN_MS/VOICE_SBC_FRAME_PERIOD_IN_MS)
#define VOB_VOICE_PCM_FRAME_SIZE                (VOICE_SBC_PCM_DATA_SIZE_PER_FRAME)
#define VOB_VOICE_PCM_CAPTURE_CHUNK_SIZE        (VOICE_SBC_PCM_DATA_SIZE_PER_FRAME*VOB_VOICE_CAPTURE_FRAME_CNT)
#define VOB_VOICE_ENCODED_DATA_FRAME_SIZE       (VOICE_SBC_ENCODED_DATA_SIZE_PER_FRAME)
#else
#define VOB_VOICE_CAPTURE_INTERVAL_IN_MS        (100)
#define VOB_VOICE_PCM_FRAME_SIZE                (32000/10)
#define VOB_VOICE_PCM_CAPTURE_CHUNK_SIZE        (VOB_VOICE_PCM_FRAME_SIZE)
#endif

typedef struct {
    uint8_t     algorithm;      // ENCODING_ALGORITHM_OPUS or ENCODING_ALGORITHM_SBC
    uint8_t     frameCountPerXfer;
    uint16_t    encodedFrameSize;
    uint16_t    pcmFrameSize;

    uint8_t     channelCnt;
    uint8_t     complexity;
    uint8_t     packetLossPercentage;
    uint8_t     sizePerSample;
    uint16_t    appType;
    uint16_t    bandWidth;
    uint32_t    bitRate;
    uint32_t    sampleRate;
    uint32_t    signalType;
    uint32_t    periodPerFrame;
    uint8_t     isUseVbr;
    uint8_t     isConstraintUseVbr;
    uint8_t     isUseInBandFec;
    uint8_t     isUseDtx;
} __attribute__((packed)) VOICE_OPUS_CONFIG_INFO_T;

typedef struct {
    uint8_t     algorithm;      // ENCODING_ALGORITHM_OPUS or ENCODING_ALGORITHM_SBC
    uint8_t     frameCountPerXfer;
    uint16_t    encodedFrameSize;
    uint16_t    pcmFrameSize;

    uint16_t    blockSize;
    uint8_t     channelCnt;
    uint8_t     channelMode;
    uint8_t     bitPool;
    uint8_t     sizePerSample;
    uint8_t     sampleRate;
    uint8_t     numBlocks;
    uint8_t     numSubBands;
    uint8_t     mSbcFlag;
    uint8_t     allocMethod;

} __attribute__((packed)) VOICE_SBC_CONFIG_INFO_T;


#ifdef __cplusplus
extern "C" {
#endif
void voice_compression_init(uint8_t encode_type);
void voice_compression_deinit(uint8_t encode_type);

uint32_t voice_compression_handle(uint8_t encode_type, uint8_t *input_buf, uint32_t sampleCount, uint8_t isReset);
uint8_t voice_compression_validity_check();
uint32_t voice_compression_get_encode_buf_size();
int voice_compression_get_encoded_data(uint8_t *out_buf,uint32_t len);
void voice_compression_reset_encode_buf();





#ifdef __cplusplus	
}
#endif

#endif	// __VOICE_SBC_H__

