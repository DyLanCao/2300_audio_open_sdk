/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "smartvoice_enhancement.h"
#include "lc_mmse_ns.h"
#include "agc.h"
#include "med_memory.h"
#include "hal_trace.h"
#include "app_audio.h"

#undef SPEECH_TX_2MIC_NS2
#undef SPEECH_TX_NS2
#undef SPEECH_TX_AGC

#if defined(SPEECH_TX_2MIC_NS2)
#include "coherent_denoise.h"
CoherentDenoiseState *speech_tx_2mic_ns2_st = NULL;
extern CoherentDenoiseConfig coherent_denoise_cfg;
#endif

#if defined(SPEECH_TX_NS2)
static SpeechNs2State *lc_mmse_ns_st = NULL; 
#endif

#if defined(SPEECH_TX_AGC)
static AgcState *agc_capture = NULL;
const AgcConfig WEAK agc_capture_cfg = {
    .bypass = 0,
    .target_level = 3,
    .compression_gain = 6,
    .limiter_enable = 1,
};
#endif

#define MED_MEM_POOL_SIZE (50 * 1024)

static uint8_t *g_medMemPool = NULL;

void sv_enhancement_init(int sample_rate, int frame_size, int channels)
{
    ASSERT(sample_rate == 16000, "[%s] Only support 16k voice, current samplerate is %d", __FUNCTION__, sample_rate);
    ASSERT(frame_size == SV_ENHANCEMENT_FRAME_SIZE, "[%s] Only support 10ms frame, current frame size is %d", __FUNCTION__, frame_size);

    TRACE("[%s] sample rate - %d, frame size - %d, channels - %d",
        __FUNCTION__, sample_rate, frame_size, channels);

#if MED_MEM_POOL_SIZE > 0
    app_audio_mempool_get_buff(&g_medMemPool, MED_MEM_POOL_SIZE);
    med_heap_init(&g_medMemPool[0], MED_MEM_POOL_SIZE);
#endif

#if defined(SPEECH_TX_2MIC_NS2)
    speech_tx_2mic_ns2_st = coherent_denoise_create(sample_rate, frame_size / channels / 2, &coherent_denoise_cfg);
#endif

#if defined(SPEECH_TX_NS2)
    speech_ns2_create(&lc_mmse_ns_st, sample_rate, frame_size / channels, -10);
#endif

#if defined(SPEECH_TX_AGC)
    agc_capture = agc_state_create(sample_rate, frame_size / channels, &agc_capture_cfg);
    agc_set_config(agc_capture, 2, 3, 1);
#endif
}

void sv_enhancement_destroy(void)
{
#if defined(SPEECH_TX_AGC)
    agc_state_destroy(agc_capture);
#endif

#if defined(SPEECH_TX_NS2)
    speech_ns2_destroy(lc_mmse_ns_st);
#endif

#if defined(SPEECH_TX_2MIC_NS2)
    coheren_denoise_destroy(speech_tx_2mic_ns2_st);
#endif

#if MED_MEM_POOL_SIZE > 0
    uint32_t total = 0, used = 0, max_used = 0;
    med_memory_info(&total, &used, &max_used);
    TRACE("[%s] MED MALLOC MEM: total - %d, used - %d, max_used - %d.",
        __FUNCTION__, total, used, max_used);
    ASSERT(used == 0, "[%s] used != 0", __FUNCTION__);
#endif
}

static void capture_step(int16_t *buf, int16_t *out, int frame_size)
{
    ASSERT(frame_size == SV_ENHANCEMENT_FRAME_SIZE, "[%s] Only support 10ms frame, current frame size is %d",
        __FUNCTION__, frame_size);

#ifdef SPEECH_TX_2MIC_NS2
    coherent_denoise_process(speech_tx_2mic_ns2_st, (int16_t *)buf, frame_size, (int16_t *)out);
    // Channel num: two-->one
    frame_size >>= 1;
#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2
    for (uint32_t i = 0; i < frame_size / 2; i++)
    {
        out[i] = buf[2 * i];      // Left channel
        // buf[i] = buf[2 * i + 1]; // Right channel
    }
    // Channel num: two-->one
    frame_size >>= 1;
#else // single channel
    memcpy(out, buf, sizeof(int16_t) * frame_size);
#endif

#ifdef SPEECH_TX_NS2
    speech_ns2_process(lc_mmse_ns_st, (int16_t *)out, frame_size);
#endif

#if defined(SPEECH_TX_AGC)
    agc_process(agc_capture, (int16_t *)out);
#endif
}

uint32_t sv_enhancement_process_capture(uint8_t *buf, uint32_t len)
{
    int16_t *buf16 = (int16_t *)buf;
    int frame_size = len / 2;
    ASSERT(frame_size % SV_ENHANCEMENT_FRAME_SIZE == 0, "[%s] Only support 10ms * N frame, current frame size is %d",
        __FUNCTION__, frame_size);

// pass through
#if 0
#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2
    // Test MIC: Get one channel data
    // Enable this, should bypass SPEECH_TX_2MIC_NS and SPEECH_TX_2MIC_NS2
    for (uint32_t i = 0; i < frame_size / 2; i++)
    {
        buf16[i] = buf16[2 * i + 1];      // Left channel
        // buf16[i] = buf16[2 * i + 1]; // Right channel
    }

    frame_size >>= 1;

    return len / 2;
#else
    return len;
#endif
#endif
    
    for (int offset = 0; offset < frame_size; offset += SV_ENHANCEMENT_FRAME_SIZE) {
        capture_step(buf16 + offset, buf16 + offset / SPEECH_CODEC_CAPTURE_CHANNEL_NUM, SV_ENHANCEMENT_FRAME_SIZE);
    }

    return len;
}
