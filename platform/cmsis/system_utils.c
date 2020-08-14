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
#include "cmsis.h"
#include "hal_cache.h"
#include "hal_cmu.h"
#include "hal_location.h"
#if !defined(NOSTD) && defined(ACCURATE_DB_TO_FLOAT)
#include <math.h>
#endif

extern uint32_t __got_info_start[];

void BOOT_TEXT_FLASH_LOC GotBaseInit(void)
{
    asm volatile("ldr r9, =__got_info_start");
}

// -----------------------------------------------------------
// Boot initialization
// CAUTION: This function and all the functions it called must
//          NOT access global data/bss, for the global data/bss
//          is not available at that time.
// -----------------------------------------------------------
extern uint32_t __boot_sram_start_flash__[];
extern uint32_t __boot_sram_end_flash__[];
extern uint32_t __boot_sram_start__[];
extern uint32_t __boot_bss_sram_start__[];
extern uint32_t __boot_bss_sram_end__[];

extern uint32_t __sram_text_data_start_flash__[];
extern uint32_t __sram_text_data_end_flash__[];
extern uint32_t __sram_text_data_start__[];
extern uint32_t __sram_bss_start__[];
extern uint32_t __sram_bss_end__[];
extern uint32_t __fast_sram_text_data_start__[];
extern uint32_t __fast_sram_text_data_end__[];
extern uint32_t __fast_sram_text_data_start_flash__[];
extern uint32_t __fast_sram_text_data_end_flash__[];

void BOOT_TEXT_FLASH_LOC BootInit(void)
{
    uint32_t *dst, *src;

    // Enable icache
    hal_cache_enable(HAL_CACHE_ID_I_CACHE);
    // Enable dcache
    hal_cache_enable(HAL_CACHE_ID_D_CACHE);
    // Enable write buffer
    hal_cache_writebuffer_enable(HAL_CACHE_ID_D_CACHE);

    // Init GOT base register
    GotBaseInit();

    // Init boot sections
    for (dst = __boot_sram_start__, src = __boot_sram_start_flash__;
            src < __boot_sram_end_flash__;
            dst++, src++) {
        *dst = *src;
    }

    for (dst = __boot_bss_sram_start__; dst < __boot_bss_sram_end__; dst++) {
        *dst = 0;
    }

#ifdef FPGA
    hal_cmu_fpga_setup();
#else
    hal_cmu_setup();
#endif

    for (dst = __sram_text_data_start__, src = __sram_text_data_start_flash__;
            src < __sram_text_data_end_flash__;
            dst++, src++) {
        *dst = *src;
    }

    for (dst = __sram_bss_start__; dst < __sram_bss_end__; dst++) {
        *dst = 0;
    }

    for (dst = __fast_sram_text_data_start__, src = __fast_sram_text_data_start_flash__;
            src < __fast_sram_text_data_end_flash__;
            dst++, src++) {
        *dst = *src;
    }
}

// -----------------------------------------------------------
// Mutex flag
// -----------------------------------------------------------

int set_bool_flag(bool *flag)
{
    bool busy;

    do {
        busy = (bool)__LDREXB((unsigned char *)flag);
        if (busy) {
            __CLREX();
            return -1;
        }
    } while (__STREXB(true, (unsigned char *)flag));
    __DMB();

    return 0;
}

void clear_bool_flag(bool *flag)
{
    *flag = false;
    __DMB();
}

// -----------------------------------------------------------
// Misc
// -----------------------------------------------------------

float db_to_float(int32_t db)
{
    float coef;

#if !defined(NOSTD) && defined(ACCURATE_DB_TO_FLOAT)
    // The math lib will consume 4K+ bytes of space
    coef = powf(10, (float)db / 20);
#else
    static const float factor_m9db = 0.35481339;
    static const float factor_m3db = 0.70794578;
    static const float factor_m1db = 0.89125094;
    uint32_t cnt;
    uint32_t remain;
    int i;

    coef = 1;

    if (db < 0) {
        cnt = -db / 9;
        remain = -db % 9;
        for (i = 0; i < cnt; i++) {
            coef *= factor_m9db;
        }
        cnt = remain / 3;
        remain = remain % 3;
        for (i = 0; i < cnt; i++) {
            coef *= factor_m3db;
        }
        for (i = 0; i < remain; i++) {
            coef *= factor_m1db;
        }
    } else if (db > 0) {
        cnt = db / 9;
        remain = db % 9;
        for (i = 0; i < cnt; i++) {
            coef /= factor_m9db;
        }
        cnt = remain / 3;
        remain = remain % 3;
        for (i = 0; i < cnt; i++) {
            coef /= factor_m3db;
        }
        for (i = 0; i < remain; i++) {
            coef /= factor_m1db;
        }
    }
#endif

    return coef;
}

uint32_t get_msb_pos(uint32_t val)
{
    uint32_t lead_zero;

    lead_zero = __CLZ(val);
    return (lead_zero >= 32) ? 32 : 31 - lead_zero;
}

uint32_t get_lsb_pos(uint32_t val)
{
    return __CLZ(__RBIT(val));
}

