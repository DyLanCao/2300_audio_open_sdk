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
#ifndef __APP_MEDIA_PLAYER_H__
#define __APP_MEDIA_PLAYER_H__

#include "resources.h"

#define MEDIA_DEFAULT_LANGUAGE (0)

typedef enum
{
    T_AUDIO_ID = 0x0,
        
    T_AUDIO_NUMBER,     
    
}MEDIA_AUDIO_TYPE;

#define MAX_PHB_NUMBER 20

typedef struct 
{
       uint16_t id;
       uint16_t status;
 //>>>>>>>>>>>>>>>>>>>>>>>   APP_AUDIO_STATUS<<<<<<<<<<<<<<<     
 	uint16_t aud_type;
	uint16_t aud_id;
       char phb_number[MAX_PHB_NUMBER];	
       
}msg_num_ptr;

uint32_t media_PlayAudio(AUD_ID_ENUM id,uint8_t device_id);

void media_Set_IncomingNumber(const char* pNumber);

void media_FreeMemory(void* data);

int app_play_audio_onoff(bool on, APP_AUDIO_STATUS* status);

void app_play_audio_set_lang(int L);

int app_play_audio_get_lang();

void app_audio_playback_done(void);

#ifdef AUDIO_LINEIN
uint32_t app_play_audio_lineinmode_more_data(uint8_t *buf, uint32_t len);

int app_play_audio_lineinmode_init(uint8_t mode, uint32_t buff_len);

int app_play_audio_lineinmode_start(APP_AUDIO_STATUS* status);

int app_play_audio_lineinmode_stop(APP_AUDIO_STATUS* status);
#endif

#endif

