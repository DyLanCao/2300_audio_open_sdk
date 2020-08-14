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
#include <assert.h>
#include "nvrecord.h"
#include "nvrecord_fp_account_key.h"
#include "hal_trace.h"
#include "co_math.h" 


extern nv_record_struct nv_record_config;
static NV_FP_ACCOUNT_KEY_RECORD_T *nvrecord_fp_account_key_p = NULL;

void nv_record_fp_account_key_init(void)
{
    if (NULL == nvrecord_fp_account_key_p)
    {
        nvrecord_fp_account_key_p = 
            (NV_FP_ACCOUNT_KEY_RECORD_T *)nvrec_config_get_int(nv_record_config.config, 
            (const char *)section_name_fp_account_key, 
            (const char *)NV_RECORD_ACCOUNT_KEY, 
            NVRAM_ACCOUNT_KEY_INVALID);
    }
    
    if ((NV_FP_ACCOUNT_KEY_RECORD_T *)NVRAM_ACCOUNT_KEY_INVALID == nvrecord_fp_account_key_p)
    {
        nvrecord_fp_account_key_p = (NV_FP_ACCOUNT_KEY_RECORD_T *)pool_malloc(sizeof(NV_FP_ACCOUNT_KEY_RECORD_T));
        if (!nvrecord_fp_account_key_p)
        {
            ASSERT(0,"%s pool_malloc failed",__func__);
            return;
        }
        
        memset(nvrecord_fp_account_key_p, 0, sizeof(NV_FP_ACCOUNT_KEY_RECORD_T));
        
        nvrec_config_set_int(nv_record_config.config, section_name_fp_account_key,
            (const char *)NV_RECORD_ACCOUNT_KEY, (int)nvrecord_fp_account_key_p);
    }    
}

void nv_record_fp_account_key_add(NV_FP_ACCOUNT_KEY_ENTRY_T* param_rec)
{
    TRACE("Add accunt key:");
    DUMP8("0x%02x ", param_rec->key, sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));
    
    if (FP_ACCOUNT_KEY_RECORD_NUM == nvrecord_fp_account_key_p->key_count)
    {
        // discard the earliest key
        memcpy((uint8_t *)&(nvrecord_fp_account_key_p->accountKey[0]), 
            (uint8_t *)&(nvrecord_fp_account_key_p->accountKey[1]),
            (FP_ACCOUNT_KEY_RECORD_NUM - 1)*sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));

        nvrecord_fp_account_key_p->key_count--;
    }

    memcpy((uint8_t *)&(nvrecord_fp_account_key_p->accountKey[nvrecord_fp_account_key_p->key_count]), 
            param_rec,
            sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));
    
    nvrecord_fp_account_key_p->key_count++;
    
    nv_record_update_runtime_userdata();
}

bool nv_record_fp_account_key_get_by_index(uint8_t index, uint8_t* outputKey)
{
    if (nvrecord_fp_account_key_p->key_count < (index + 1))
    {
        return false;
    }
    
    memcpy(outputKey, (uint8_t *)&(nvrecord_fp_account_key_p->accountKey[index]),
        sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));

    TRACE("Get accunt key %d as:", index);
    DUMP8("0x%02x ", outputKey, sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));
    
    return true;
}

uint8_t nv_record_fp_account_key_count(void)
{
    return nvrecord_fp_account_key_p->key_count;
}

