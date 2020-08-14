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
#ifndef __COHERENT_DENOISE_H__
#define __COHERENT_DENOISE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t     bypass;
    float       delay_taps;             // MIC L/R delay samples. 0: 适用于麦克距离为<2cm; 1: 适用于麦克距离为2cm左右; 2: 适用于麦克距离为4cm左右
    int32_t     freq_smooth_enable;     // 1: 频域滤波打开; 0: 频域滤波关闭; 默认打开
    int32_t     wnr_enable;             // wind noise reduction enable
} CoherentDenoiseConfig;

struct CoherentDenoiseState_;

typedef struct CoherentDenoiseState_ CoherentDenoiseState;

#define CONSTRUCT_FUNC_NAME_A(p, c, m)          p ## _ ## c ## _ ## m
#define CONSTRUCT_FUNC_NAME(p, c, m)            CONSTRUCT_FUNC_NAME_A(p, c, m)

#ifndef COHERENT_DENOISE_IMPL
#define COHERENT_DENOISE_IMPL fixed
#endif

#define coherent_denoise_create    CONSTRUCT_FUNC_NAME(coherent_denoise, COHERENT_DENOISE_IMPL, create)
#define coheren_denoise_destroy    CONSTRUCT_FUNC_NAME(coherent_denoise, COHERENT_DENOISE_IMPL, destroy)
#define coheren_denoise_set_config CONSTRUCT_FUNC_NAME(coherent_denoise, COHERENT_DENOISE_IMPL, set_config)
#define coherent_denoise_process   CONSTRUCT_FUNC_NAME(coherent_denoise, COHERENT_DENOISE_IMPL, process)

CoherentDenoiseState *coherent_denoise_create(int32_t sample_rate, int32_t frame_size, const CoherentDenoiseConfig *cfg);

int32_t coheren_denoise_destroy(CoherentDenoiseState *st);

int32_t coheren_denoise_set_config(CoherentDenoiseState *st, const CoherentDenoiseConfig *cfg);

int32_t coherent_denoise_process(CoherentDenoiseState *st, short *pcm_buf, int32_t pcm_len, short *out_buf);

#ifdef __cplusplus
}
#endif

#endif
