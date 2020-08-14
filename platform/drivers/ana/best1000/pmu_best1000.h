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
#ifndef __PMU_BEST1000_H__
#define __PMU_BEST1000_H__

#ifdef __cplusplus
extern "C" {
#endif

enum PMU_EFUSE_PAGE_T {
    PMU_EFUSE_PAGE_SECURITY     = 0,
    PMU_EFUSE_PAGE_BOOT         = 1,
    PMU_EFUSE_PAGE_FEATURE      = 2,
    PMU_EFUSE_PAGE_BATTER_LV    = 3,

    PMU_EFUSE_PAGE_BATTER_HV    = 4,
    PMU_EFUSE_PAGE_RESERVED_5   = 5,
    PMU_EFUSE_PAGE_RESERVED_6   = 6,
    PMU_EFUSE_PAGE_RESERVED_7   = 7,

    PMU_EFUSE_PAGE_RESERVED_8   = 8,
    PMU_EFUSE_PAGE_DCCALIB2_L   = 9,
    PMU_EFUSE_PAGE_DCCALIB2_R   = 10,
    PMU_EFUSE_PAGE_DCCALIB_L    = 11,

    PMU_EFUSE_PAGE_DCCALIB_R    = 12,
    PMU_EFUSE_PAGE_BONDING      = 13,
    PMU_EFUSE_PAGE_INTERNAL     = 14,
    PMU_EFUSE_PAGE_PROD_TEST    = 15,
};

void pmu_codec_adc_pre_start(void);

void pmu_codec_adc_post_start(void (*delay_ms)(uint32_t));

void pmu_codec_dac_pre_start(void);

void pmu_codec_dac_post_start(void (*delay_ms)(uint32_t));

void pmu_fm_config(int enable);

#ifdef __cplusplus
}
#endif

#endif

