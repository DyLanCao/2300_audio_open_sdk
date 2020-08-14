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
#ifndef __A2DPPLAY_H__
#define __A2DPPLAY_H__

#include "avdtp_api.h"

typedef struct  {
    uint32_t sample_rate;
    uint8_t num_channels;
    uint8_t bits_depth;
    uint32_t frame_samples;    
    float factor_reference;     
} A2DP_AUDIO_OUTPUT_CONFIG_T;

typedef enum {
    A2DP_AUDIO_CHANNEL_SELECT_STEREO,
    A2DP_AUDIO_CHANNEL_SELECT_LRMERGE,
    A2DP_AUDIO_CHANNEL_SELECT_LCHNL,
    A2DP_AUDIO_CHANNEL_SELECT_RCHNL,
} A2DP_AUDIO_CHANNEL_SELECT_E;

typedef struct {
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint16_t curSubSequenceNumber;
    uint16_t totalSubSequenceNumber;
    uint32_t frame_samples;
    uint32_t decoded_frames;
    uint32_t undecode_frames;
    uint32_t average_frames;
    A2DP_AUDIO_OUTPUT_CONFIG_T *stream_info;
} A2DP_AUDIO_LASTFRAME_INFO_T;

typedef struct{          
    float proportiongain;     
    float integralgain;       
    float derivativegain;  
    float deltaDev_last;
    float alpha;
    float error[3];                  
    float result;                
}A2DP_AUDIO_SYNC_PID_T;

typedef A2DP_AUDIO_LASTFRAME_INFO_T A2DP_AUDIO_SYNCFRAME_INFO_T;

typedef int(*A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK)(btif_media_header_t *, unsigned char *, unsigned int len);

#ifdef __cplusplus
extern "C" {
#endif

uint32_t a2dp_audio_playback_handler(uint8_t *buffer, uint32_t buffer_bytes);
float a2dp_audio_sync_pid_calc(A2DP_AUDIO_SYNC_PID_T *pid, float diff);
int a2dp_audio_sync_init(void);
int a2dp_audio_sync_reset_data(void);
int a2dp_audio_sync_tune_sample_rate(double ratio);
int a2dp_audio_init(A2DP_AUDIO_OUTPUT_CONFIG_T *config, A2DP_AUDIO_CHANNEL_SELECT_E chnl_sel, uint8_t codec_type, uint16_t dest_packet_mut);
int a2dp_audio_deinit(void);
int a2dp_audio_start(void);
int a2dp_audio_stop(void);
int a2dp_audio_detect_next_packet_callback_register(A2DP_AUDIO_DETECT_NEXT_PACKET_CALLBACK callback);
int a2dp_audio_detect_first_packet(void);
int a2dp_audio_store_packet(btif_media_header_t * header, unsigned char *buf, unsigned int len);
int a2dp_audio_discards_packet(uint32_t packets);
int a2dp_audio_discards_samples(uint32_t samples);
int a2dp_audio_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T *sync_info);
int a2dp_audio_get_packet_samples(void);
int a2dp_audio_lastframe_info_ptr_get(A2DP_AUDIO_LASTFRAME_INFO_T **lastframe_info);
int a2dp_audio_lastframe_info_get(A2DP_AUDIO_LASTFRAME_INFO_T *lastframe_info);
void a2dp_audio_clear_input_raw_packet_list(void);
int a2dp_audio_refill_packet(void);

#ifdef __cplusplus
}
#endif

#endif//__A2DPPLAY_H__
