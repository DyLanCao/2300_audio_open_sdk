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
#include "plat_addr_map.h"
#include "reg_cmu_best1400.h"
#include "hal_cmu.h"
#include "hal_aud.h"
#include "hal_bootmode.h"
#include "hal_chipid.h"
#include "hal_location.h"
#include "hal_psc.h"
#include "hal_sleep_mcu_pd.h"
#include "hal_timer.h"
#include "cmsis_nvic.h"
#include "pmu.h"
#include "hal_trace.h"

#define HAL_CMU_PWM_SLOW_CLOCK          (32 * 1000)

#define HAL_CMU_PLL_LOCKED_TIMEOUT      US_TO_TICKS(200)
#define HAL_CMU_26M_READY_TIMEOUT       MS_TO_TICKS(3)

enum CMU_USB_CLK_SRC_T {
    CMU_USB_CLK_SRC_PLL_60M         = 0,
    CMU_USB_CLK_SRC_6M              = 1,
    CMU_USB_CLK_SRC_PLL_48M         = 2,
    CMU_USB_CLK_SRC_OSC_24M_X2      = 3,
};

static struct CMU_T * const cmu = (struct CMU_T *)CMU_BASE;

BOOT_TEXT_SRAM_LOC int hal_cmu_clock_enable(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        cmu->HCLK_ENABLE = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        cmu->PCLK_ENABLE = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        cmu->OCLK_ENABLE = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return 0;
}

BOOT_TEXT_SRAM_LOC int hal_cmu_clock_disable(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        cmu->HCLK_DISABLE = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        cmu->PCLK_DISABLE = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        cmu->OCLK_DISABLE = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return 0;
}

enum HAL_CMU_CLK_STATUS_T hal_cmu_clock_get_status(enum HAL_CMU_MOD_ID_T id)
{
    uint32_t status;

    if (id >= HAL_CMU_MOD_QTY) {
        return HAL_CMU_CLK_DISABLED;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        status = cmu->HCLK_ENABLE & (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        status = cmu->PCLK_ENABLE & (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        status = cmu->OCLK_ENABLE & (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return status ? HAL_CMU_CLK_ENABLED : HAL_CMU_CLK_DISABLED;
}

int hal_cmu_clock_set_mode(enum HAL_CMU_MOD_ID_T id, enum HAL_CMU_CLK_MODE_T mode)
{
    __IO uint32_t *reg;
    uint32_t val;
    uint32_t lock;

    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        reg = &cmu->HCLK_MODE;
        val = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        reg = &cmu->PCLK_MODE;
        val = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        reg = &cmu->OCLK_MODE;
        val = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    lock = int_lock();
    if (mode == HAL_CMU_CLK_MANUAL) {
        *reg |= val;
    } else {
        *reg &= ~val;
    }
    int_unlock(lock);

    return 0;
}

enum HAL_CMU_CLK_MODE_T hal_cmu_clock_get_mode(enum HAL_CMU_MOD_ID_T id)
{
    uint32_t mode;

    if (id >= HAL_CMU_MOD_QTY) {
        return HAL_CMU_CLK_DISABLED;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        mode = cmu->HCLK_MODE & (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        mode = cmu->PCLK_MODE & (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        mode = cmu->OCLK_MODE & (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return mode ? HAL_CMU_CLK_MANUAL : HAL_CMU_CLK_AUTO;
}

int hal_cmu_reset_set(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        cmu->HRESET_SET = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        cmu->PRESET_SET = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        cmu->ORESET_SET = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return 0;
}

int hal_cmu_reset_clear(enum HAL_CMU_MOD_ID_T id)
{
    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        cmu->HRESET_CLR = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        cmu->PRESET_CLR = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        cmu->ORESET_CLR = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return 0;
}

enum HAL_CMU_RST_STATUS_T hal_cmu_reset_get_status(enum HAL_CMU_MOD_ID_T id)
{
    uint32_t status;

    if (id >= HAL_CMU_MOD_QTY) {
        return HAL_CMU_RST_SET;
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        status = cmu->HRESET_SET & (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        status = cmu->PRESET_SET & (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        status = cmu->ORESET_SET & (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }

    return status ? HAL_CMU_RST_CLR : HAL_CMU_RST_SET;
}

int hal_cmu_reset_pulse(enum HAL_CMU_MOD_ID_T id)
{
    volatile int i;

    if (id >= HAL_CMU_MOD_QTY) {
        return 1;
    }

    if (hal_cmu_reset_get_status(id) == HAL_CMU_RST_SET) {
        return hal_cmu_reset_clear(id);
    }

    if (id < HAL_CMU_MOD_P_CMU) {
        cmu->HRESET_PULSE = (1 << id);
    } else if (id < HAL_CMU_MOD_O_SLEEP) {
        cmu->PRESET_PULSE = (1 << (id - HAL_CMU_MOD_P_CMU));
    } else {
        cmu->ORESET_PULSE = (1 << (id - HAL_CMU_MOD_O_SLEEP));
    }
    // Delay 5+ PCLK cycles (10+ HCLK cycles)
    for (i = 0; i < 3; i++);

    return 0;
}

void hal_cmu_timer_select_fast(void)
{
    uint32_t lock;

    lock = int_lock();
    // 6.5M
    cmu->UART_CLK |= (1 << CMU_SEL_TIMER_FAST_SHIFT);
    int_unlock(lock);
}

void hal_cmu_timer_select_slow(void)
{
    uint32_t lock;

    lock = int_lock();
    // 16K
    cmu->UART_CLK &= ~(1 << CMU_SEL_TIMER_FAST_SHIFT);
    int_unlock(lock);
}

void hal_cmu_timer1_select_fast(void)
{
    uint32_t lock;

    lock = int_lock();
    // 6.5M
    cmu->UART_CLK |= (1 << (CMU_SEL_TIMER_FAST_SHIFT + 1));
    int_unlock(lock);
}

void hal_cmu_timer1_select_slow(void)
{
    uint32_t lock;

    lock = int_lock();
    // 16K
    cmu->UART_CLK &= ~(1 << (CMU_SEL_TIMER_FAST_SHIFT + 1));
    int_unlock(lock);
}

BOOT_TEXT_SRAM_LOC static void hal_cmu_clksrc_on(enum HAL_CMU_FREQ_T freq)
{
    switch(freq) {
    case HAL_CMU_FREQ_208M:
    case HAL_CMU_FREQ_78M:
        cmu->SYS_CLK_ENABLE = CMU_PU_DB104M_ENABLE;
        hal_sys_timer_delay(US_TO_TICKS(5));
        cmu->SYS_CLK_ENABLE = CMU_EN_DB104M_ENABLE;
        break;
    case HAL_CMU_FREQ_104M:
        // TODO: config analog/rf to switch clock 104M
        cmu->SYS_CLK_ENABLE = CMU_PU_DB104M_ENABLE;
        hal_sys_timer_delay(US_TO_TICKS(5));
        cmu->SYS_CLK_ENABLE = CMU_EN_DB104M_ENABLE;
        break;
    case HAL_CMU_FREQ_52M:
        cmu->SYS_CLK_ENABLE = CMU_PU_DOUBLER_ENABLE;
        hal_sys_timer_delay(US_TO_TICKS(5));
        cmu->SYS_CLK_ENABLE = CMU_EN_DOUBLER_ENABLE;
        break;
    case HAL_CMU_FREQ_26M:
        cmu->SYS_CLK_ENABLE = CMU_EN_CLK_TOP_OSC_ENABLE;
        break;
    default:
        break;
    }
}

BOOT_TEXT_SRAM_LOC int hal_cmu_flash_set_freq(enum HAL_CMU_FREQ_T freq)
{
    uint32_t enable;
    uint32_t disable;

    if (freq >= HAL_CMU_FREQ_QTY) {
        return 1;
    }
    switch(freq) {
    case HAL_CMU_FREQ_208M:
        /* real clock freq is 156MHz */
        enable  = CMU_SEL_FLS_HIGH_ENABLE
                | CMU_SEL_FLS_H_OSCX6_ENABLE;
        disable = 0;
        break;
    case HAL_CMU_FREQ_104M:
        enable  = CMU_SEL_FLS_HIGH_ENABLE;
        disable = CMU_SEL_FLS_H_OSCX6_DISABLE;
        break;
    case HAL_CMU_FREQ_78M:
        enable  = CMU_SEL_FLS_MID_ENABLE
                | CMU_SEL_FLS_M_OSCX3_ENABLE;
        disable = CMU_SEL_FLS_HIGH_DISABLE;
        break;
    case HAL_CMU_FREQ_52M:
        enable  = CMU_SEL_FLS_MID_ENABLE;
        disable = CMU_SEL_FLS_HIGH_DISABLE
                | CMU_SEL_FLS_M_OSCX3_DISABLE;
        break;
    case HAL_CMU_FREQ_26M:
        enable  = CMU_SEL_FLS_L_OSC_ENABLE;

        disable = CMU_SEL_FLS_HIGH_DISABLE
                | CMU_SEL_FLS_MID_DISABLE;
        break;
    case HAL_CMU_FREQ_32K:
        enable = 0;
        disable = CMU_SEL_FLS_HIGH_DISABLE
                | CMU_SEL_FLS_MID_DISABLE
                | CMU_SEL_FLS_L_OSC_DISABLE;
        break;
    default:
        return 1;
    }


    hal_cmu_clksrc_on(freq);

    hal_cmu_clock_disable(HAL_CMU_MOD_O_FLASH);

    cmu->SYS_CLK_ENABLE = enable;
    cmu->SYS_CLK_DISABLE = disable;

    hal_cmu_clock_enable(HAL_CMU_MOD_O_FLASH);

    return 0;
}

enum HAL_CMU_FREQ_T BOOT_TEXT_SRAM_LOC hal_cmu_flash_get_freq(void)
{
    uint32_t sys_clk;

    sys_clk = cmu->SYS_CLK_ENABLE;

    if (sys_clk & CMU_SEL_FLS_HIGH_ENABLE) {
        if (sys_clk & CMU_SEL_FLS_H_OSCX6_ENABLE)
            /* real clock freq is 156MHz */
            return HAL_CMU_FREQ_208M;
        else
            return HAL_CMU_FREQ_104M;
    } else if (sys_clk & CMU_SEL_FLS_MID_ENABLE) {
        if (sys_clk & CMU_SEL_FLS_M_OSCX3_ENABLE)
            return HAL_CMU_FREQ_78M;
        else
            return HAL_CMU_FREQ_52M;
    } else {
        if (sys_clk & CMU_SEL_FLS_L_OSC_ENABLE)
            return HAL_CMU_FREQ_26M;
    }
    return HAL_CMU_FREQ_32K;
}

// Clock selection diagram by Hua Li:
/*

             ----
clk_32k -->--|0  \
             |   |-->----   sel_sys_mid
clk_26m -->--|1  /      |        |
             ----       |      ----
               |        ---->--|0  \
         sel_sys_l_osc         |   |-->----
                        ---->--|1  /      |
             ----       |      ----       |     ----
clk_52m -->--|0  \      |                 --->--|0  \
             |   |-->----                       |   |-->-- clk_sys
clk_78m -->--|1  /                        --->--|1  /
             ----                         |     ----
               |          sel_sys_h_oscx6 |       |
         sel_sys_m_oscx3         |        |  sel_sys_high
                               ----       |
clk_104m -->[CG]>-clk_104m-->--|0  \      |
          |                    |   |-->----
          ->[CG]>-clk_156m-->--|1  /
                               ----

*/

BOOT_TEXT_SRAM_LOC int hal_cmu_sys_set_freq(enum HAL_CMU_FREQ_T freq)
{
    uint32_t enable;
    uint32_t disable;

    if (freq >= HAL_CMU_FREQ_QTY) {
        return 1;
    }
    switch(freq) {
    case HAL_CMU_FREQ_208M:
        /* real clock freq is 156MHz */
        enable  = CMU_SEL_SYS_HIGH_ENABLE
                | CMU_SEL_SYS_H_OSCX6_ENABLE;
        disable = 0;
        break;
    case HAL_CMU_FREQ_104M:
        enable  = CMU_SEL_SYS_HIGH_ENABLE;
        disable = CMU_SEL_SYS_H_OSCX6_DISABLE;
        break;
    case HAL_CMU_FREQ_78M:
        enable  = CMU_SEL_SYS_MID_ENABLE
                | CMU_SEL_SYS_M_OSCX3_ENABLE;
        disable = CMU_SEL_SYS_HIGH_DISABLE;
        break;
    case HAL_CMU_FREQ_52M:
        enable  = CMU_SEL_SYS_MID_ENABLE;
        disable = CMU_SEL_SYS_HIGH_DISABLE
                | CMU_SEL_SYS_M_OSCX3_DISABLE;
        break;
    case HAL_CMU_FREQ_26M:
        enable  = CMU_SEL_SYS_L_OSC_ENABLE;
        disable = CMU_SEL_SYS_HIGH_DISABLE
                | CMU_SEL_SYS_MID_DISABLE;
        break;
    case HAL_CMU_FREQ_32K:
        enable = 0;
        disable = CMU_SEL_SYS_HIGH_DISABLE
                | CMU_SEL_SYS_MID_DISABLE
                | CMU_SEL_SYS_L_OSC_DISABLE;
        break;
    default:
        return 1;
    }

    hal_cmu_clksrc_on(freq);

    cmu->SYS_CLK_ENABLE = enable;
    cmu->SYS_CLK_DISABLE = disable;

    return 0;
}

int hal_cmu_mem_set_freq(enum HAL_CMU_FREQ_T freq)
{
    return 0;
}

enum HAL_CMU_FREQ_T BOOT_TEXT_SRAM_LOC hal_cmu_sys_get_freq(void)
{
    uint32_t sys_clk;

    sys_clk = cmu->SYS_CLK_ENABLE;

    if (sys_clk & CMU_SEL_SYS_HIGH_ENABLE) {
        if (sys_clk & CMU_SEL_SYS_H_OSCX6_ENABLE)
            /* real clock freq is 156MHz */
            return HAL_CMU_FREQ_208M;
        else
            return HAL_CMU_FREQ_104M;
    } else if (sys_clk & CMU_SEL_SYS_MID_ENABLE) {
        if (sys_clk & CMU_SEL_SYS_M_OSCX3_ENABLE)
            return HAL_CMU_FREQ_78M;
        else
            return HAL_CMU_FREQ_52M;
    } else {
        if (sys_clk & CMU_SEL_SYS_L_OSC_ENABLE)
            return HAL_CMU_FREQ_26M;
    }
    return HAL_CMU_FREQ_32K;
}

int BOOT_TEXT_SRAM_LOC hal_cmu_flash_select_pll(enum HAL_CMU_PLL_T pll)
{
    return 0;
}

int hal_cmu_mem_select_pll(enum HAL_CMU_PLL_T pll)
{
    return 0;
}

int hal_cmu_sys_select_pll(enum HAL_CMU_PLL_T pll)
{
    return 0;
}

int hal_cmu_get_pll_status(enum HAL_CMU_PLL_T pll)
{
    return 0;
}

int hal_cmu_pll_disable(enum HAL_CMU_PLL_T pll, enum HAL_CMU_PLL_USER_T user)
{
    return 0;
}

int hal_cmu_pll_enable(enum HAL_CMU_PLL_T pll, enum HAL_CMU_PLL_USER_T user)
{
    return 0;
}

uint32_t hal_cmu_codec_adc_get_div(void)
{
    return 0;
}

int hal_cmu_codec_adc_set_div(uint32_t div)
{
    return 0;
}

int hal_cmu_codec_dac_set_div(uint32_t div)
{
    return 0;
}

uint32_t hal_cmu_codec_dac_get_div(void)
{
    return 0;
}

void hal_cmu_codec_iir_enable(uint32_t speed)
{
    uint32_t clk, lock;
    enum HAL_CMU_FREQ_T freq = HAL_CMU_FREQ_26M;

    lock = int_lock();

    clk = cmu->CODEC_CLK;
    clk &= ~(CMU_SEL_IIR_HIGH
            | CMU_SEL_IIR_MID
            | CMU_SEL_IIR_H_OSCX6
            | CMU_SEL_IIR_M_OSCX3);

    if (speed >= 156000000) {
        clk |= (CMU_SEL_IIR_HIGH | CMU_SEL_IIR_H_OSCX6);
        freq = HAL_CMU_FREQ_208M;
    } else if (speed >= 104000000) {
        clk |= CMU_SEL_IIR_HIGH;
        freq = HAL_CMU_FREQ_104M;
    } else if (speed >= 78000000) {
        clk |= (CMU_SEL_IIR_MID | CMU_SEL_IIR_M_OSCX3);
        freq = HAL_CMU_FREQ_78M;
    } else if (speed >= 52000000) {
        clk |= CMU_SEL_IIR_MID;
        freq = HAL_CMU_FREQ_52M;
    }

    hal_cmu_clksrc_on(freq);

    cmu->CODEC_CLK = clk;

    int_unlock(lock);
}

void hal_cmu_codec_iir_disable(void)
{
    uint32_t lock;

    lock = int_lock();

    // IIR default clock is 26MHz
    cmu->CODEC_CLK &= ~(CMU_SEL_IIR_HIGH
            | CMU_SEL_IIR_MID
            | CMU_SEL_IIR_H_OSCX6
            | CMU_SEL_IIR_M_OSCX3);

    int_unlock(lock);
}

int hal_cmu_codec_iir_set_div(uint32_t div)
{
    return 0;
}

void hal_cmu_codec_rs_enable(uint32_t speed)
{
#ifdef __AUDIO_RESAMPLE__
    if (hal_cmu_get_audio_resample_status()) {
        uint32_t lock;
        lock = int_lock();
        //cmu->CODEC_CLK |= CMU_SEL_OSC_CODECRS | CMU_SEL_OSCX2_CODECRS;
        int_unlock(lock);
    }
#endif
}

void hal_cmu_codec_rs_disable(void)
{
#ifdef __AUDIO_RESAMPLE__
    if (hal_cmu_get_audio_resample_status()) {
        uint32_t lock;
        lock = int_lock();
        //cmu->CODEC_CLK |= CMU_SEL_OSC_CODECRS | CMU_SEL_OSCX2_CODECRS;
        int_unlock(lock);
    }
#endif
}

void hal_cmu_codec_clock_enable(void)
{
    hal_cmu_clock_enable(HAL_CMU_MOD_H_CODEC);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_CODEC);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECRS);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECDA0);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECDA1);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECADC);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECAD0);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECAD1);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_CODECAD2);
}

void hal_cmu_codec_clock_disable(void)
{
    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECRS);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODEC);
    hal_cmu_clock_disable(HAL_CMU_MOD_H_CODEC);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECDA0);    
    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECDA1);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECADC);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECAD0);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECAD1);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_CODECAD2);
}

void hal_cmu_codec_reset_set(void)
{
    hal_cmu_reset_set(HAL_CMU_MOD_O_CODECRS);
    hal_cmu_reset_set(HAL_CMU_MOD_O_CODEC);
    hal_cmu_reset_set(HAL_CMU_MOD_H_CODEC);
    hal_cmu_reset_set(HAL_CMU_MOD_O_CODECDA0);   
    hal_cmu_reset_set(HAL_CMU_MOD_O_CODECDA1);
    hal_cmu_reset_set(HAL_CMU_MOD_O_CODECADC);
    hal_cmu_reset_set(HAL_CMU_MOD_O_CODECAD0);
    hal_cmu_reset_set(HAL_CMU_MOD_O_CODECAD1);
    hal_cmu_reset_set(HAL_CMU_MOD_O_CODECAD2);
}

void hal_cmu_codec_reset_clear(void)
{
    hal_cmu_reset_clear(HAL_CMU_MOD_H_CODEC);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_CODEC);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECRS);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECDA0);   
    hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECDA1);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECADC);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECAD0);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECAD1);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_CODECAD2);
}

void hal_cmu_i2s_clock_out_enable(enum HAL_I2S_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2C_CLK |= CMU_EN_CLK_I2S_OUT;
    int_unlock(lock);
}

void hal_cmu_i2s_clock_out_disable(enum HAL_I2S_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2C_CLK &= ~CMU_EN_CLK_I2S_OUT;
    int_unlock(lock);
}

void hal_cmu_i2s_set_slave_mode(enum HAL_I2S_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2C_CLK |= CMU_SEL_I2S_CLKIN;
    int_unlock(lock);
}

void hal_cmu_i2s_set_master_mode(enum HAL_I2S_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2C_CLK &= ~CMU_SEL_I2S_CLKIN;
    int_unlock(lock);
}

void hal_cmu_i2s_clock_enable(enum HAL_I2S_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2S_SPDIF_CLK |= CMU_EN_CLK_OSC_I2S;
    int_unlock(lock);
}

void hal_cmu_i2s_clock_disable(enum HAL_I2S_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2S_SPDIF_CLK &= ~CMU_EN_CLK_OSC_I2S;
    int_unlock(lock);
}

int hal_cmu_i2s_set_div(enum HAL_I2S_ID_T id, uint32_t div)
{
    uint32_t lock;

    if (div < 2) {
        return 1;
    }

    div -= 2;
    if ((div & (CMU_CFG_DIV_I2S_MASK >> CMU_CFG_DIV_I2S_SHIFT)) != div) {
        return 1;
    }

    lock = int_lock();
    cmu->I2S_SPDIF_CLK = SET_BITFIELD(cmu->I2S_SPDIF_CLK, CMU_CFG_DIV_I2S, div);
    int_unlock(lock);
    return 0;
}

int hal_cmu_spdif_clock_enable(enum HAL_SPDIF_ID_T id)
{
    uint32_t lock;

    if (id >= HAL_SPDIF_ID_QTY) {
        return 1;
    }

    lock = int_lock();
    cmu->I2S_SPDIF_CLK |= CMU_EN_CLK_OSC_SPDIF;
    int_unlock(lock);
    return 0;
}

int hal_cmu_spdif_clock_disable(enum HAL_SPDIF_ID_T id)
{
    uint32_t lock;

    if (id >= HAL_SPDIF_ID_QTY) {
        return 1;
    }

    lock = int_lock();
    cmu->I2S_SPDIF_CLK &= ~CMU_EN_CLK_OSC_SPDIF;
    int_unlock(lock);

    return 0;
}

int hal_cmu_spdif_set_div(enum HAL_SPDIF_ID_T id, uint32_t div)
{
    uint32_t lock;

    if (id >= HAL_SPDIF_ID_QTY) {
        return 1;
    }

    if (div < 2) {
        return 2;
    }

    div -= 2;
    if ((div & (CMU_CFG_DIV_SPDIF_MASK >> CMU_CFG_DIV_SPDIF_SHIFT)) != div) {
        return 2;
    }

    lock = int_lock();
    cmu->I2S_SPDIF_CLK = SET_BITFIELD(cmu->I2S_SPDIF_CLK, CMU_CFG_DIV_SPDIF, div);
    int_unlock(lock);
    return 0;
}

#ifdef CHIP_HAS_USB
void hal_cmu_usb_set_device_mode(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->SYS_DIV |= CMU_USB_ID;
    int_unlock(lock);
}

void hal_cmu_usb_set_host_mode(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->SYS_DIV &= ~CMU_USB_ID;
    int_unlock(lock);
}

#ifdef ROM_BUILD
void hal_cmu_rom_select_usb_clock(enum HAL_CMU_USB_CLOCK_SEL_T sel)
{
}
#endif

void hal_cmu_usb_clock_enable(void)
{
    uint32_t lock;
    bool clk_24m_x2;

    if (hal_cmu_get_crystal_freq() == 24000000) {
        clk_24m_x2 = true;
    } else {
        clk_24m_x2 = false;
    }

    hal_cmu_clksrc_on(HAL_CMU_FREQ_208M);

    cmu->SYS_CLK_ENABLE = CMU_EN_USB_TOP_ENABLE;

    lock = int_lock();
    cmu->SYS_DIV &= ~CMU_SEL_USB_6M;
    if (clk_24m_x2) {
        // 24M->48M
        cmu->CMU_RES[1] |= CMU_USB_CLK_SEL_24M_X2;
    } else {
        // 156M->48M
        cmu->CMU_RES[1] &= ~CMU_USB_CLK_SEL_24M_X2;
    }
    int_unlock(lock);

    hal_cmu_clock_enable(HAL_CMU_MOD_H_USBC);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_USB32K);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_USB);

    hal_cmu_reset_set(HAL_CMU_MOD_O_USB);
    hal_cmu_reset_set(HAL_CMU_MOD_O_USB32K);
    hal_cmu_reset_set(HAL_CMU_MOD_H_USBC);
    hal_sys_timer_delay(US_TO_TICKS(60));
    hal_cmu_reset_clear(HAL_CMU_MOD_H_USBC);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_USB32K);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_USB);
}

void hal_cmu_usb_clock_disable(void)
{
    hal_cmu_reset_set(HAL_CMU_MOD_O_USB);
    hal_cmu_reset_set(HAL_CMU_MOD_O_USB32K);
    hal_cmu_reset_set(HAL_CMU_MOD_H_USBC);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_USB);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_USB32K);
    hal_cmu_clock_disable(HAL_CMU_MOD_H_USBC);

    cmu->SYS_CLK_DISABLE = CMU_EN_USB_TOP_DISABLE;
}

void BOOT_TEXT_FLASH_LOC hal_cmu_usb_init_div(void)
{
}
#endif /* CHIP_HAS_USB */

void BOOT_TEXT_FLASH_LOC hal_cmu_apb_init_div(void)
{
}

int hal_cmu_uart0_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq)
{
    uint32_t lock, clk;
    int ret = 0;

    lock = int_lock();

    clk = cmu->UART_CLK;
    if (freq == HAL_CMU_PERIPH_FREQ_26M) {
        clk &= ~(CMU_SEL_OSCX2_UART0 | CMU_SEL_78M_UART0);
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) {
        clk &= ~CMU_SEL_78M_UART0;
        clk |= CMU_SEL_OSCX2_UART0;
    } else {
        ret = 1;
    }
    cmu->UART_CLK = clk;

    int_unlock(lock);

    return ret;
}

int hal_cmu_uart1_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq)
{
    uint32_t lock, clk;
    int ret = 0;

    lock = int_lock();

    clk = cmu->UART_CLK;
    if (freq == HAL_CMU_PERIPH_FREQ_26M) {
        clk &= ~(CMU_SEL_OSCX2_UART1 | CMU_SEL_78M_UART1);
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) {
        clk &= ~CMU_SEL_78M_UART1;
        clk |= CMU_SEL_OSCX2_UART1;
    } else {
        ret = 1;
    }
    cmu->UART_CLK = clk;

    int_unlock(lock);

    return ret;
}

int hal_cmu_uart2_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq)
{
    uint32_t lock, clk;
    int ret = 0;

    lock = int_lock();

    clk = cmu->UART_CLK;
    if (freq == HAL_CMU_PERIPH_FREQ_26M) {
        clk &= ~(CMU_SEL_OSCX2_UART2 | CMU_SEL_78M_UART2);
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) {
        clk &= ~CMU_SEL_78M_UART2;
        clk |= CMU_SEL_OSCX2_UART2;
    } else {
        ret = 1;
    }
    cmu->UART_CLK = clk;

    int_unlock(lock);

    return ret;
}

int hal_cmu_i2c_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq)
{
    uint32_t lock, clk;
    int ret = 0;

    lock = int_lock();

    clk = cmu->I2C_CLK;
    if (freq == HAL_CMU_PERIPH_FREQ_26M) {
        clk &= ~(CMU_SEL_78M_I2C | CMU_SEL_OSCX2_I2C);
        clk |= CMU_SEL_OSC_I2C;
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) {
        clk &= ~(CMU_SEL_78M_I2C | CMU_SEL_OSC_I2C);
        clk |= CMU_SEL_OSCX2_I2C;
    } else {
        ret = 1;
    }
    cmu->I2C_CLK = clk;

    int_unlock(lock);

    return ret;
}

int hal_cmu_spi_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq)
{
    uint32_t lock, clk;
    int ret = 0;

    lock = int_lock();

    clk = cmu->SYS_DIV;
    if (freq == HAL_CMU_PERIPH_FREQ_26M) {
        clk &= ~(CMU_SEL_78M_SPI0 | CMU_SEL_OSCX2_SPI0);
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) {
        clk &= ~CMU_SEL_78M_SPI0;
        clk |= CMU_SEL_OSCX2_SPI0;
    } else {
        ret = 1;
    }
    cmu->SYS_DIV = clk;

    int_unlock(lock);

    return ret;
}

int hal_cmu_ispi_set_freq(enum HAL_CMU_PERIPH_FREQ_T freq)
{
    uint32_t lock;
    int ret = 0;

    lock = int_lock();
    if (freq == HAL_CMU_PERIPH_FREQ_26M) {
        cmu->SYS_DIV &= ~CMU_SEL_OSCX2_SPI1;
    } else if (freq == HAL_CMU_PERIPH_FREQ_52M) {
        cmu->SYS_DIV |= CMU_SEL_OSCX2_SPI1;
    } else {
        ret = 1;
    }
    int_unlock(lock);

    return ret;
}

int hal_cmu_clock_out_enable(enum HAL_CMU_CLOCK_OUT_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    cmu->CLK_OUT = SET_BITFIELD(cmu->CLK_OUT, CMU_CFG_CLK_OUT, id) | CMU_EN_CLK_OUT;
    int_unlock(lock);

    return 0;
}

void hal_cmu_clock_out_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->CLK_OUT &= ~CMU_EN_CLK_OUT;
    int_unlock(lock);
}

int hal_cmu_i2s_mclk_enable(enum HAL_CMU_I2S_MCLK_ID_T id)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2S_SPDIF_CLK = SET_BITFIELD(cmu->I2S_SPDIF_CLK, CMU_SEL_I2S_MCLK, id) | CMU_EN_I2S_MCLK;
    int_unlock(lock);

    return 0;
}

void hal_cmu_i2s_mclk_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2S_SPDIF_CLK &= ~CMU_EN_I2S_MCLK;
    int_unlock(lock);
}

int hal_cmu_pwm_set_freq(enum HAL_PWM_ID_T id, uint32_t freq)
{
    uint32_t lock;
    int clk_32k;
    uint32_t div;

    if (id >= HAL_PWM_ID_QTY) {
        return 1;
    }

    if (freq == HAL_CMU_PWM_SLOW_CLOCK) {
        clk_32k = 1;
        div = 0;
    } else {
        clk_32k = 0;
        div = hal_cmu_get_crystal_freq() / freq;
        if (div < 2) {
            return 1;
        }

        div -= 2;
        if ((div & (CMU_CFG_DIV_PWM0_MASK >> CMU_CFG_DIV_PWM0_SHIFT)) != div) {
            return 1;
        }
    }

    lock = int_lock();
    if (id == HAL_PWM_ID_0) {
        cmu->PWM01_CLK = (cmu->PWM01_CLK & ~(CMU_CFG_DIV_PWM0_MASK | CMU_SEL_OSC_PWM0 | CMU_EN_OSC_PWM0)) |
            CMU_CFG_DIV_PWM0(div) | (clk_32k ? 0 : (CMU_SEL_OSC_PWM0 | CMU_EN_OSC_PWM0));
    } else if (id == HAL_PWM_ID_1) {
        cmu->PWM01_CLK = (cmu->PWM01_CLK & ~(CMU_CFG_DIV_PWM1_MASK | CMU_SEL_OSC_PWM1 | CMU_EN_OSC_PWM1)) |
            CMU_CFG_DIV_PWM1(div) | (clk_32k ? 0 : (CMU_SEL_OSC_PWM1 | CMU_EN_OSC_PWM1));
    } else if (id == HAL_PWM_ID_2) {
        cmu->PWM23_CLK = (cmu->PWM23_CLK & ~(CMU_CFG_DIV_PWM2_MASK | CMU_SEL_OSC_PWM2 | CMU_EN_OSC_PWM2)) |
            CMU_CFG_DIV_PWM2(div) | (clk_32k ? 0 : (CMU_SEL_OSC_PWM2 | CMU_EN_OSC_PWM2));
    } else {
        cmu->PWM23_CLK = (cmu->PWM23_CLK & ~(CMU_CFG_DIV_PWM3_MASK | CMU_SEL_OSC_PWM3 | CMU_EN_OSC_PWM3)) |
            CMU_CFG_DIV_PWM3(div) | (clk_32k ? 0 : (CMU_SEL_OSC_PWM3 | CMU_EN_OSC_PWM3));
    }
    int_unlock(lock);
    return 0;
}

void hal_cmu_jtag_enable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->MCU_TIMER &= ~CMU_SECURE_BOOT;
    int_unlock(lock);
}

void hal_cmu_jtag_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->MCU_TIMER |= CMU_SECURE_BOOT;
    int_unlock(lock);
}

void hal_cmu_jtag_clock_enable(void)
{
    cmu->SYS_CLK_ENABLE = CMU_EN_CLK_TOP_JTAG_ENABLE;
}

void hal_cmu_jtag_clock_disable(void)
{
    cmu->SYS_CLK_DISABLE = CMU_EN_CLK_TOP_JTAG_DISABLE;
}

void hal_cmu_rom_clock_init(void)
{
    //enable OSC 26MHz clock
    cmu->SYS_CLK_ENABLE = CMU_EN_CLK_TOP_OSC_ENABLE;
}

void BOOT_TEXT_FLASH_LOC hal_cmu_osc_x2_enable(void)
{
#if defined(DIGX2_AS_52M_CLK)
    //OSCX2 clock comes from digital frequency doubler
    cmu->CLK_SELECT |= CMU_SEL_OSCX2_DIG;
    cmu->SYS_CLK_ENABLE = CMU_EN_X2_DIG_ENABLE;
#else
    //OSCX2 clock comes from analog
    cmu->CLK_SELECT &= ~CMU_SEL_OSCX2_DIG;
    cmu->SYS_CLK_DISABLE = CMU_EN_X2_DIG_DISABLE;
#endif
    cmu->SYS_CLK_ENABLE = CMU_PU_DOUBLER_ENABLE;
    hal_sys_timer_delay(US_TO_TICKS(5));
    cmu->SYS_CLK_ENABLE = CMU_EN_DOUBLER_ENABLE;
}

void BOOT_TEXT_FLASH_LOC hal_cmu_osc_x4_enable(void)
{
#if defined(DIGX4_AS_HIGHSPEED_CLK)
    cmu->CLK_SELECT |= CMU_SEL_OSCX4_DIG;
    cmu->SYS_CLK_ENABLE = CMU_EN_X4_DIG_ENABLE;
#else
    cmu->CLK_SELECT &= ~CMU_SEL_OSCX4_DIG;
    cmu->SYS_CLK_DISABLE = CMU_EN_X4_DIG_DISABLE;
#endif
    cmu->SYS_CLK_ENABLE = CMU_EN_CLK_TOP_OSCX4_ENABLE;
    // Enable OSCX6 for the whole digital domain
    cmu->SYS_CLK_ENABLE = CMU_EN_DB104M_ENABLE;
}

void BOOT_TEXT_FLASH_LOC hal_cmu_module_init_state(void)
{
    cmu->SYS_CLK_ENABLE = CMU_EN_CLK_TOP_OSC_ENABLE;
    // DMA channel config
    cmu->DMA_CFG0 =
        // codec
        CMU_DMA_CH0_REQ(0) | CMU_DMA_CH1_REQ(1) |
        // btpcm
        CMU_DMA_CH2_REQ(18) | CMU_DMA_CH3_REQ(19) |
        // uart0
        CMU_DMA_CH4_REQ(4) | CMU_DMA_CH5_REQ(5);
    cmu->DMA_CFG1 =
        // uart1
        CMU_DMA_CH6_REQ(6) | CMU_DMA_CH7_REQ(7) |
        // i2c
        CMU_DMA_CH8_REQ(8) | CMU_DMA_CH9_REQ(9) |
        // uart2
        CMU_DMA_CH10_REQ(10) | CMU_DMA_CH11_REQ(11);
    cmu->DMA_CFG2 =
#ifdef BTDUMP_ENABLE
        // btdump + spi tx
        CMU_DMA_CH12_REQ(20) | CMU_DMA_CH13_REQ(17) |
#else
        // spi
        CMU_DMA_CH12_REQ(16) | CMU_DMA_CH13_REQ(17) |
#endif
        // i2s
        CMU_DMA_CH14_REQ(14) | CMU_DMA_CH15_REQ(15);
    // Codec Fault Mask
    cmu->UART_CLK |= CMU_MASK_OBS_MASK;

#ifndef SIMU
    cmu->ORESET_SET = SYS_ORST_USB | SYS_ORST_I2C | SYS_ORST_SPI | SYS_ORST_UART0 | SYS_ORST_UART1 |
        SYS_ORST_I2S | SYS_ORST_UART2 | SYS_ORST_USB32K | SYS_ORST_PWM0 | SYS_ORST_PWM1 |
        SYS_ORST_PWM2 | SYS_ORST_PWM3 | SYS_ORST_CODEC | SYS_ORST_CODECIIR | SYS_ORST_CODECRS |
        SYS_ORST_CODECADC | SYS_ORST_CODECAD0 | SYS_ORST_CODECAD1 | SYS_ORST_CODECDA0 |
        SYS_ORST_CODECDA1;
    cmu->PRESET_SET = SYS_PRST_PWM | SYS_PRST_I2C | SYS_PRST_SPI | SYS_PRST_UART0 | SYS_PRST_UART1 |
        SYS_PRST_I2S | SYS_PRST_UART2;
    cmu->HRESET_SET = SYS_HRST_USBC | SYS_HRST_CODEC | SYS_HRST_USBH;

    cmu->OCLK_DISABLE = SYS_OCLK_USB | SYS_OCLK_I2C | SYS_OCLK_SPI | SYS_OCLK_UART0 | SYS_OCLK_UART1 |
        SYS_OCLK_I2S | SYS_OCLK_UART2 | SYS_OCLK_USB32K | SYS_OCLK_PWM0 | SYS_OCLK_PWM1 |
        SYS_OCLK_PWM2 | SYS_OCLK_PWM3 | SYS_OCLK_CODEC | SYS_OCLK_CODECIIR | SYS_OCLK_CODECRS |
        SYS_OCLK_CODECADC | SYS_OCLK_CODECAD0 | SYS_OCLK_CODECAD1 | SYS_OCLK_CODECDA0 |
        SYS_OCLK_CODECDA1;
    cmu->PCLK_DISABLE = SYS_PCLK_PWM | SYS_PCLK_I2C |SYS_PCLK_SPI |SYS_PCLK_UART0 | SYS_PCLK_UART1 |
        SYS_PCLK_I2S | SYS_PCLK_UART2;
    cmu->HCLK_DISABLE = SYS_HCLK_USBC | SYS_HCLK_CODEC | SYS_HCLK_USBH;

    cmu->ORESET_CLR = SYS_ORST_CODECIIR | SYS_ORST_WDT;
    cmu->OCLK_ENABLE = SYS_OCLK_CODECIIR | SYS_OCLK_WDT;
#endif
}

void BOOT_TEXT_FLASH_LOC hal_cmu_ema_init(void)
{
}

void hal_cmu_lpu_wait_26m_ready(void)
{
    while ((cmu->WAKEUP_CLK_CFG & CMU_LPU_STATUS_26M) == 0);
}

int hal_cmu_lpu_busy(void)
{
    if ((cmu->WAKEUP_CLK_CFG & CMU_LPU_AUTO_SWITCH26) &&
        (cmu->WAKEUP_CLK_CFG & CMU_LPU_STATUS_26M) == 0) {
        return 1;
    }
    if ((cmu->WAKEUP_CLK_CFG & CMU_LPU_AUTO_SWITCHPLL) &&
        (cmu->WAKEUP_CLK_CFG & CMU_LPU_STATUS_PLL) == 0) {
        return 1;
    }
    return 0;
}

int BOOT_TEXT_FLASH_LOC hal_cmu_lpu_init(enum HAL_CMU_LPU_CLK_CFG_T cfg, uint32_t timer_26m, uint32_t timer_pll)
{
    uint32_t lpu_clk;

    if (cfg >= HAL_CMU_LPU_CLK_QTY) {
        return 1;
    }
    if ((timer_26m & (CMU_TIMER_WT26_MASK >> CMU_TIMER_WT26_SHIFT)) != timer_26m) {
        return 2;
    }
    if ((timer_pll & (CMU_TIMER_WTPLL_MASK >> CMU_TIMER_WTPLL_SHIFT)) != timer_pll) {
        return 3;
    }
    if (hal_cmu_lpu_busy()) {
        return -1;
    }

    if (cfg == HAL_CMU_LPU_CLK_26M) {
        lpu_clk = CMU_LPU_AUTO_SWITCH26;
    } else if (cfg == HAL_CMU_LPU_CLK_PLL) {
        lpu_clk = CMU_LPU_AUTO_SWITCHPLL | CMU_LPU_AUTO_SWITCH26;
    } else {
        lpu_clk = 0;
    }

    if (lpu_clk & CMU_LPU_AUTO_SWITCH26) {
        // Disable RAM wakeup early
        cmu->MCU_TIMER &= ~CMU_RAM_RETN_UP_EARLY;
        // MCU/ROM/RAM auto clock gating (which depends on RAM gating signal)
        cmu->HCLK_MODE &= ~(SYS_HCLK_MCU | SYS_HCLK_ROM | SYS_HCLK_RAM0 | SYS_HCLK_RAM1);
    }

    cmu->WAKEUP_CLK_CFG = CMU_TIMER_WT26(timer_26m) | CMU_TIMER_WTPLL(0) | lpu_clk;
    if (timer_pll) {
        hal_sys_timer_delay(US_TO_TICKS(60));
        cmu->WAKEUP_CLK_CFG = CMU_TIMER_WT26(timer_26m) | CMU_TIMER_WTPLL(timer_pll) | lpu_clk;
    }
    return 0;
}

int SRAM_TEXT_LOC hal_cmu_lpu_sleep(void)
{
    uint32_t start;
    uint32_t timeout;
    uint32_t saved_clk_cfg;
    uint32_t wakeup_cfg;

    saved_clk_cfg = cmu->SYS_CLK_ENABLE;
    wakeup_cfg = cmu->WAKEUP_CLK_CFG;

    // Setup wakeup mask
    cmu->WAKEUP_MASK0 = NVIC->ISER[0];
    cmu->WAKEUP_MASK1 = NVIC->ISER[1];

    if (wakeup_cfg & CMU_LPU_AUTO_SWITCHPLL) {
        // Do nothing
        // Hardware will switch system/memory/flash freq to 32K and shutdown PLLs automatically
    } else {
        hal_cmu_clock_disable(HAL_CMU_MOD_O_FLASH);

        if (wakeup_cfg & CMU_LPU_AUTO_SWITCH26) {
#if 0
            // switch flash freq to 26M
            cmu->SYS_CLK_ENABLE = CMU_SEL_FLS_L_OSC_ENABLE;
            cmu->SYS_CLK_DISABLE = CMU_SEL_FLS_HIGH_DISABLE
                                 | CMU_SEL_FLS_MID_DISABLE;
#endif
            // switch flash freq to 32K
            cmu->SYS_CLK_DISABLE = CMU_SEL_FLS_HIGH_DISABLE
                                 | CMU_SEL_FLS_MID_DISABLE
                                 | CMU_SEL_FLS_L_OSC_DISABLE;
        } else {
            // switch flash freq to 32K
            cmu->SYS_CLK_DISABLE = CMU_SEL_FLS_HIGH_DISABLE
                                 | CMU_SEL_FLS_MID_DISABLE
                                 | CMU_SEL_FLS_L_OSC_DISABLE;
        }

        // Switch system freq to 26M
        cmu->SYS_CLK_ENABLE  = CMU_SEL_SYS_L_OSC_ENABLE;
        cmu->SYS_CLK_DISABLE = CMU_SEL_SYS_HIGH_DISABLE
                             | CMU_SEL_SYS_MID_DISABLE;

        cmu->SYS_CLK_DISABLE = CMU_EN_CLK_TOP_OSC_DISABLE;

        // Shutdown 156M clk
        if (saved_clk_cfg & CMU_PU_DB104M_ENABLE) {

            if (saved_clk_cfg & CMU_EN_DB104M_ENABLE)
                cmu->SYS_CLK_DISABLE = CMU_EN_DB104M_DISABLE;

            if (saved_clk_cfg & CMU_EN_USB_TOP_ENABLE)
                cmu->SYS_CLK_DISABLE = CMU_EN_USB_TOP_DISABLE;

            cmu->SYS_CLK_DISABLE = CMU_PU_DB104M_ENABLE;
        }
        // Shutdown 52M clk
        if (saved_clk_cfg & CMU_PU_DOUBLER_ENABLE) {

            if (saved_clk_cfg & CMU_EN_DOUBLER_ENABLE)
                cmu->SYS_CLK_DISABLE = CMU_EN_DOUBLER_DISABLE;

            cmu->SYS_CLK_DISABLE = CMU_PU_DOUBLER_DISABLE;
        }

        if (wakeup_cfg & CMU_LPU_AUTO_SWITCH26) {
            // Do nothing
            // Hardware will switch system/memory/flash freq to 32K automatically
        } else {
            // switch system freq to 32K
            cmu->SYS_CLK_DISABLE  = CMU_SEL_SYS_L_OSC_DISABLE;
        }
    }

    if (wakeup_cfg & CMU_LPU_AUTO_SWITCH26) {
        // Enable auto memory retention
        cmu->SLEEP = (cmu->SLEEP & ~CMU_MANUAL_RAM_RETN) |
            CMU_DEEPSLEEP_EN | CMU_DEEPSLEEP_ROMRAM_EN | CMU_DEEPSLEEP_START;
    } else {
        // Disable auto memory retention
        cmu->SLEEP = (cmu->SLEEP & ~CMU_DEEPSLEEP_ROMRAM_EN) |
            CMU_DEEPSLEEP_EN | CMU_MANUAL_RAM_RETN | CMU_DEEPSLEEP_START;
    }

    SCB->SCR = SCB_SCR_SLEEPDEEP_Msk;
    __WFI();

    if (wakeup_cfg & CMU_LPU_AUTO_SWITCHPLL) {
        // Do nothing
    } else {
        // Wait for 26M ready
        if (wakeup_cfg & CMU_LPU_AUTO_SWITCH26) {
            start = hal_sys_timer_get();
            timeout = HAL_CMU_26M_READY_TIMEOUT;
            do {
                if(cmu->WAKEUP_CLK_CFG & CMU_LPU_STATUS_26M) {
                    cmu->SYS_CLK_ENABLE = CMU_EN_CLK_TOP_OSC_ENABLE;
                    break;
                }
            } while((hal_sys_timer_get() - start) < timeout);
        } else {
            hal_sys_timer_delay(MS_TO_TICKS(2));
            // Switch system freq to 26M
            cmu->SYS_CLK_ENABLE  = CMU_SEL_SYS_L_OSC_ENABLE;
        }
        // Restore 156M clk
        if (saved_clk_cfg & CMU_PU_DB104M_ENABLE) {
            cmu->SYS_CLK_ENABLE = CMU_PU_DB104M_ENABLE;

            hal_sys_timer_delay(US_TO_TICKS(5));

            if (saved_clk_cfg & CMU_EN_DB104M_ENABLE)
                cmu->SYS_CLK_ENABLE = CMU_EN_DB104M_ENABLE;

            if (saved_clk_cfg & CMU_EN_USB_TOP_ENABLE)
                cmu->SYS_CLK_ENABLE = CMU_EN_USB_TOP_ENABLE;
        }
        // Restore 52M clk
        if (saved_clk_cfg & CMU_PU_DOUBLER_ENABLE) {
            cmu->SYS_CLK_ENABLE = CMU_PU_DOUBLER_ENABLE;

            hal_sys_timer_delay(US_TO_TICKS(5));

            if (saved_clk_cfg & CMU_EN_DOUBLER_ENABLE)
                cmu->SYS_CLK_ENABLE = CMU_EN_DOUBLER_ENABLE;
        }
    }

    // Restore system/flash freq
    cmu->SYS_CLK_ENABLE = saved_clk_cfg;

    // The original system/flash freq are at least 26M
    //cmu->SYS_CLK_DISABLE = ~saved_clk_cfg;

    // Wait at least 100us until the voltages become stable
    hal_sys_timer_delay(US_TO_TICKS(100));

    hal_cmu_clock_enable(HAL_CMU_MOD_O_FLASH);
    return 0;
}

void hal_cmu_bt_clock_enable(void)
{
    hal_cmu_clock_enable(HAL_CMU_MOD_O_BT_ALL);
}

void hal_cmu_bt_clock_disable(void)
{
    hal_cmu_clock_disable(HAL_CMU_MOD_O_BT_ALL);    
}

void hal_cmu_bt_reset_set(void)
{
    hal_cmu_reset_set(HAL_CMU_MOD_O_BT_ALL);
    hal_cmu_reset_set(HAL_CMU_MOD_O_BT_CPU);
}

void hal_cmu_bt_reset_clear(void)
{
    hal_cmu_reset_clear(HAL_CMU_MOD_O_BT_ALL);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_BT_CPU);
}

void hal_cmu_bt_module_init(void)
{
    //btcmu->CLK_MODE = 0;
}

volatile uint32_t *hal_cmu_get_bootmode_addr(void)
{
    return &cmu->BOOTMODE;
}

volatile uint32_t *hal_cmu_get_memsc_addr(void)
{
    return &cmu->MEMSC[0];
}

void BOOT_TEXT_FLASH_LOC hal_cmu_low_freq_mode_init(void)
{
}

void hal_cmu_low_freq_mode_enable(enum HAL_CMU_FREQ_T old_freq, enum HAL_CMU_FREQ_T new_freq)
{
}

void hal_cmu_low_freq_mode_disable(enum HAL_CMU_FREQ_T old_freq, enum HAL_CMU_FREQ_T new_freq)
{
}

void hal_cmu_pcm_clock_out_enable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2C_CLK |= CMU_EN_CLK_PCM_OUT;
    int_unlock(lock);
}

void hal_cmu_pcm_clock_out_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2C_CLK &= ~CMU_EN_CLK_PCM_OUT;
    int_unlock(lock);
}

void hal_cmu_pcm_set_slave_mode(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2C_CLK |= CMU_SEL_PCM_CLKIN | CMU_POL_CLK_PCM_IN;
    int_unlock(lock);
}

void hal_cmu_pcm_set_master_mode(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->I2C_CLK &= ~CMU_SEL_PCM_CLKIN;
    int_unlock(lock);
}

void hal_cmu_pcm_clock_enable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->SYS_DIV |= CMU_EN_CLK_OSC_PCM;
    int_unlock(lock);
}

void hal_cmu_pcm_clock_disable(void)
{
    uint32_t lock;

    lock = int_lock();
    cmu->SYS_DIV &= ~CMU_EN_CLK_OSC_PCM;
    int_unlock(lock);
}

int hal_cmu_pcm_set_div(uint32_t div)
{
    uint32_t lock;

    lock = int_lock();
    cmu->SYS_DIV = SET_BITFIELD(cmu->SYS_DIV, CMU_CFG_DIV_PCM, div);
    int_unlock(lock);
    return 0;
}

void hal_cmu_audio_resample_enable(void)
{
}

void hal_cmu_audio_resample_disable(void)
{
}

int hal_cmu_get_audio_resample_status(void)
{
    return true;
}

void hal_cmu_set_digx4_clk_phase_1p0(void)
{
    cmu->CLK_SELECT = (cmu->CLK_SELECT & (~CMU_SEL_X4_PHASE_MASK)) | (CMU_SEL_X4_PHASE(7));
}

void hal_cmu_set_digx4_clk_phase_0p9(void)
{
    cmu->CLK_SELECT = (cmu->CLK_SELECT & (~CMU_SEL_X4_PHASE_MASK)) | (CMU_SEL_X4_PHASE(1));
}
