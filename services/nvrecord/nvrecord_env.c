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
#include "nvrecord_env.h"
#include "hal_trace.h"
#include "customparam_section.h"

#define NV_RECORD_ENV_KEY  "env_key"

extern nv_record_struct nv_record_config;
static struct nvrecord_env_t *nvrecord_env_p = NULL;

static int nv_record_env_new(void)
{

    nvrecord_env_p = (struct nvrecord_env_t *)pool_malloc(sizeof(struct nvrecord_env_t));

    if (!nvrecord_env_p)
    {
        TRACE("%s pool_malloc failed ");
        return -1;
    }
    nvrec_config_set_int(nv_record_config.config,section_name_other,(const char *)NV_RECORD_ENV_KEY,(int)nvrecord_env_p);

    nvrecord_env_p->media_language.language = NVRAM_ENV_MEDIA_LANGUAGE_DEFAULT;
    nvrecord_env_p->ibrt_mode.mode = NVRAM_ENV_TWS_MODE_DEFAULT;
    nvrecord_env_p->factory_tester_status.status = NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT;
#ifdef IS_MULTI_AI_ENABLED
    nvrecord_env_p->voice_key_enable = false;
    nvrecord_env_p->aiManagerInfo.setedCurrentAi = 0;
    nvrecord_env_p->aiManagerInfo.aiStatusDisableFlag = 0;
    nvrecord_env_p->aiManagerInfo.amaAssistantEnableStatus = 1;
#endif
    nv_record_update_runtime_userdata();

    TRACE("%s nvrecord_env_p:%x",__func__, nvrecord_env_p);

    return 0;
}

static int nv_record_env_rebuild(void)
{
    if(nv_record_config_rebuild() != BT_STS_SUCCESS)
        return -1;

    if(nv_record_env_new())
        return -1;

    nv_record_flash_flush();

    return 0;
}

int nv_record_env_init(void)
{
    int nRet = 0;
    nv_record_open(section_usrdata_ddbrecord);

    nv_custom_parameter_section_init();

    nvrecord_env_p = (struct nvrecord_env_t *)nvrec_config_get_int(nv_record_config.config, (const char *)section_name_other,(const char *)NV_RECORD_ENV_KEY, NVRAM_ENV_INVALID);

    if (nvrecord_env_p == (struct nvrecord_env_t *)NVRAM_ENV_INVALID)
    {
        TRACE("NVRAM_ENV_INVALID !!");
        if (nv_record_env_rebuild() < 0)
            nRet = -1;
        else
            TRACE("NVRAM REBUILD SUCCESS !!");
    }
    TRACE("%s nvrecord_env_p:%x",__func__, nvrecord_env_p);
#ifdef __TWS__
    TRACE("tws_mode.mode:%d",nvrecord_env_p->tws_mode.mode);
    DUMP8("0x%02x ", nvrecord_env_p->tws_mode.record.bdAddr.addr, 6);
#endif
    return nRet;
}

int nv_record_env_get(struct nvrecord_env_t **nvrecord_env)
{
    if (!nvrecord_env)
        return -1;

    if (!nvrecord_env_p)
        return -1;

    *nvrecord_env = nvrecord_env_p;

    return 0;
}

int nv_record_env_set(struct nvrecord_env_t *nvrecord_env)
{
    if (!nvrecord_env)
        return -1;

    if (!nvrecord_env_p)
        return -1;

    nv_record_update_runtime_userdata();

    return 0;
}

void nv_record_update_ibrt_info(uint32_t newMode,bt_bdaddr_t *ibrtPeerAddr)
{
    if (nvrecord_env_p)
    {
        TRACE("##%s,%d",__func__,newMode);
        nvrecord_env_p->ibrt_mode.mode = newMode;
        nv_record_config.is_update = true;
        memcpy(nvrecord_env_p->ibrt_mode.record.bdAddr.address,ibrtPeerAddr->address,6);
    }
}
#ifdef WL_DEBUG_MODE
void nv_record_update_ibrt_info_debug_mode(void)
{
    uint32_t newMode=NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT_DEBUG_MODE;

    TRACE("##%s,0x%x",__func__,newMode);

    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);

    nv_record_sector_clear();
    nv_record_env_init();
    nvrecord_env_p->debug_status.status = newMode;
    nv_record_env_set(nvrecord_env);
    nv_record_flash_flush();
}

uint32_t nv_debug_mode_get(void)
{
    return  nvrecord_env_p->debug_status.status;
}

void nv_record_init_board_mode(void)
{
    uint32_t init_flg=NVRAM_ENV_WL_BOARD_MODE;

    TRACE("##%s,0x%x",__func__,init_flg);

    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);

    nv_record_sector_clear();
    nv_record_env_init();
    nvrecord_env_p->debug_status.init_flag = init_flg;
    nv_record_env_set(nvrecord_env);
    nv_record_flash_flush();
}

uint32_t nv_init_flag_mode_get(void)
{
    return  nvrecord_env_p->debug_status.init_flag;
}

#endif 