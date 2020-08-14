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
#ifndef __APP_SMARTVOICE_H
#define __APP_SMARTVOICE_H

#undef  UUID_128_ENABLE
#define IS_ENABLE_START_DATA_XFER_STEP			0

#define NO_ENCODING                             0
#define ENCODING_ALGORITHM_ADPCM                1
#define ENCODING_ALGORITHM_OPUS                 2
#define ENCODING_ALGORITHM_SBC                 	3

#define IS_USE_ENCODED_DATA_SEQN_PROTECTION    	1
#define IS_SMARTVOICE_DEBUG_ON					1
#define IS_FORCE_STOP_VOICE_STREAM				0

#define APP_SMARTVOICE_PATH_TYPE_SPP	(1 << 0)
#define APP_SMARTVOICE_PATH_TYPE_BLE	(1 << 1)
#define APP_SMARTVOICE_PATH_TYPE_HFP	(1 << 2)

#define APP_SMARTVOICE_PATH_TYPE		(APP_SMARTVOICE_PATH_TYPE_BLE | APP_SMARTVOICE_PATH_TYPE_SPP)
	
#define APP_SMARTVOICE_SPP_CONNECTED             (1 << 0)
#define APP_SMARTVOICE_BLE_CONNECTED             (1 << 1)
#define APP_SMARTVOICE_SPP_BLE_BOTH_CONNECTED    (APP_SMARTVOICE_SPP_CONNECTED | APP_SMARTVOICE_BLE_CONNECTED) 

#define APP_SMARTVOICE_SPP_DISCONNECTED             (~(1 << 0))
#define APP_SMARTVOICE_BLE_DISCONNECTED             (~(1 << 1))
#define APP_SMARTVOICE_SPP_BLE_BOTH_DISCONNECTED    (0)         

#define APP_SMARTVOICE_VOICE_OVER_SPP            (1)
#define APP_SMARTVOICE_VOICE_OVER_BLE            (2)

#define APP_SMARTVOICE_MIC_TIMEOUT_INTERVEL           (8000)

#ifdef __cplusplus
extern "C" {
#endif

enum {
    APP_SMARTVOICE_STATE_IDLE,
    APP_SMARTVOICE_STATE_MIC_STARTING,
    APP_SMARTVOICE_STATE_MIC_STARTED,
    APP_SMARTVOICE_STATE_SPEAKER_STARTING,
    APP_SMARTVOICE_STATE_SPEAKER_STARTED,
    APP_SMARTVOICE_STATE_MAX
};

typedef enum
{
	CLOUD_MUSIC_CONTROL_NEXT = 0,
	CLOUD_MUSIC_CONTROL_PRE,
	CLOUD_MUSIC_CONTROL_PAUSE,
	CLOUD_MUSIC_CONTROL_RESUME
} CLOUD_MUSIC_CONTROL_REQ_E;



void app_smartvoice_config_mtu(uint16_t MTU);
void app_sv_start_voice_stream_via_hfp(void);
void app_sv_stop_voice_stream(void);
void app_sv_start_voice_stream_via_ble(void);
void app_sv_control_cloud_music_playing(CLOUD_MUSIC_CONTROL_REQ_E req);

void app_smartvoice_switch_hfp_stream_status(void);
void app_smartvoice_switch_ble_stream_status(void);
void app_smartvoice_switch_spp_stream_status(void);

void app_sv_switch_cloud_platform(void);

void app_sv_throughput_test_init(void);
void app_sv_stop_throughput_test(void);
void app_sv_throughput_test_transmission_handler(void);
#ifdef __cplusplus
}
#endif

#endif

