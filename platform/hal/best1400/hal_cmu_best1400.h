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
#ifndef __HAL_CMU_BEST1400_H__
#define __HAL_CMU_BEST1400_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_CMU_VALID_CRYSTAL_FREQ          { 26000000, 24000000, }

enum HAL_CMU_MOD_ID_T {
    // HCLK/HRST
    HAL_CMU_MOD_H_MCU = 0,      // 0
    HAL_CMU_MOD_H_ROM,          // 1
    HAL_CMU_MOD_H_RAM0,         // 2
    HAL_CMU_MOD_H_RAM1,         // 3
    HAL_CMU_MOD_H_RAM2,         // 4
    HAL_CMU_MOD_H_RAM3,         // 5
    HAL_CMU_MOD_H_AHB0,         // 6
    HAL_CMU_MOD_H_AHB1,         // 7
    HAL_CMU_MOD_H_GDMA,         // 8
    HAL_CMU_MOD_H_EXTMEM,       // 9
    HAL_CMU_MOD_H_FLASH,        // 10
    HAL_CMU_MOD_H_USBC,         // 11
    HAL_CMU_MOD_H_CODEC,        // 12
    HAL_CMU_MOD_H_I2C_SLAVE,    // 13
    HAL_CMU_MOD_H_USBH,         // 14
    HAL_CMU_MOD_H_AH2H_BT,      // 15
    HAL_CMU_MOD_H_BT_DUMP,      // 16
    // PCLK/PRST
    HAL_CMU_MOD_P_CMU,          // 17
    HAL_CMU_MOD_P_GPIO,         // 18
    HAL_CMU_MOD_P_GPIO_INT,     // 19
    HAL_CMU_MOD_P_WDT,          // 20
    HAL_CMU_MOD_P_PWM,          // 21
    HAL_CMU_MOD_P_TIMER0,       // 22
    HAL_CMU_MOD_P_TIMER1,       // 23
    HAL_CMU_MOD_P_I2C0,         // 24
    HAL_CMU_MOD_P_SPI,          // 25
    HAL_CMU_MOD_P_SPI_ITN,      // 26
    HAL_CMU_MOD_P_UART0,        // 27
    HAL_CMU_MOD_P_UART1,        // 28
    HAL_CMU_MOD_P_I2S0,         // 29
    HAL_CMU_MOD_P_UART2,        // 30
    HAL_CMU_MOD_P_IOMUX,        // 31
    HAL_CMU_MOD_P_PCM,          // 32
    HAL_CMU_MOD_P_GLOBAL,       // 33
    // OCLK/ORST
    HAL_CMU_MOD_O_SLEEP,        // 34
    HAL_CMU_MOD_O_FLASH,        // 35
    HAL_CMU_MOD_O_USB,          // 36
    HAL_CMU_MOD_O_GPIO,         // 37
    HAL_CMU_MOD_O_WDT,          // 38
    HAL_CMU_MOD_O_TIMER0,       // 39
    HAL_CMU_MOD_O_TIMER1,       // 40
    HAL_CMU_MOD_O_I2C0,         // 41
    HAL_CMU_MOD_O_SPI,          // 42
    HAL_CMU_MOD_O_SPI_ITN,      // 43
    HAL_CMU_MOD_O_UART0,        // 44
    HAL_CMU_MOD_O_UART1,        // 45
    HAL_CMU_MOD_O_I2S0,         // 46
    HAL_CMU_MOD_O_UART2,        // 47
    HAL_CMU_MOD_O_USB32K,       // 48
    HAL_CMU_MOD_O_IOMUX,        // 49
    HAL_CMU_MOD_O_PWM0,         // 50
    HAL_CMU_MOD_O_PWM1,         // 51
    HAL_CMU_MOD_O_PWM2,         // 52
    HAL_CMU_MOD_O_PWM3,         // 53
    HAL_CMU_MOD_O_CODEC,        // 54
    HAL_CMU_MOD_O_CODECIIR,     // 55
    HAL_CMU_MOD_O_CODECRS,      // 56
    HAL_CMU_MOD_O_CODECADC,     // 57
    HAL_CMU_MOD_O_CODECAD0,     // 58
    HAL_CMU_MOD_O_CODECAD1,     // 59
    HAL_CMU_MOD_O_CODECAD2,     // 60
    HAL_CMU_MOD_O_CODECDA0,     // 61
    HAL_CMU_MOD_O_CODECDA1,     // 62
    HAL_CMU_MOD_O_PCM,          // 63
    HAL_CMU_MOD_O_BT_ALL,       // 64
    HAL_CMU_MOD_O_BT_CPU,       // 65

    HAL_CMU_MOD_QTY,

    HAL_CMU_MOD_GLOBAL = HAL_CMU_MOD_P_GLOBAL,
};

enum HAL_CMU_CLOCK_OUT_ID_T {
    HAL_CMU_CLOCK_OUT_32K = 0,
    HAL_CMU_CLOCK_OUT_OSC,
    HAL_CMU_CLOCK_OUT_OSCX2,
    HAL_CMU_CLOCK_OUT_OSCX4,
    HAL_CMU_CLOCK_OUT_SYS,
    HAL_CMU_CLOCK_OUT_PCLK,
    HAL_CMU_CLOCK_OUT_FLS,
    HAL_CMU_CLOCK_OUT_USB,
    HAL_CMU_CLOCK_OUT_I2S,
    HAL_CMU_CLOCK_OUT_RESV,
    HAL_CMU_CLOCK_OUT_SPI0,
    HAL_CMU_CLOCK_OUT_SPI1,
    HAL_CMU_CLOCK_OUT_UART0,
    HAL_CMU_CLOCK_OUT_I2C,
    HAL_CMU_CLOCK_OUT_PCM,
    HAL_CMU_CLOCK_OUT_CODEC_IIR,
    HAL_CMU_CLOCK_OUT_CODEC_RS,
    HAL_CMU_CLOCK_OUT_CODEC_ADC,
    HAL_CMU_CLOCK_OUT_CODEC_PWM0,
    HAL_CMU_CLOCK_OUT_I2S_MCLK,
    HAL_CMU_CLOCK_OUT_DCDC,
    HAL_CMU_CLOCK_OUT_78M,
    HAL_CMU_CLOCK_OUT_13M,

    HAL_CMU_CLOCK_OUT_QTY,
};

enum HAL_CMU_I2S_MCLK_ID_T {

    HAL_CMU_I2S_MCLK_QTY,
};

void hal_cmu_set_digx4_clk_phase_1p0(void);

void hal_cmu_set_digx4_clk_phase_0p9(void);

#ifdef __cplusplus
}
#endif

#endif

