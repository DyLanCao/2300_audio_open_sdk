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
#ifndef __USERDATA_SECTION_H__
#define __USERDATA_SECTION_H__
#include "section_def.h"
#ifdef __cplusplus
extern "C" {
#endif

#define FILTER_IIR_CFG_NUM  10
#define USRDATA_WRITEFLASH_SIZE 256
#define USERDATA_SECTION_MAGIC 0xaaec
#define USERDATA_SECTION_STRUCT_VERSION 1


typedef struct _filter_iir_cfg{
    float32_t fc;   /*Hz*/
    float32_t q;    /*q bw=cf/q*/
    float32_t gain; /*+/- dB*/
}filter_iir_cfg;

typedef struct{
    uint32_t anc_active_index;
    uint32_t volume;
    filter_iir_cfg filter_iir[FILTER_IIR_CFG_NUM];
}_userdata_sec;

typedef struct{
    section_head_t head;
    _userdata_sec userdata;
    uint8_t reserve_data[USRDATA_WRITEFLASH_SIZE - sizeof(section_head_t) - sizeof(_userdata_sec)];
}struct_userdatasec;



#ifdef __cplusplus
}
#endif
#endif

