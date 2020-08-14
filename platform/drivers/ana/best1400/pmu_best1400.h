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
#ifndef __PMU_BEST1400_H__
#define __PMU_BEST1400_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ISPI_PMU_REG(reg)                   (((reg) & 0xFFF) | 0x0000)
#define ISPI_ANA_REG(reg)                   (((reg) & 0xFFF) | 0x1000)
#define ISPI_RF_REG(reg)                    (((reg) & 0xFFF) | 0x2000)

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

enum PMU_IRQ_TYPE_T {
    PMU_IRQ_TYPE_GPADC,
    PMU_IRQ_TYPE_RTC,
    PMU_IRQ_TYPE_CHARGER,
    PMU_IRQ_TYPE_USB_PIN,
    PMU_IRQ_TYPE_GPIO,
    PMU_IRQ_TYPE_WDT,

    PMU_IRQ_TYPE_QTY
};

typedef void (*PMU_IRQ_HANDLER_T)(void);

int pmu_set_irq_handler(enum PMU_IRQ_TYPE_T type, PMU_IRQ_HANDLER_T hdlr);

uint8_t pmu_gpio_setup_irq(enum HAL_GPIO_PIN_T pin, const struct HAL_GPIO_IRQ_CFG_T *cfg);

void pmu_codec_hppa_enable(int enable);

void pmu_codec_mic_bias_enable(uint32_t map);

void pmu_led_uart_enable(enum HAL_IOMUX_PIN_T pin);

void pmu_led_uart_disable(enum HAL_IOMUX_PIN_T pin);

#ifdef __cplusplus
}
#endif

#endif

