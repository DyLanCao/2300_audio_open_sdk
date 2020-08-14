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
#ifndef __PMU_BEST3001_H__
#define __PMU_BEST3001_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ISPI_PMU_REG(reg)                   (((reg) & 0xFFF) | 0x0000)

enum PMU_EFUSE_PAGE_T {
    PMU_EFUSE_PAGE_SECURITY     = 0,
    PMU_EFUSE_PAGE_BOOT         = 1,
    PMU_EFUSE_PAGE_FEATURE      = 2,
    PMU_EFUSE_PAGE_BATTER_LV    = 3,

    PMU_EFUSE_PAGE_BATTER_HV    = 4,
    PMU_EFUSE_PAGE_SW_CFG       = 5,
    PMU_EFUSE_PAGE_PROD_TEST    = 6,
    PMU_EFUSE_PAGE_RESERVED_7   = 7,

    PMU_EFUSE_PAGE_RESERVED_8   = 8,
    PMU_EFUSE_PAGE_DCCALIB2_L   = 9,
    PMU_EFUSE_PAGE_DCCALIB2_R   = 10,
    PMU_EFUSE_PAGE_DCCALIB_L    = 11,

    PMU_EFUSE_PAGE_DCCALIB_R    = 12,
    PMU_EFUSE_PAGE_DCXO         = 13,
    PMU_EFUSE_PAGE_KEY_INFO1    = 14,
    PMU_EFUSE_PAGE_KEY_INFO2    = 15,
};

void pmu_codec_hppa_enable(int enable);

void pmu_codec_mic_bias_enable(uint32_t map);

#ifdef SS_SPECIFIC
uint8_t ss_fm_mode_switch (uint32_t fm_mode);
#endif

#if defined(CHIP_HAS_TEMP_SNSR)
void pmu_dco_temp_calib (uint16_t*);
void pmu_temp_sensor_enable (uint8_t);
#endif

#ifdef __cplusplus
}
#endif

#endif

