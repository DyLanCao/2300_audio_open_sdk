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
#ifndef __APP_ANC_H__
#define __APP_ANC_H__

#include "hal_aud.h"
#include "app_key.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_anc_set_playback_samplerate(enum AUD_SAMPRATE_T sample_rate);
void app_anc_init(enum AUD_IO_PATH_T input_path, enum AUD_SAMPRATE_T playback_rate, enum AUD_SAMPRATE_T capture_rate);
void app_anc_close(void);
void app_anc_enable(void);
void app_anc_disable(void);
bool anc_enabled(void);
void test_anc(void);
void app_anc_resample(uint32_t res_ratio, uint32_t *in, uint32_t *out, uint32_t samples);
void app_anc_key(APP_KEY_STATUS *status, void *param);
int app_anc_open_module(void);
int app_anc_close_module(void);
enum AUD_SAMPRATE_T app_anc_get_play_rate(void);
bool app_anc_work_status(void);
void app_anc_send_howl_evt(uint32_t howl);
uint32_t app_anc_get_anc_status(void);
bool app_pwr_key_monitor_get_val(void);
bool app_anc_switch_get_val(void);
void app_anc_ios_init(void);

void app_anc_set_init_done(void);
bool app_anc_set_reboot(void);
void app_anc_status_post(uint8_t status);

#ifdef __cplusplus
}
#endif

#endif
