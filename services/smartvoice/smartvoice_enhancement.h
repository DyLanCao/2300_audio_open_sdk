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
#ifndef SMARTVOICE_ENHANCEMENT_H
#define SMARTVOICE_ENHANCEMENT_H

#include <stdint.h>

#define SV_ENHANCEMENT_FRAME_SIZE (16000 / 1000 * 10 * SPEECH_CODEC_CAPTURE_CHANNEL_NUM)

#ifdef __cplusplus
extern "C" {
#endif

void sv_enhancement_init(int sample_rate, int frame_size, int channels);

void sv_enhancement_destroy(void);

uint32_t sv_enhancement_process_capture(uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
