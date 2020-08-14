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
#ifndef __PMU_BEST2000_H__
#define __PMU_BEST2000_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_cmu.h"

enum PMU_EFUSE_PAGE_T {
    PMU_EFUSE_PAGE_SECURITY     = 0,
    PMU_EFUSE_PAGE_BOOT         = 1,
    PMU_EFUSE_PAGE_FEATURE      = 2,
    PMU_EFUSE_PAGE_BATTER_LV    = 3,

    PMU_EFUSE_PAGE_BATTER_HV    = 4,
    PMU_EFUSE_PAGE_SW_CFG       = 5,
    PMU_EFUSE_PAGE_PROD_TEST    = 6,
    PMU_EFUSE_PAGE_RESERVED_7   = 7,

    PMU_EFUSE_PAGE_BT_POWER     = 8,
    PMU_EFUSE_PAGE_DCCALIB2_L   = 9,
    PMU_EFUSE_PAGE_DCCALIB2_R   = 10,
    PMU_EFUSE_PAGE_DCCALIB_L    = 11,

    PMU_EFUSE_PAGE_DCCALIB_R    = 12,
    PMU_EFUSE_PAGE_BONDING      = 13,
    PMU_EFUSE_PAGE_KEY_INFO1    = 14,
    PMU_EFUSE_PAGE_KEY_INFO2    = 15,
};

enum PMU_PLL_DIV_TYPE_T {
    PMU_PLL_DIV_DIG,
    PMU_PLL_DIV_CODEC,
};

// TODO: To be removed
typedef int (*PMU_EFUSE_GET_HANDLER)(int page, unsigned short *efuse);

void pmu_override_efuse_get_handler(PMU_EFUSE_GET_HANDLER hdlr);

void pmu_codec_hppa_enable(int enable);

void pmu_codec_mic_bias_enable(uint32_t map);

void pmu_pll_div_reset_set(enum HAL_CMU_PLL_T pll);

void pmu_pll_div_reset_clear(enum HAL_CMU_PLL_T pll);

void pmu_pll_div_set(enum HAL_CMU_PLL_T pll, enum PMU_PLL_DIV_TYPE_T type, uint32_t div);

void pmu_ntc_capture_enable(void);

void pmu_ntc_capture_disable(void);


#ifdef __cplusplus
}
#endif

#endif

