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
#include "cmsis_nvic.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "pmu.h"
#include "analog.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_chipid.h"
#include "hal_location.h"
#include "hal_cmu.h"
#include "hal_aud.h"
#include "tgt_hardware.h"
#include "usbphy.h"

// LDO soft start interval is about 1000 us
#define PMU_LDO_PU_STABLE_TIME_US       1800
#define PMU_DCDC_PU_STABLE_TIME_US      100
#define PMU_VANA_STABLE_TIME_US         10
#define PMU_VCORE_STABLE_TIME_US        10

#ifdef __PMU_VIO_DYNAMIC_CTRL_MODE__
#define IO_VOLT_ACTIVE_NORMAL           PMU_IO_2_2V
#else
#define IO_VOLT_ACTIVE_NORMAL           PMU_IO_2_6V
#endif
#ifdef DIGMIC_HIGH_VOLT
#define IO_VOLT_ACTIVE_RISE             PMU_IO_3_0V
#else
#define IO_VOLT_ACTIVE_RISE             PMU_IO_2_6V
#endif
#define IO_VOLT_SLEEP                   PMU_IO_2_7V

#define PMU_DCDC_ANA_2_3V               0xF
#define PMU_DCDC_ANA_2_2V               0xE
#define PMU_DCDC_ANA_2_1V               0xD
#define PMU_DCDC_ANA_2_0V               0xC
#define PMU_DCDC_ANA_1_9V               0xB
#define PMU_DCDC_ANA_1_8V               0xA
#define PMU_DCDC_ANA_1_7V               0x9
#define PMU_DCDC_ANA_1_6V               0x8
#define PMU_DCDC_ANA_1_5V               0x7
#define PMU_DCDC_ANA_1_4V               0x6
#define PMU_DCDC_ANA_1_3V               0x5

#define PMU_DCDC_ANA_SLEEP_1_6V         0xF
#define PMU_DCDC_ANA_SLEEP_1_5V         0xE
#define PMU_DCDC_ANA_SLEEP_1_4V         0xD
#define PMU_DCDC_ANA_SLEEP_1_3V         0xC
#define PMU_DCDC_ANA_SLEEP_1_2V         0xA
#define PMU_DCDC_ANA_SLEEP_1_1V         0x8

#define PMU_DCDC_ANA2_FLAG              (1 << 7)

enum PMU_DCDC_ANA2_VOLT_T {
    PMU_DCDC_ANA2_2_4V = 0x8 | PMU_DCDC_ANA2_FLAG,
    PMU_DCDC_ANA2_2_5V,
    PMU_DCDC_ANA2_2_6V,
    PMU_DCDC_ANA2_2_7V,
    PMU_DCDC_ANA2_2_9V,
    PMU_DCDC_ANA2_3_1V,
    PMU_DCDC_ANA2_3_3V,
    PMU_DCDC_ANA2_3_5V,
};

enum PMU_DCDC_ANA2_SLEEP_VOLT_T {
    PMU_DCDC_ANA2_SLEEP_1_5V = 0x8 | PMU_DCDC_ANA2_FLAG,
    PMU_DCDC_ANA2_SLEEP_1_xV,
    PMU_DCDC_ANA2_SLEEP_1_7V,
    PMU_DCDC_ANA2_SLEEP_1_8V,
    PMU_DCDC_ANA2_SLEEP_2_0V,
    PMU_DCDC_ANA2_SLEEP_2_1V,
    PMU_DCDC_ANA2_SLEEP_2_2V,
    PMU_DCDC_ANA2_SLEEP_2_5V,
};

#define PMU_DCDC_DIG_1_4V               0x6
#define PMU_DCDC_DIG_1_3V               0x5
#define PMU_DCDC_DIG_1_2V               0x4
#define PMU_DCDC_DIG_1_1V               0x3
#define PMU_DCDC_DIG_1_0V               0x2
#define PMU_DCDC_DIG_0_9V               0x1
#define PMU_DCDC_DIG_0_8V               0x0

#ifdef PMU_DCDC_DIG2_SUPPORT
#define PMU_DCDC_DIG2_FLAG              (1 << 7)

#define PMU_DCDC_DIG2_1_25V             (0x8 | PMU_DCDC_DIG2_FLAG)
#define PMU_DCDC_DIG2_1_15V             (0x6 | PMU_DCDC_DIG2_FLAG)
#define PMU_DCDC_DIG2_1_05V             (0x5 | PMU_DCDC_DIG2_FLAG)
#endif

#define PMU_VDIG_1_35V                  0xF
#define PMU_VDIG_1_3V                   0xE
#define PMU_VDIG_1_25V                  0xD
#define PMU_VDIG_1_2V                   0xC
#define PMU_VDIG_1_15V                  0xB
#define PMU_VDIG_1_1V                   0xA
#define PMU_VDIG_1_05V                  0x9
#define PMU_VDIG_1_0V                   0x8
#define PMU_VDIG_0_95V                  0x7
#define PMU_VDIG_0_9V                   0x6
#define PMU_VDIG_0_85V                  0x5
#define PMU_VDIG_0_8V                   0x4
#define PMU_VDIG_0_75V                  0x3
#define PMU_VDIG_0_7V                   0x2
#define PMU_VDIG_0_65V                  0x1
#define PMU_VDIG_0_6V                   0x0

#define PMU_IO_3_3V                     0x15
#define PMU_IO_3_2V                     0x14
#define PMU_IO_3_1V                     0x13
#define PMU_IO_3_0V                     0x12
#define PMU_IO_2_9V                     0x11
#define PMU_IO_2_8V                     0x10
#define PMU_IO_2_7V                     0xF
#define PMU_IO_2_6V                     0xE
#define PMU_IO_2_5V                     0xD
#define PMU_IO_2_4V                     0xC
#define PMU_IO_2_3V                     0xB
#define PMU_IO_2_2V                     0xA
#define PMU_IO_2_1V                     0x9
#define PMU_IO_2_0V                     0x8
#define PMU_IO_1_9V                     0x7
#define PMU_IO_1_8V                     0x6
#define PMU_IO_1_7V                     0x5

#define PMU_CODEC_3_7V                  0x1F
#define PMU_CODEC_3_6V                  0x1E
#define PMU_CODEC_3_5V                  0x1D
#define PMU_CODEC_3_4V                  0x1C
#define PMU_CODEC_3_3V                  0x1B
#define PMU_CODEC_3_2V                  0x1A
#define PMU_CODEC_3_1V                  0x19
#define PMU_CODEC_3_0V                  0x18
#define PMU_CODEC_2_9V                  0x17
#define PMU_CODEC_2_8V                  0x16
#define PMU_CODEC_2_7V                  0x0F
#define PMU_CODEC_2_6V                  0x0E
#define PMU_CODEC_2_5V                  0x0D
#define PMU_CODEC_2_4V                  0x0C
#define PMU_CODEC_2_3V                  0x0B
#define PMU_CODEC_2_2V                  0x0A
#define PMU_CODEC_2_1V                  0x09
#define PMU_CODEC_2_0V                  0x08
#define PMU_CODEC_1_9V                  0x07
#define PMU_CODEC_1_8V                  0x06
#define PMU_CODEC_1_7V                  0x05
#define PMU_CODEC_1_6V                  0x04
#define PMU_CODEC_1_5V                  0x03
#define PMU_CODEC_1_4V                  0x02
#define PMU_CODEC_1_3V                  0x01
#define PMU_CODEC_1_2V                  0x00

#define PMU_USB_3_3V                    0xC
#define PMU_USB_3_2V                    0xB
#define PMU_USB_3_1V                    0xA
#define PMU_USB_3_0V                    0x9
#define PMU_USB_2_9V                    0x8
#define PMU_USB_2_8V                    0x7
#define PMU_USB_2_7V                    0x6
#define PMU_USB_2_6V                    0x5
#define PMU_USB_2_5V                    0x4
#define PMU_USB_2_4V                    0x3

// 00   PMU_REG_METAL_ID
#define PMU_METAL_ID_SHIFT              0
#define PMU_METAL_ID_MASK               (0xF << PMU_METAL_ID_SHIFT)
#define PMU_METAL_ID(n)                 (((n) & 0xF) << PMU_METAL_ID_SHIFT)

// 02   PMU_REG_POWER_KEY_CFG
#define PMU_PU_LPO_DR                   (1 << 14)
#define PMU_PU_LPO_REG                  (1 << 13)
#define PMU_POWERKEY_WAKEUP_OSC_EN      (1 << 12)
#define PMU_RTC_POWER_ON_EN             (1 << 11)

// 05   PMU_REG_CHARGER_CFG
#define PMU_CHARGE_OUT_INTR_MSK         (1 << 12)
#define PMU_CHARGE_IN_INTR_MSK          (1 << 11)
#define PMU_AC_ON_OUT_EN                (1 << 10)
#define PMU_AC_ON_IN_EN                 (1 << 9)
#define PMU_CHARGE_INTR_EN              (1 << 8)
#define PMU_AC_ON_DB_VALUE_SHIFT        0
#define PMU_AC_ON_DB_VALUE_MASK         (0xFF << PMU_AC_ON_DB_VALUE_SHIFT)
#define PMU_AC_ON_DB_VALUE(n)           BITFIELD_VAL(PMU_AC_ON_DB_VALUE, n)

// 07   ana setting
#define PMU_ANA_LP_MODE_EN              (1<<13)
#define PMU_ANA_LP_EN_DR                (1<<12)
#define PMU_ANA_LP_EN_REG               (1<<11)
#define PMU_ANA_DSLEEP_ON               (1<<10)
#define PMU_ANA_VANA_DR                 (1<<9)
#define PMU_ANA_VANA_EN                 (1<<8)
#define PMU_ANA_DSLEEP_VOLT(n)          (((n) & 0xF) << 4)
#define PMU_ANA_DSLEEP_VOLT_MASK        (0xF << 4)
#define PMU_ANA_DSLEEP_VOLT_SHIFT       (4)
#define PMU_ANA_NORMAL_VOLT(n)          (((n) & 0xF) << 0)
#define PMU_ANA_NORMAL_VOLT_MASK        (0xF << 0)
#define PMU_ANA_NORMAL_VOLT_SHIFT       (0)

// 08   dig setting
#define PMU_DIG_LP_MODE_EN              (1<<13)
#define PMU_DIG_LP_EN_DR                (1<<12)
#define PMU_DIG_LP_EN_REG               (1<<11)
#define PMU_DIG_DSLEEP_ON               (1<<10)
#define PMU_DIG_VDIG_DR                 (1<<9)
#define PMU_DIG_VDIG_EN                 (1<<8)
#define PMU_DIG_DSLEEP_VOLT(n)          (((n) & 0xF) << 4)
#define PMU_DIG_DSLEEP_VOLT_MASK        (0xF << 4)
#define PMU_DIG_DSLEEP_VOLT_SHIFT       (4)
#define PMU_DIG_NORMAL_VOLT(n)          (((n) & 0xF) << 0)
#define PMU_DIG_NORMAL_VOLT_MASK        (0xF << 0)
#define PMU_DIG_NORMAL_VOLT_SHIFT       (0)

// 09   IO SETTING
#define PMU_IO_LP_MODE_EN               (1<<15)
#define PMU_IO_LP_EN_DR                 (1<<14)
#define PMU_IO_LP_EN_REG                (1<<13)
#define PMU_IO_DSLEEP_ON                (1<<12)
#define PMU_IO_VIO_DR                   (1<<11)
#define PMU_IO_VIO_EN                   (1<<10)
#define PMU_IO_NORMAL_VOLT(n)           (((n) & 0x1F) << 5)
#define PMU_IO_NORMAL_VOLT_MASK         (0x1F << 5)
#define PMU_IO_NORMAL_VOLT_SHIFT        (5)
#define PMU_IO_DSLEEP_VOLT(n)           (((n) & 0x1F) << 0)
#define PMU_IO_DSLEEP_VOLT_MASK         (0x1F << 0)
#define PMU_IO_DSLEEP_VOLT_SHIFT        (0)

// 0A   GP SETTING
#define PMU_GP_LP_MODE_EN               (1<<15)
#define PMU_GP_LP_EN_DR                 (1<<14)
#define PMU_GP_LP_EN_REG                (1<<13)
#define PMU_GP_DSLEEP_ON                (1<<12)
#define PMU_GP_VGP_DR                   (1<<11)
#define PMU_GP_VGP_EN                   (1<<10)
#define PMU_GP_NORMAL_VOLT(n)           (((n) & 0x1F) << 5)
#define PMU_GP_NORMAL_VOLT_MASK         (0x1F << 5)
#define PMU_GP_NORMAL_VOLT_SHIFT        (5)
#define PMU_GP_DSLEEP_VOLT(n)           (((n) & 0x1F) << 0)
#define PMU_GP_DSLEEP_VOLT_MASK         (0x1F << 0)
#define PMU_GP_DSLEEP_VOLT_SHIFT        (0)

// 0B   USB SETTING
#define PMU_USB_LP_MODE_EN              (1<<13)
#define PMU_USB_LP_EN_DR                (1<<12)
#define PMU_USB_LP_EN_REG               (1<<11)
#define PMU_USB_DSLEEP_ON               (1<<10)
#define PMU_USB_VUSB_DR                 (1<<9)
#define PMU_USB_VUSB_EN                 (1<<8)
#define PMU_USB_NORMAL_VOLT(n)          (((n) & 0xF) << 4)
#define PMU_USB_NORMAL_VOLT_MASK        (0xF << 4)
#define PMU_USB_NORMAL_VOLT_SHIFT       (4)
#define PMU_USB_DSLEEP_VOLT(n)          (((n) & 0xF) << 0)
#define PMU_USB_DSLEEP_VOLT_MASK        (0xF << 0)
#define PMU_USB_DSLEEP_VOLT_SHIFT       (0)

// 0C   CRYSTAL SETTING
#define PMU_CRYSTAL_DSLEEP_ON           (1<<13)
#define PMU_CRYSTAL_VCRYSTAL_DR         (1<<12)
#define PMU_CRYSTAL_VCRYSTAL_EN         (1<<11)
#define PMU_CRYSTAL_LP_MODE_EN          (1<<10)
#define PMU_CRYSTAL_LP_EN_DR            (1<<9)
#define PMU_CRYSTAL_LP_EN_REG           (1<<8)
#define PMU_CRYSTAL_NORMAL_VOLT(n)      (((n) & 0x7) << 4)
#define PMU_CRYSTAL_NORMAL_VOLT_MASK    (0x7 << 4)
#define PMU_CRYSTAL_NORMAL_VOLT_SHIFT   (3)
#define PMU_CRYSTAL_DSLEEP_VOLT(n)      (((n) & 0x7) << 0)
#define PMU_CRYSTAL_DSLEEP_VOLT_MASK    (0x7 << 0)
#define PMU_CRYSTAL_DSLEEP_VOLT_SHIFT   (0)

// 0E   CODEC SETTING
#define PMU_CODEC_DSLEEP_ON             (1<<15)
#define PMU_CODEC_VCODEC_DR             (1<<14)
#define PMU_CODEC_VCODEC_EN             (1<<13)
#define PMU_CODEC_LP_MODE_EN            (1<<12)
#define PMU_CODEC_LP_EN_DR              (1<<11)
#define PMU_CODEC_LP_EN_REG             (1<<10)
#define PMU_CODEC_NORMAL_VOLT(n)        (((n) & 0x1F) << 5)
#define PMU_CODEC_NORMAL_VOLT_MASK      (0x1F << 5)
#define PMU_CODEC_NORMAL_VOLT_SHIFT     (5)
#define PMU_CODEC_DSLEEP_VOLT(n)        (((n) & 0x1F) << 0)
#define PMU_CODEC_DSLEEP_VOLT_MASK      (0x1F << 0)
#define PMU_CODEC_DSLEEP_VOLT_SHIFT     (0)

// 13   BUCK VOLT SETTING
#define PMU_BUCK_VCORE_NORMAL(n)        (((n) & 0xF) << 12)
#define PMU_BUCK_VCORE_NORMAL_MASK      (0xF << 12)
#define PMU_BUCK_VCORE_NORMAL_SHIFT     (12)
#define PMU_BUCK_VCORE_DSLEEP(n)        (((n) & 0xF) << 8)
#define PMU_BUCK_VCORE_DSLEEP_MASK      (0xF << 8)
#define PMU_BUCK_VCORE_DSLEEP_SHIFT     (8)
#define PMU_BUCK_VANA_NORMAL(n)         (((n) & 0xF) << 4)
#define PMU_BUCK_VANA_NORMAL_MASK       (0xF << 4)
#define PMU_BUCK_VANA_NORMAL_SHIFT      (4)
#define PMU_BUCK_VANA_DSLEEP(n)         (((n) & 0xF) << 0)
#define PMU_BUCK_VANA_DSLEEP_MASK       (0xF << 0)
#define PMU_BUCK_VANA_DSLEEP_SHIFT      (0)

// 14   DCDC SETTING
#define PMU_TEST_MODE_SHIFT             13
#define PMU_TEST_MODE_MASK              (0x7 << PMU_TEST_MODE_SHIFT)
#define PMU_TEST_MODE(n)                BITFIELD_VAL(PMU_TEST_MODE, n)
#define PMU_BUCK_CC_MODE_EN             (1<<12)
#define PMU_DCDC_ANA_LP_MODE_EN         (1<<11)
#define PMU_DCDC_ANA_DR                 (1<<10)
#define PMU_DCDC_ANA_EN                 (1<<9)
#define PMU_DCDC_ANA_DSLEEP_ON          (1<<6)
#define PMU_DCDC_DIG_LP_MODE_EN         (1<<5)
#define PMU_DCDC_DIG_DR                 (1<<4)
#define PMU_DCDC_DIG_EN                 (1<<3)
#define PMU_DCDC_DIG_DSLEEP_ON          (1<<0)

// 17   PMU_REG_BUCK_VCOMP
#define PMU_BUCK_HV_ANA_NORMAL          (1 << 13)
#define PMU_BUCK_HV_ANA_DSLEEP          (1 << 12)

// 1A   PMU_REG_PWR_SEL
#define PMU_PMU_VSEL1_SHIFT             13
#define PMU_PMU_VSEL1_MASK              (0x7 << PMU_PMU_VSEL1_SHIFT)
#define PMU_PMU_VSEL1(n)                BITFIELD_VAL(PMU_PMU_VSEL1, n)
#define PMU_POWER_SEL_CNT_SHIFT         8
#define PMU_POWER_SEL_CNT_MASK          (0x1F << PMU_POWER_SEL_CNT_SHIFT)
#define PMU_POWER_SEL_CNT(n)            BITFIELD_VAL(PMU_POWER_SEL_CNT, n)
#define PMU_PWR_SEL_DR                  (1 << 7)
#define PMU_PWR_SEL                     (1 << 6)
#define PMU_CLK_BG_EN                   (1 << 5)
#define PMU_CLK_BG_DIV_VALUE_SHIFT      0
#define PMU_CLK_BG_DIV_VALUE_MASK       (0x1F << PMU_CLK_BG_DIV_VALUE_SHIFT)
#define PMU_CLK_BG_DIV_VALUE(n)         BITFIELD_VAL(PMU_CLK_BG_DIV_VALUE, n)

// 21   PMU_REG_USBPHY
#define PMU_EN_USBDIGPHY_CLK            (1 << 11)
#define PMU_RESETN_USBPHY_DR            (1 << 10)
#define PMU_RESETN_USBPHY_REG           (1 << 9)

// 29   PMU_REG_MIC_BIAS_A
#define PMU_MIC_BIASA_CHANSEL_SHIFT     14
#define PMU_MIC_BIASA_CHANSEL_MASK      (0x3 << PMU_MIC_BIASA_CHANSEL_SHIFT)
#define PMU_MIC_BIASA_CHANSEL(n)        BITFIELD_VAL(PMU_MIC_BIASA_CHANSEL, n)
#define PMU_MIC_BIASA_EN                (1 << 13)
#define PMU_MIC_BIASA_ENLPF             (1 << 12)
#define PMU_MIC_BIASA_LPFSEL_SHIFT      10
#define PMU_MIC_BIASA_LPFSEL_MASK       (0x3 << PMU_MIC_BIASA_LPFSEL_SHIFT)
#define PMU_MIC_BIASA_LPFSEL(n)         BITFIELD_VAL(PMU_MIC_BIASA_LPFSEL, n)
#define PMU_MIC_BIASA_VSEL_SHIFT        5
#define PMU_MIC_BIASA_VSEL_MASK         (0x1F << PMU_MIC_BIASA_VSEL_SHIFT)
#define PMU_MIC_BIASA_VSEL(n)           BITFIELD_VAL(PMU_MIC_BIASA_VSEL, n)
#define PMU_MIC_LDO_RES_SHIFT           1
#define PMU_MIC_LDO_RES_MASK            (0xF << PMU_MIC_LDO_RES_SHIFT)
#define PMU_MIC_LDO_RES(n)              BITFIELD_VAL(PMU_MIC_LDO_RES, n)

// 2A   PMU_REG_MIC_BIAS_B
#define PMU_MIC_BIASB_CHANSEL_SHIFT     14
#define PMU_MIC_BIASB_CHANSEL_MASK      (0x3 << PMU_MIC_BIASB_CHANSEL_SHIFT)
#define PMU_MIC_BIASB_CHANSEL(n)        BITFIELD_VAL(PMU_MIC_BIASB_CHANSEL, n)
#define PMU_MIC_BIASB_EN                (1 << 13)
#define PMU_MIC_BIASB_ENLPF             (1 << 12)
#define PMU_MIC_BIASB_LPFSEL_SHIFT      10
#define PMU_MIC_BIASB_LPFSEL_MASK       (0x3 << PMU_MIC_BIASB_LPFSEL_SHIFT)
#define PMU_MIC_BIASB_LPFSEL(n)         BITFIELD_VAL(PMU_MIC_BIASB_LPFSEL, n)
#define PMU_MIC_BIASB_VSEL_SHIFT       5
#define PMU_MIC_BIASB_VSEL_MASK         (0x1F << PMU_MIC_BIASB_VSEL_SHIFT)
#define PMU_MIC_BIASB_VSEL(n)           BITFIELD_VAL(PMU_MIC_BIASB_VSEL, n)
#define PMU_MIC_LDO_EN                  (1 << 4)
//#define PMU_MIC_LDO_LOOPCTRL            (1 << 4)
#define PMU_MIC_LDO_PULLDOWN            (1 << 3)

// 2B   PMU_REG_LED_CFG
#define PMU_LED_IO1_IBIT_SHIFT          14
#define PMU_LED_IO1_IBIT_MASK           (0x3 << PMU_LED_IO1_IBIT_SHIFT)
#define PMU_LED_IO1_IBIT(n)             BITFIELD_VAL(PMU_LED_IO1_IBIT, n)
#define PMU_LED_IO1_OENB                (1 << 13)
#define PMU_LED_IO1_PDEN                (1 << 12)
#define PMU_LED_IO1_PU                  (1 << 11)
#define PMU_LED_IO1_PUEN                (1 << 10)
#define PMU_LED_IO1_SEL_SHIFT           8
#define PMU_LED_IO1_SEL_MASK            (0x3 << PMU_LED_IO1_SEL_SHIFT)
#define PMU_LED_IO1_SEL(n)              BITFIELD_VAL(PMU_LED_IO1_SEL, n)

// 2C   PMU_REG_USB11_CFG
#define PMU_PU_USB11_DR                 (1 << 11)
#define PMU_PU_USB11_REG                (1 << 10)

// 4F   PMU_REG_POWER_OFF
#define PMU_SOFT_POWER_OFF              (1 << 0)

// 5E   PMU_REG_CHARGER_STATUS
#define PMU_AC_ON_DET_OUT_MASKED        (1 << 4)
#define PMU_AC_ON_DET_IN_MASKED         (1 << 3)
#define PMU_AC_ON                       (1 << 2)
#define PMU_AC_ON_DET_OUT               (1 << 1)
#define PMU_AC_ON_DET_IN                (1 << 0)

// !!! REG BITFIELD CHECKED !!!

// 1D   SLEEP ENABLE
#define PMU_SLEEP_EN                    (1 << 15)

// 1F   PMU_REG_INT_MASK
#define PMU_INT_MASK_USB_INSERT         (1 << 15)
#define PMU_INT_MASK_RTC1               (1 << 14)
#define PMU_INT_MASK_RTC0               (1 << 13)

// 20   PMU_REG_INT_EN
#define PMU_INT_EN_USB_INSERT           (1 << 15)
#define PMU_INT_EN_RTC1                 (1 << 14)
#define PMU_INT_EN_RTC0                 (1 << 13)

// 21   PMU_REG_USB_PIN_POL
#define PMU_USB_POL_RX_DP               (1 << 13)
#define PMU_USB_POL_RX_DM               (1 << 12)

// 2A   PMU_REG_USB_PIN_CFG
#define PMU_USB_DEBOUNCE_EN             (1 << 2)
#define PMU_USB_NOLS_MODE               (1 << 1)
#define PMU_USB_INSERT_DET_EN           (1 << 0)

// 31   PMU_REG_EFUSE_CTRL
#define REG_EFUSE_WRITE_COUNTER_SHIFT       8
#define REG_EFUSE_WRITE_COUNTER_MASK        (0xFF << REG_EFUSE_WRITE_COUNTER_SHIFT)
#define REG_EFUSE_WRITE_COUNTER(n)          BITFIELD_VAL(REG_EFUSE_WRITE_COUNTER, n)
#define REG_EFUSE_READ_COUNTER_SHIFT        4
#define REG_EFUSE_READ_COUNTER_MASK         (0xF << REG_EFUSE_READ_COUNTER_SHIFT)
#define REG_EFUSE_READ_COUNTER(n)           BITFIELD_VAL(REG_EFUSE_READ_COUNTER, n)
#define REG_EFUSE_PROGRAM_MODE_EN           (1 << 3)
#define REG_EFUSE_PU_AVDD                   (1 << 2)
#define REG_EFUSE_READ_MODE_EN              (1 << 1)
#define REG_EFUSE_CLK_EN                    (1 << 0)

// 32   PMU_REG_EFUSE_SEL
#define REG_EFUSE_PGMEN_OFF_COUNTER_SHIFT   12
#define REG_EFUSE_PGMEN_OFF_COUNTER_MASK    (0xF << REG_EFUSE_PGMEN_OFF_COUNTER_SHIFT)
#define REG_EFUSE_PGMEN_OFF_COUNTER(n)      BITFIELD_VAL(REG_EFUSE_PGMEN_OFF_COUNTER, n)
#define REG_EFUSE_PGMEN_ON_COUNTER_SHIFT    8
#define REG_EFUSE_PGMEN_ON_COUNTER_MASK     (0xF << REG_EFUSE_PGMEN_ON_COUNTER_SHIFT)
#define REG_EFUSE_PGMEN_ON_COUNTER(n)       BITFIELD_VAL(REG_EFUSE_PGMEN_ON_COUNTER, n)
#define REG_EFUSE_ADDRESS_SHIFT             0
#define REG_EFUSE_ADDRESS_MASK              (0xFF << REG_EFUSE_ADDRESS_SHIFT)
#define REG_EFUSE_ADDRESS(n)                BITFIELD_VAL(REG_EFUSE_ADDRESS, n)

// 34   PMU_REG_EFUSE_READ_TRIG
#define RESERVED_ANA                        (1 << 15)
#define CLK_32K_COUNT_NUM_SHIFT             11
#define CLK_32K_COUNT_NUM_MASK              (0xF << CLK_32K_COUNT_NUM_SHIFT)
#define CLK_32K_COUNT_NUM(n)                BITFIELD_VAL(CLK_32K_COUNT_NUM, n)
#define REG_IPTAT_CORE_I_SEL_SHIFT          5
#define REG_IPTAT_CORE_I_SEL_MASK           (0x3F << REG_IPTAT_CORE_I_SEL_SHIFT)
#define REG_IPTAT_CORE_I_SEL(n)             BITFIELD_VAL(REG_IPTAT_CORE_I_SEL, n)
#define DIG_IPTAT_CORE_EN                   (1 << 4)
#define REG_EFUSE_DATA_OUT_DR               (1 << 3)
#define REG_EFUSE_WRITE_AUTO_MODE           (1 << 2)
#define REG_EFUSE_TRIGGER_WRITE             (1 << 1)
#define REG_EFUSE_TRIGGER_READ              (1 << 0)

// 50   PMU_REG_INT_STATUS
#define PMU_INT_STATUS_USB_INSERT       (1 << 15)
#define PMU_INT_STATUS_RTC1             (1 << 14)
#define PMU_INT_STATUS_RTC0             (1 << 13)

// 51   PMU_REG_INT_CLR
#define PMU_INT_CLR_USB_INSERT          (1 << 15)
#define PMU_INT_CLR_RTC1                (1 << 14)
#define PMU_INT_CLR_RTC0                (1 << 13)

// 53   PMU_REG_EFUSE_VAL
#define SMIC_EFUSE_A_SHIFT                  8
#define SMIC_EFUSE_A_MASK                   (0xFF << SMIC_EFUSE_A_SHIFT)
#define SMIC_EFUSE_A(n)                     BITFIELD_VAL(SMIC_EFUSE_A, n)
#define SMIC_EFUSE_Q_SHIFT                  0
#define SMIC_EFUSE_Q_MASK                   (0xFF << SMIC_EFUSE_Q_SHIFT)
#define SMIC_EFUSE_Q(n)                     BITFIELD_VAL(SMIC_EFUSE_Q, n)

// 5F   PMU_REG_USB_PIN_STATUS
#define PMU_USB_STATUS_RX_DM            (1 << 10)
#define PMU_USB_STATUS_RX_DP            (1 << 9)

// ANA_70
#define ANA_70_DIG_DBL_EN                   (1 << 8)
#define ANA_70_DIG_DBL_RST                  (1 << 9)

// ANA_71
#define ANA_71_REG_DBL_FREQ_SEL             (1 << 14)

// USBPHY_18
#define USBPHY_18_DIG_USBPLL_USB_EN         (1 << 6)

enum PMU_REG_T {
    PMU_REG_METAL_ID            = 0x00,
    PMU_REG_POWER_KEY_CFG       = 0x02,
    PMU_REG_BIAS_CFG            = 0x03,
    PMU_REG_CHARGER_CFG         = 0x05,
    PMU_REG_ANA_CFG             = 0x07,
    PMU_REG_DIG_CFG             = 0x08,
    PMU_REG_IO_CFG              = 0x09,
    PMU_REG_GP_CFG              = 0x0A,
    PMU_REG_USB_CFG             = 0x0B,
    PMU_REG_CRYSTAL_CFG         = 0x0C,
    PMU_REG_CODEC_CFG           = 0x0E,
    PMU_REG_BUCK_VOLT           = 0x13,
    PMU_REG_DCDC_CFG            = 0x14,
    PMU_REG_BUCK_FREQ           = 0x15,
    PMU_REG_PWR_SEL             = 0x1A,
    PMU_REG_USBPHY              = 0x21,
    PMU_REG_RTC_DIV_1HZ         = 0x22,   
    PMU_REG_RTC_LOAD_LOW        = 0x23,
    PMU_REG_RTC_LOAD_HIGH       = 0x24,
    PMU_REG_RTC_MATCH0_LOW      = 0x25,
    PMU_REG_RTC_MATCH0_HIGH     = 0x26,
    PMU_REG_MIC_BIAS_A          = 0x29,
    PMU_REG_MIC_BIAS_B          = 0x2A,
    PMU_REG_LED_CFG             = 0x2B,
    PMU_REG_USB11_CFG           = 0x2C,   
    PMU_REG_RESERVED_ANA        = 0x38,
    PMU_REG_POWER_OFF           = 0x4F,
    PMU_REG_RTC_VAL_LOW         = 0x54,
    PMU_REG_RTC_VAL_HIGH        = 0x55,
    PMU_REG_CHARGER_STATUS      = 0x5E,

    PMU_REG_MODULE_START        = PMU_REG_ANA_CFG,

// !!! REG CHECKED !!!

    PMU_REG_BUCK_VCOMP          = 0x17,
    PMU_REG_SLEEP_CFG           = 0x1D,
    PMU_REG_INT_MASK            = 0x1F,
    PMU_REG_INT_EN              = 0x20,
    PMU_REG_USB_PIN_POL         = 0x21,
    PMU_REG_USB_PIN_CFG         = 0x2A,
    PMU_REG_EFUSE_CTRL          = 0x31,
    PMU_REG_EFUSE_SEL           = 0x32,
    PMU_REG_EFUSE_READ_TRIG     = 0x34,
    PMU_REG_INT_STATUS          = 0x50,
    PMU_REG_INT_CLR             = 0x51,
    PMU_REG_EFUSE_VAL           = 0x53,
    PMU_REG_USB_PIN_STATUS      = 0x5F,

    ANA_REG_70                  = 0x70,
    ANA_REG_71                  = 0x71,

    USBPHY_REG_18               = 0x18,
};

enum PMU_VCORE_REQ_T {
    PMU_VCORE_FLASH_WRITE_ENABLED   = (1 << 0),
    PMU_VCORE_FLASH_FREQ_HIGH       = (1 << 1),
    PMU_VCORE_FLASH_FREQ_MEDIUM     = (1 << 2),
    PMU_VCORE_PSRAM_FREQ_HIGH       = (1 << 3),
    PMU_VCORE_USB_ENABLED           = (1 << 4),
    PMU_VCORE_IIR_FREQ_MEDIUM       = (1 << 5),
    PMU_VCORE_IIR_FREQ_HIGH         = (1 << 6),
    PMU_VCORE_SYS_FREQ_HIGH         = (1 << 7),
};

enum CRYSTAL_FREQ_T {
    CRYSTAL_FREQ_24M = 0,
    CRYSTAL_FREQ_26M,
    CRYSTAL_FREQ_40M,
    CRYSTAL_FREQ_48M,
};

union BOOT_SETTINGS_T {
    struct {
        unsigned short usb_dld_dis      :1;
        unsigned short uart_dld_en      :1;
        unsigned short uart_trace_en    :1;
        unsigned short pll_dis          :1;
        unsigned short uart_baud_div2   :1;
        unsigned short sec_freq_div2    :1;
        unsigned short crystal_freq     :2;
        unsigned short reserved         :4;
        unsigned short chksum           :4;
    };
    unsigned short reg;
};

enum PMU_MODUAL_T {
    PMU_ANA,
    PMU_DIG,
    PMU_IO,
    PMU_GP,
    PMU_USB,
    PMU_CRYSTAL,
    PMU_CODEC,
};

struct PMU_MODULE_CFG_T {
    unsigned short manual_bit;
    unsigned short ldo_en;
    unsigned short lp_mode;
    unsigned short dsleep_mode;
    unsigned short dsleep_v;
    unsigned short dsleep_v_shift;
    unsigned short normal_v;
    unsigned short normal_v_shift;
};

#define PMU_MOD_CFG_VAL(m)              { \
    PMU_##m##_V##m##_DR, PMU_##m##_V##m##_EN, \
    PMU_##m##_LP_MODE_EN, PMU_##m##_DSLEEP_ON, \
    PMU_##m##_DSLEEP_VOLT_MASK, PMU_##m##_DSLEEP_VOLT_SHIFT,\
    PMU_##m##_NORMAL_VOLT_MASK, PMU_##m##_NORMAL_VOLT_SHIFT }

static const struct PMU_MODULE_CFG_T pmu_module_cfg[] = {
    PMU_MOD_CFG_VAL(ANA),
    PMU_MOD_CFG_VAL(DIG),
    PMU_MOD_CFG_VAL(IO),
    PMU_MOD_CFG_VAL(GP),
    PMU_MOD_CFG_VAL(USB),
    PMU_MOD_CFG_VAL(CRYSTAL),
    PMU_MOD_CFG_VAL(CODEC),
};

#ifdef ANC_PROD_TEST
#define OPT_TYPE
#else
#define OPT_TYPE                        const
#endif

#if 0
static OPT_TYPE bool audio_output_diff =
#ifdef AUDIO_OUTPUT_DIFF
    true;
#else
    false;
#endif
#endif

static OPT_TYPE bool vcodec_off =
#ifdef VCODEC_OFF
    true;
#else
    false;
#endif
static OPT_TYPE uint8_t ana_act_dcdc =
#if defined(VANA_2P5V)
#ifdef RELIABLITY_TEST
    PMU_DCDC_ANA2_2_7V;
#else
    PMU_DCDC_ANA2_2_5V;
#endif
#elif defined(VANA_2P3V)
    PMU_DCDC_ANA_2_3V;
#elif defined(VANA_2P1V)
    PMU_DCDC_ANA_2_1V;
#elif defined(VANA_2P0V)
	PMU_DCDC_ANA_2_0V;
#elif defined(VANA_1P9V)
	PMU_DCDC_ANA_1_9V;
#elif defined(VANA_1P5V)
    PMU_DCDC_ANA_1_5V;
#elif defined(VANA_1P6V)
    PMU_DCDC_ANA_1_6V;
#else
    PMU_DCDC_ANA_1_8V;
#endif
static OPT_TYPE uint16_t POSSIBLY_UNUSED vcodec_mv =
#ifdef VCODEC_1P5V
    1500;
#elif defined(VCODEC_1P6V)
    1600;
#elif defined(VCODEC_2P5V)
    2500;
#elif defined(VCODEC_3P5V)
    3500;
#else // VCODEC_1P8V
    1800;
#endif
static OPT_TYPE bool POSSIBLY_UNUSED vcrystal_off =
#ifdef VCRYSTAL_OFF
    true;
#else
    false;
#endif

static enum PMU_POWER_MODE_T BOOT_BSS_LOC pmu_power_mode = PMU_POWER_MODE_NONE;
static enum PMU_POWER_MODE_T pmu_active_power_mode = PMU_POWER_MODE_NONE;

static enum PMU_VCORE_REQ_T BOOT_BSS_LOC pmu_vcore_req;

static enum PMU_USB_PIN_CHK_STATUS_T usb_pin_status;

static PMU_USB_PIN_CHK_CALLBACK usb_pin_callback;

static PMU_CHARGER_IRQ_HANDLER_T charger_irq_handler;

static uint8_t SRAM_BSS_DEF(vio_risereq_map);
STATIC_ASSERT(sizeof(vio_risereq_map) * 8 >= PMU_VIORISE_REQ_USER_QTY, "vio_risereq_map size too small");

static uint8_t SRAM_DATA_DEF(vio_act_normal) = IO_VOLT_ACTIVE_NORMAL;
static uint8_t SRAM_DATA_DEF(vio_act_rise) = IO_VOLT_ACTIVE_RISE;
static uint8_t SRAM_DATA_DEF(vio_lp) = IO_VOLT_SLEEP;

static const uint8_t ana_lp_dcdc = PMU_DCDC_ANA_SLEEP_1_1V;

#define PMU_LDO_ANA_2_7V                0xf
#define PMU_LDO_ANA_2_6V                0xe
#define PMU_LDO_ANA_2_5V                0xd
#define PMU_LDO_ANA_2_4V                0xc
#define PMU_LDO_ANA_2_3V                0xb
#define PMU_LDO_ANA_2_2V                0xa
#define PMU_LDO_ANA_2_1V                0x9
#define PMU_LDO_ANA_2_0V                0x8
#define PMU_LDO_ANA_1_9V                0x7
#define PMU_LDO_ANA_1_8V                0x6

//static const uint8_t ana_lp_ldo = PMU_LDO_ANA_1_8V;
#ifdef RELIABLITY_TEST
//static const uint8_t ana_act_ldo = PMU_LDO_ANA_2_5V;
#else
//static const uint8_t ana_act_ldo = PMU_LDO_ANA_2_5V;
#endif

// Move all the data/bss invovled in pmu_open() to .sram_data/.sram_bss,
// so that pmu_open() can be called at the end of BootInit(),
// for data/bss is initialized after BootInit().
static uint16_t SRAM_BSS_DEF(dig_act_ldo);
static const uint8_t dig_lp_ldo = PMU_VDIG_0_8V;
static const uint8_t dig_lp_dcdc = PMU_DCDC_DIG_0_8V;

#ifdef RTC_ENABLE
struct PMU_RTC_CTX_T {
    bool enabled;
    bool alarm_set;
    uint32_t alarm_val;
};

static struct PMU_RTC_CTX_T BOOT_BSS_LOC rtc_ctx;

static PMU_RTC_IRQ_HANDLER_T rtc_irq_handler;

static void BOOT_TEXT_SRAM_LOC pmu_rtc_save_context(void)
{
    if (pmu_rtc_enabled()) {
        rtc_ctx.enabled = true;
        if (pmu_rtc_alarm_status_set()) {
            rtc_ctx.alarm_set = true;
            rtc_ctx.alarm_val = pmu_rtc_get_alarm();
        }
    } else {
        rtc_ctx.enabled = false;
    }
}

static void pmu_rtc_restore_context(void)
{
    uint32_t rtc_val;

    if (rtc_ctx.enabled) {
        pmu_rtc_enable();
        if (rtc_ctx.alarm_set) {
            rtc_val = pmu_rtc_get();
            if (rtc_val - rtc_ctx.alarm_val <= 1 || rtc_ctx.alarm_val - rtc_val < 5) {
                rtc_ctx.alarm_val = rtc_val + 5;
            }
            pmu_rtc_set_alarm(rtc_ctx.alarm_val);
        }
    }
}
#endif

static unsigned int NOINLINE BOOT_TEXT_SRAM_LOC pmu_count_zeros(unsigned int val, unsigned int bits)
{
    int cnt = 0;
    int i;

    for (i = 0; i < bits; i++) {
        if ((val & (1 << i)) == 0) {
            cnt++;
        }
    }

    return cnt;
}

uint32_t BOOT_TEXT_FLASH_LOC read_hw_metal_id(void)
{
    int ret;
    uint16_t val;
    uint32_t metal_id;
    union BOOT_SETTINGS_T boot;

#ifdef RTC_ENABLE
    // RTC will be restored in pmu_open()
    pmu_rtc_save_context();
#endif

    // Reset PMU (to recover from a possible insane state, e.g., ESD reset)
    pmu_write(PMU_REG_METAL_ID, 0xCAFE);
    pmu_write(PMU_REG_METAL_ID, 0x5FEE);
    hal_sys_timer_delay(4);

    ret = pmu_get_efuse(PMU_EFUSE_PAGE_BOOT, &boot.reg);
    if (ret) {
        boot.reg = 0;
    } else {
        if (pmu_count_zeros(boot.reg, 12) != boot.chksum) {
            boot.reg = 0;
        }
    }
    hal_cmu_set_crystal_freq_index(boot.crystal_freq);
    // Update ISPI cfg
    ret = hal_analogif_open();
    if (ret) {
        SAFE_PROGRAM_STOP();
    }

#if 0
#ifdef USB_HIGH_SPEED
    usbphy_read(USBPHY_REG_18, &val);
    val |= USBPHY_18_DIG_USBPLL_USB_EN;
    usbphy_write(USBPHY_REG_18, val);

    analog_read(0x63, &val);
    val |= (1<<13);         // to choose USB PLL
    analog_write(0x63, val);
#else
    usbphy_read(USBPHY_REG_18, &val);
    val &= ~USBPHY_18_DIG_USBPLL_USB_EN;
    usbphy_write(USBPHY_REG_18, val);

    analog_read(0x63, &val);
    val &= ~(1<<13);         // to choose USB PLL
    analog_write(0x63, val);
#endif
#endif

    // Enable 26M doubler (52M)
    analog_read(ANA_REG_70, &val);
    val = (val & ~ANA_70_DIG_DBL_EN) | ANA_70_DIG_DBL_RST;
    analog_write(ANA_REG_70, val);

    analog_read(ANA_REG_71, &val);
#ifdef ANA_26M_X4_ENABLE
    val |= ANA_71_REG_DBL_FREQ_SEL;
#else
    val &= ~ANA_71_REG_DBL_FREQ_SEL;
#endif
    analog_write(ANA_REG_71, val);

    analog_read(ANA_REG_70, &val);
    val |= ANA_70_DIG_DBL_EN;
    analog_write(ANA_REG_70, val);
    hal_sys_timer_delay(US_TO_TICKS(100));
    val &= ~ANA_70_DIG_DBL_RST;
    analog_write(ANA_REG_70, val);

    ret = pmu_read(PMU_REG_METAL_ID, &val);
    if (ret) {
        return 0;
    }

    metal_id = GET_BITFIELD(val, PMU_METAL_ID);

    return metal_id;
}

int BOOT_TEXT_SRAM_LOC pmu_get_efuse(enum PMU_EFUSE_PAGE_T page, unsigned short *efuse)
{
    int ret;
    unsigned short val;
    unsigned char bytes[2];
    int i;

    // Disable READ_MODE_EN
    val = REG_EFUSE_WRITE_COUNTER(0x38) | REG_EFUSE_READ_COUNTER(2) | REG_EFUSE_CLK_EN;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        return ret;
    }
    // Disable CLK_EN
    val &= ~REG_EFUSE_CLK_EN;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }
    // Enable CLK_EN
    val |= REG_EFUSE_CLK_EN;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }
    // Enable READ_MODE_EN
    val |= REG_EFUSE_READ_MODE_EN;
    ret = pmu_write(PMU_REG_EFUSE_CTRL, val);
    if (ret) {
        goto _exit;
    }

    for (i = 0; i < 2; i++) {
        val = REG_EFUSE_PGMEN_OFF_COUNTER(3) | REG_EFUSE_PGMEN_ON_COUNTER(3) | REG_EFUSE_ADDRESS(page * 2 + i);
        ret = pmu_write(PMU_REG_EFUSE_SEL, val);
        if (ret) {
            goto _exit;
        }
        ret = pmu_read(PMU_REG_EFUSE_READ_TRIG, &val);
        if (ret) {
            goto _exit;
        }
        val |= REG_EFUSE_TRIGGER_READ;
        ret = pmu_write(PMU_REG_EFUSE_READ_TRIG, val);
        if (ret) {
            goto _exit;
        }
        val &= ~REG_EFUSE_TRIGGER_READ;
        ret = pmu_write(PMU_REG_EFUSE_READ_TRIG, val);
        if (ret) {
            goto _exit;
        }
        ret = pmu_read(PMU_REG_EFUSE_VAL, &val);
        if (ret) {
            goto _exit;
        }
        bytes[i] = GET_BITFIELD(val, SMIC_EFUSE_Q);
    }

    *efuse = (bytes[1] << 8) | bytes[0];

_exit:
    // Disable READ_MODE_EN
    val = REG_EFUSE_WRITE_COUNTER(0x38) | REG_EFUSE_READ_COUNTER(2) | REG_EFUSE_CLK_EN;
    pmu_write(PMU_REG_EFUSE_CTRL, val);
    // Disable CLK_EN
    val &= ~REG_EFUSE_CLK_EN;
    pmu_write(PMU_REG_EFUSE_CTRL, val);

    return ret;
}

int pmu_get_security_value(union SECURITY_VALUE_T *val)
{
    int ret;

    ret = pmu_get_efuse(PMU_EFUSE_PAGE_SECURITY, &val->reg);
    if (ret) {
        // Error
        goto _no_security;
    }

    if (!val->security_en) {
        // OK
        goto _no_security;
    }
    ret = 1;
    if (pmu_count_zeros(val->key_id, 3) != val->key_chksum) {
        // Error
        goto _no_security;
    }
    if (pmu_count_zeros(val->vendor_id, 6) != val->vendor_chksum) {
        // Error
        goto _no_security;
    }
    if ((pmu_count_zeros(val->reg, 15) & 1) != val->chksum) {
        // Error
        goto _no_security;
    }

    // OK
    return 0;

_no_security:
    val->reg = 0;

    return ret;
}

void pmu_shutdown(void)
{
    uint16_t val;
    uint32_t lock = int_lock();

#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
    pmu_mode_change(PMU_POWER_MODE_LDO);
    hal_sys_timer_delay(MS_TO_TICKS(1));
#endif

#ifdef RTC_ENABLE
    pmu_rtc_save_context();
#endif

    // Reset PMU
    pmu_write(PMU_REG_METAL_ID, 0xCAFE);
    pmu_write(PMU_REG_METAL_ID, 0x5FEE);
    hal_sys_timer_delay(4);

#ifdef RTC_ENABLE
    pmu_rtc_restore_context();
#endif

    pmu_read(PMU_REG_PWR_SEL, &val);
    val |= PMU_PWR_SEL_DR | PMU_PWR_SEL;
    pmu_write(PMU_REG_PWR_SEL, val);

    pmu_read(PMU_REG_POWER_OFF,&val);
    val |= PMU_SOFT_POWER_OFF;
    pmu_write(PMU_REG_POWER_OFF,val);
    hal_sys_timer_delay(MS_TO_TICKS(50));

    //can't reach here
    TRACE_IMM("\nError: pmu_shutdown failed!\n");
    hal_sys_timer_delay(MS_TO_TICKS(5));
    hal_cmu_sys_reboot();

    int_unlock(lock);
}

static inline uint16_t pmu_get_module_addr(enum PMU_MODUAL_T module)
{
    if (module == PMU_CODEC) {
        return module + PMU_REG_MODULE_START + 1;
    } else {
        return module + PMU_REG_MODULE_START;
    }
}

void pmu_module_config(enum PMU_MODUAL_T module,unsigned short is_manual,unsigned short ldo_on,unsigned short lp_mode,unsigned short dpmode)
{
    int ret;
    unsigned short val;
    unsigned char module_address;
    const struct PMU_MODULE_CFG_T *module_cfg_p = &pmu_module_cfg[module];

    module_address = pmu_get_module_addr(module);

    ret = pmu_read(module_address, &val);
    if(ret == 0)
    {
#ifdef VCORE_LDO_ON
        if (module == PMU_DIG /*&& hal_get_chip_metal_id() == 0*/) {
            // VCORE should NEVER be disabled
            if (ldo_on == PMU_LDO_OFF) {
                vcore_ldo_off = true;
                // Force to enter LP mode and set voltage to min
                val |= PMU_DIG_LP_MODE_EN | PMU_DIG_LP_EN_DR | PMU_DIG_LP_EN_REG | PMU_DIG_DSLEEP_ON | PMU_DIG_VDIG_DR | PMU_DIG_VDIG_EN;
                val = (val & ~(PMU_DIG_DSLEEP_VOLT_MASK | PMU_DIG_NORMAL_VOLT_MASK)) |
                    PMU_DIG_DSLEEP_VOLT(0) | PMU_DIG_NORMAL_VOLT(0);
            } else {
                vcore_ldo_off = false;
                // Cancel LP mode direct-reg and restore voltage
                val |= PMU_DIG_LP_MODE_EN | PMU_DIG_DSLEEP_ON | PMU_DIG_VDIG_DR | PMU_DIG_VDIG_EN;
                val &= ~(PMU_DIG_LP_EN_DR | PMU_DIG_LP_EN_REG);
                val = (val & ~(PMU_DIG_DSLEEP_VOLT_MASK | PMU_DIG_NORMAL_VOLT_MASK)) |
                    PMU_DIG_DSLEEP_VOLT(dig_lp_ldo) | PMU_DIG_NORMAL_VOLT(dig_act_ldo);
            }
        } else
#endif
        {
            if (module == PMU_USB) {
                uint16_t usb11;

                pmu_read(PMU_REG_USB11_CFG, &usb11);
                usb11 |= PMU_PU_USB11_DR;
                if (ldo_on) {
                    usb11 |= PMU_PU_USB11_REG;
                } else {
                    usb11 &= ~PMU_PU_USB11_REG;
                }
                pmu_write(PMU_REG_USB11_CFG, usb11);
            }

            if(is_manual)
            {
                val |= module_cfg_p->manual_bit;
            }
            else
            {
                val &= ~module_cfg_p->manual_bit;
            }
            if(ldo_on)
            {
                val |= module_cfg_p->ldo_en;
            }
            else
            {
                val &= ~module_cfg_p->ldo_en;
            }
            if(lp_mode)
            {
                val |= module_cfg_p->lp_mode;
            }
            else
            {
                val &= ~module_cfg_p->lp_mode;
            }
            if(dpmode)
            {
                val |= module_cfg_p->dsleep_mode;
            }
            else
            {
                val &= ~module_cfg_p->dsleep_mode;
            }
        }
        pmu_write(module_address, val);
    }
}

void pmu_module_set_volt(unsigned char module, unsigned short sleep_v,unsigned short normal_v)
{
    int ret;
    unsigned short val;
    unsigned char module_address;
    const struct PMU_MODULE_CFG_T *module_cfg_p = &pmu_module_cfg[module];

#ifdef VCORE_LDO_ON
    if (module == PMU_DIG /*&& hal_get_chip_metal_id() == 0*/) {
        if (vcore_ldo_off) {
            // Never change vcore ldo voltage when it is switched off nominally
            return;
        }
    }
#endif

    module_address = pmu_get_module_addr(module);

    ret = pmu_read(module_address, &val);
    if(ret == 0)
    {
        val &= ~module_cfg_p->normal_v;
        val |= (normal_v << module_cfg_p->normal_v_shift) & module_cfg_p->normal_v;
        val &= ~module_cfg_p->dsleep_v;
        val |= (sleep_v << module_cfg_p->dsleep_v_shift) & module_cfg_p->dsleep_v;
        pmu_write(module_address, val);
    }
}

int pmu_module_get_volt(unsigned char module, unsigned short *sleep_vp,unsigned short *normal_vp)
{
    int ret;
    unsigned short val;
    unsigned char module_address;
    const struct PMU_MODULE_CFG_T *module_cfg_p = &pmu_module_cfg[module];

    module_address = pmu_get_module_addr(module);

    ret = pmu_read(module_address, &val);
    if (ret == 0) {
        if (normal_vp) {
            *normal_vp = (val & module_cfg_p->normal_v) >> module_cfg_p->normal_v_shift;
        }
        if (sleep_vp) {
            *sleep_vp = (val & module_cfg_p->dsleep_v) >> module_cfg_p->dsleep_v_shift;
        }
    }

    return ret;
}

static POSSIBLY_UNUSED void pmu_dcdc_ana_get_volt(unsigned short *normal_vp, unsigned short *dsleep_vp)
{
    unsigned short val;

    pmu_read(PMU_REG_BUCK_VOLT, &val);
    if (normal_vp) {
        *normal_vp = GET_BITFIELD(val, PMU_BUCK_VANA_NORMAL);
    }
    if (dsleep_vp) {
        *dsleep_vp = GET_BITFIELD(val, PMU_BUCK_VANA_DSLEEP);
    }
}

void pmu_dcdc_ana_set_volt(unsigned short normal_v,unsigned short dsleep_v)
{
    int ret;
    unsigned short val;

#ifdef PMU_DCDC_ANA2_SUPPORT
    pmu_read(PMU_REG_BUCK_VCOMP, &val);
    if (normal_v & PMU_DCDC_ANA2_FLAG) {
        normal_v &= ~PMU_DCDC_ANA2_FLAG;
        val |= PMU_BUCK_HV_ANA_NORMAL;
    } else {
        val &= ~PMU_BUCK_HV_ANA_NORMAL;
    }
    if (dsleep_v & PMU_DCDC_ANA2_FLAG) {
        dsleep_v &= ~PMU_DCDC_ANA2_FLAG;
        val |= PMU_BUCK_HV_ANA_DSLEEP;
    } else {
        val &= ~PMU_BUCK_HV_ANA_DSLEEP;
    }
    pmu_write(PMU_REG_BUCK_VCOMP, val);
#endif

    ret = pmu_read(PMU_REG_BUCK_VOLT, &val);
    if(ret == 0)
    {
        val &= ~PMU_BUCK_VANA_DSLEEP_MASK;
        val &= ~PMU_BUCK_VANA_NORMAL_MASK;
        val |= PMU_BUCK_VANA_DSLEEP(dsleep_v);
        val |= PMU_BUCK_VANA_NORMAL(normal_v);
        pmu_write(PMU_REG_BUCK_VOLT, val);
    }
}

static void pmu_ana_set_volt(int mode_change, enum PMU_POWER_MODE_T mode)
{
    if (mode == PMU_POWER_MODE_ANA_DCDC || mode == PMU_POWER_MODE_DIG_DCDC) {
#ifdef PMU_DCDC_ANA2_SUPPORT
        pmu_dcdc_ana_set_volt(ana_act_dcdc, ana_lp_dcdc);
        hal_sys_timer_delay_us(PMU_VANA_STABLE_TIME_US);
#else
        uint16_t old_dcdc_volt;

        pmu_dcdc_ana_get_volt(&old_dcdc_volt, NULL);
        if (old_dcdc_volt < ana_act_dcdc) {
            while (old_dcdc_volt++ < ana_act_dcdc) {
                pmu_dcdc_ana_set_volt(old_dcdc_volt, ana_lp_dcdc);
            }
            hal_sys_timer_delay_us(PMU_VANA_STABLE_TIME_US);
        } else if (old_dcdc_volt > ana_act_dcdc) {
            pmu_dcdc_ana_set_volt(ana_act_dcdc, ana_lp_dcdc);
        }
#endif
    }
}

static void pmu_dcdc_dig_get_volt(unsigned short *normal_vp, unsigned short *dsleep_vp)
{
    unsigned short val;

    pmu_read(PMU_REG_BUCK_VOLT, &val);
    if (normal_vp) {
        *normal_vp = GET_BITFIELD(val, PMU_BUCK_VCORE_NORMAL);
    }
    if (dsleep_vp) {
        *dsleep_vp = GET_BITFIELD(val, PMU_BUCK_VCORE_DSLEEP);
    }
}

static void pmu_dcdc_dig_set_volt(unsigned short normal_v,unsigned short dsleep_v)
{
    int ret;
    unsigned short val;

    ret = pmu_read(PMU_REG_BUCK_VOLT, &val);
    if(ret == 0)
    {
        val &= ~PMU_BUCK_VCORE_NORMAL_MASK;
        val &= ~PMU_BUCK_VCORE_DSLEEP_MASK;
        val |= PMU_BUCK_VCORE_NORMAL(normal_v);
        val |= PMU_BUCK_VCORE_DSLEEP(dsleep_v);
        pmu_write(PMU_REG_BUCK_VOLT, val);
    }
}

static void BOOT_TEXT_SRAM_LOC pmu_dig_get_target_volt(uint16_t *ldo, uint16_t *dcdc)
{
    uint16_t ldo_volt;
    uint16_t dcdc_volt;

    if (0) {
    } else if (pmu_vcore_req & (PMU_VCORE_IIR_FREQ_HIGH | PMU_VCORE_USB_ENABLED)) {
#ifdef RELIABLITY_TEST
        ldo_volt = PMU_VDIG_1_2V;
        dcdc_volt = PMU_DCDC_DIG_1_2V;
#else
        ldo_volt = PMU_VDIG_1_1V;
        dcdc_volt = PMU_DCDC_DIG_1_1V;
#endif
    } else if (pmu_vcore_req & (PMU_VCORE_FLASH_WRITE_ENABLED | PMU_VCORE_FLASH_FREQ_HIGH |
            PMU_VCORE_IIR_FREQ_MEDIUM)) {
        ldo_volt = PMU_VDIG_1_1V;
        dcdc_volt = PMU_DCDC_DIG_1_1V;
    } else if (pmu_vcore_req & (PMU_VCORE_FLASH_FREQ_MEDIUM | PMU_VCORE_PSRAM_FREQ_HIGH |
            PMU_VCORE_SYS_FREQ_HIGH)) {
        ldo_volt = PMU_VDIG_1_0V;
        dcdc_volt = PMU_DCDC_DIG_1_0V;
    } else {
        // Common cases

        // CAUTION:
        // 1) Some chips cannot work when vcore is below 1.0V
        // 2) Vcore DCDC -> LDO, or LDO 0.95V -> 0.9V, might occasionaly cause a negative
        //    glitch (about 50mV) . Flash controller is sensitive to vcore, and might not
        //    work below 0.9V.
        ldo_volt = PMU_VDIG_1_0V;
        dcdc_volt = PMU_DCDC_DIG_1_0V;
    }

#ifdef PROGRAMMER
    // Try to keep the same vcore voltage as ROM (hardware default)
    if (ldo_volt < PMU_VDIG_1_1V) {
        ldo_volt = PMU_VDIG_1_1V;
    }
    if (dcdc_volt < PMU_DCDC_DIG_1_1V) {
        dcdc_volt = PMU_DCDC_DIG_1_1V;
    }
#endif

    if (ldo) {
        *ldo = ldo_volt;
    }
    if (dcdc) {
        *dcdc = dcdc_volt;
    }
}

static void pmu_dig_set_volt(int mode_change, enum PMU_POWER_MODE_T mode)
{
    uint32_t lock;
    uint16_t dcdc_volt;

    lock = int_lock();

    if (mode == PMU_POWER_MODE_NONE) {
        mode = pmu_power_mode;
    }

    pmu_dig_get_target_volt(&dig_act_ldo, &dcdc_volt);

    uint16_t old_dcdc_volt;
    uint16_t old_ldo_volt;
    bool volt_inc = false;

    pmu_module_get_volt(PMU_DIG, NULL, &old_ldo_volt);
    pmu_dcdc_dig_get_volt(&old_dcdc_volt, NULL);

    if (mode_change || mode == PMU_POWER_MODE_DIG_DCDC) {
        if (old_dcdc_volt < dcdc_volt) {
            volt_inc = true;
            while (old_dcdc_volt++ < dcdc_volt) {
                pmu_dcdc_dig_set_volt(old_dcdc_volt, dig_lp_dcdc);
            }
        } else if (old_dcdc_volt > dcdc_volt) {
            pmu_dcdc_dig_set_volt(dcdc_volt, dig_lp_dcdc);
        }
    }

    if (mode_change || mode != PMU_POWER_MODE_DIG_DCDC) {
        if (old_ldo_volt < dig_act_ldo) {
            volt_inc = true;
            while (old_ldo_volt++ < dig_act_ldo) {
                pmu_module_set_volt(PMU_DIG, dig_lp_ldo, old_ldo_volt);
            }
        } else if (old_ldo_volt > dig_act_ldo) {
            pmu_module_set_volt(PMU_DIG, dig_lp_ldo, dig_act_ldo);
        }
    }

    if (mode_change == 0) {
        // Update the voltage of the other mode
        if (mode == PMU_POWER_MODE_DIG_DCDC) {
            pmu_module_set_volt(PMU_DIG, dig_lp_ldo, dig_act_ldo);
        } else {
            pmu_dcdc_dig_set_volt(dcdc_volt, dig_lp_dcdc);
        }
    }

    if (volt_inc) {
        hal_sys_timer_delay_us(PMU_VCORE_STABLE_TIME_US);
    }

    int_unlock(lock);
}

static void pmu_ldo_mode_en(void)
{
    unsigned short val;

    // Enable vana ldo
    pmu_module_config(PMU_ANA,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);
    // Enable vcore ldo
    pmu_module_config(PMU_DIG,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);

    pmu_ana_set_volt(1, PMU_POWER_MODE_LDO);
    pmu_dig_set_volt(1, PMU_POWER_MODE_LDO);

    hal_sys_timer_delay_us(PMU_LDO_PU_STABLE_TIME_US);

    // Disable vana dcdc and vcore dcdc
    pmu_read(PMU_REG_DCDC_CFG, &val);
    val = val & PMU_TEST_MODE_MASK;
    pmu_write(PMU_REG_DCDC_CFG, val);
}

static void pmu_dcdc_ana_mode_en(void)
{
    unsigned short val;

    if (pmu_power_mode == PMU_POWER_MODE_DIG_DCDC) {
        // Enable vcore ldo
        pmu_module_config(PMU_DIG,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON);

        pmu_ana_set_volt(1, PMU_POWER_MODE_ANA_DCDC);
        pmu_dig_set_volt(1, PMU_POWER_MODE_ANA_DCDC);

        hal_sys_timer_delay_us(PMU_LDO_PU_STABLE_TIME_US);
    }

    // Enable vana dcdc & disable vcore dcdc
    pmu_read(PMU_REG_DCDC_CFG, &val);
    val |= PMU_DCDC_ANA_LP_MODE_EN | PMU_DCDC_ANA_DSLEEP_ON;
    val |= PMU_DCDC_ANA_DR | PMU_DCDC_ANA_EN;
    val &= ~(PMU_BUCK_CC_MODE_EN | PMU_DCDC_DIG_DR | PMU_DCDC_DIG_EN);
    pmu_write(PMU_REG_DCDC_CFG, val);

    if (pmu_power_mode != PMU_POWER_MODE_DIG_DCDC) {
        pmu_ana_set_volt(1, PMU_POWER_MODE_ANA_DCDC);
        pmu_dig_set_volt(1, PMU_POWER_MODE_ANA_DCDC);

        hal_sys_timer_delay_us(PMU_DCDC_PU_STABLE_TIME_US);

        // Disable vana ldo
        pmu_module_config(PMU_ANA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
    }
}

static void pmu_dcdc_dual_mode_en(void)
{
    unsigned short val;

    // Enable vana dcdc and vcore dcdc
    pmu_read(PMU_REG_DCDC_CFG, &val);
    val |= PMU_BUCK_CC_MODE_EN;
    val |= PMU_DCDC_DIG_DR | PMU_DCDC_DIG_EN;
    val |= PMU_DCDC_ANA_DR | PMU_DCDC_ANA_EN;
    // Note that vcore dcdc has no lp mode,
    // so the power mode must be changed to ANA_DCDC before sleep
    val &= ~(PMU_DCDC_ANA_LP_MODE_EN | PMU_DCDC_ANA_DSLEEP_ON);
    pmu_write(PMU_REG_DCDC_CFG, val);

    pmu_ana_set_volt(1, PMU_POWER_MODE_DIG_DCDC);
    pmu_dig_set_volt(1, PMU_POWER_MODE_DIG_DCDC);

    hal_sys_timer_delay_us(PMU_DCDC_PU_STABLE_TIME_US);

    // Disable vana ldo
    pmu_module_config(PMU_ANA,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
    // Disable vcore ldo
    pmu_module_config(PMU_DIG,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
}

void pmu_mode_change(enum PMU_POWER_MODE_T mode)
{
    uint32_t lock;
    bool update_mode;

    if (pmu_power_mode == mode)
        return;

    update_mode = true;

    lock = int_lock();

    if (mode == PMU_POWER_MODE_DIG_DCDC) {
        pmu_dcdc_dual_mode_en();
    } else if (mode == PMU_POWER_MODE_ANA_DCDC) {
        pmu_dcdc_ana_mode_en();
    } else if (mode == PMU_POWER_MODE_LDO) {
        pmu_ldo_mode_en();
    } else {
        update_mode = false;
    }

    if (update_mode) {
        pmu_power_mode = mode;
    }

    int_unlock(lock);
}

#ifdef SS_SPECIFIC
static uint8_t s_fm_mode;
uint8_t ss_fm_mode_switch (uint32_t fm_mode)
{
	if (fm_mode != s_fm_mode) {
		pmu_mode_change(fm_mode ? PMU_POWER_MODE_LDO : PMU_POWER_MODE_DIG_DCDC);
		s_fm_mode = fm_mode;
	}
}
#endif

void pmu_sleep_en(unsigned char sleep_en)
{
    int ret;
    unsigned short val;

    ret = pmu_read(PMU_REG_SLEEP_CFG, &val);
    if(ret == 0)
    {
        if(sleep_en)
        {
            val |= PMU_SLEEP_EN;
        }
        else
        {
            val &= ~PMU_SLEEP_EN;
        }
        pmu_write(PMU_REG_SLEEP_CFG, val);
    }
}

#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
static uint32_t pmu_vcodec_mv_to_val(uint16_t mv)
{
    return (mv == 2500 || mv == 3500) ? PMU_CODEC_2_5V : PMU_CODEC_1_8V;
}

static void BOOT_TEXT_FLASH_LOC pmu_dig_init_volt(void)
{
    uint16_t ldo_volt;
    uint16_t val;

    pmu_dig_get_target_volt(&ldo_volt, NULL);

    pmu_read(PMU_REG_DIG_CFG, &val);
    if (GET_BITFIELD(val, PMU_DIG_NORMAL_VOLT) < ldo_volt) {
        val = SET_BITFIELD(val, PMU_DIG_NORMAL_VOLT, ldo_volt);
        pmu_write(PMU_REG_DIG_CFG, val);
    }
}
#endif

int BOOT_TEXT_FLASH_LOC pmu_open(void)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
    int ret;
    uint16_t val;
    enum PMU_POWER_MODE_T mode;
    //enum HAL_CHIP_METAL_ID_T metal_id;

    ASSERT(vcodec_mv >= 2500 || vcodec_off, "Invalid vcodec cfg: vcodec_mv=%u vcodec_off=%d", vcodec_mv, vcodec_off);
    ASSERT(vcodec_mv >= 2500 ||
            (vcodec_mv == 1500 && ana_act_dcdc == PMU_DCDC_ANA_1_5V) ||
            (vcodec_mv == 1600 && ana_act_dcdc == PMU_DCDC_ANA_1_6V) ||
            (vcodec_mv == 1800 && ana_act_dcdc == PMU_DCDC_ANA_1_8V),
        "Invalid vcodec/vana cfg: vcodec_mv=%u ana_act_dcdc=%u", vcodec_mv, ana_act_dcdc);

    //metal_id = hal_get_chip_metal_id();

    // Disable all PMU irqs by default
    pmu_write(PMU_REG_INT_MASK, 0);
    pmu_write(PMU_REG_INT_EN, 0);

    // Allow PMU to sleep when power key is pressed
    ret = pmu_read(PMU_REG_POWER_KEY_CFG, &val);
    if (ret == 0) {
        val &= ~PMU_POWERKEY_WAKEUP_OSC_EN;
        pmu_write(PMU_REG_POWER_KEY_CFG, val);
    }

    // Init DCDC settings
    pmu_write(PMU_REG_BIAS_CFG, 0x3155);
    pmu_write(PMU_REG_BUCK_FREQ, 0xD800);

#ifndef NO_SLEEP
    pmu_sleep_en(1);  //enable sleep
#endif

    if (vcrystal_off) {
        pmu_read(PMU_REG_CRYSTAL_CFG, &val);
        val |= PMU_CRYSTAL_LP_EN_DR | PMU_CRYSTAL_LP_EN_REG;
        pmu_write(PMU_REG_CRYSTAL_CFG, val);

        if (1) {
            pmu_module_config(PMU_CRYSTAL,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable 26m ldo (short with vana)
        } else {
            pmu_module_config(PMU_CRYSTAL,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable 26m ldo in sleep
            pmu_module_set_volt(PMU_CRYSTAL, 0, 0);
        }
    } else {
        pmu_module_config(PMU_CRYSTAL,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable 26m ldo in sleep
    }

    pmu_module_config(PMU_GP,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VGP

#if !defined(PROGRAMMER) && !defined(VUSB_ON)
    //pmu_module_config(PMU_USB,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VUSB
#endif
    // USB PLL needs USB LDO on
    pmu_module_config(PMU_USB,PMU_MANUAL_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //enable VUSB

#ifdef __BEST_FLASH_VIA_ANA__
    pmu_module_config(PMU_IO,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF); //disable VIO in sleep
#else
    pmu_module_config(PMU_IO,PMU_AUTO_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_ON); //enable VIO in sleep
#endif
    pmu_module_set_volt(PMU_IO, vio_lp, vio_act_normal);

    pmu_module_config(PMU_CODEC, PMU_MANUAL_MODE, PMU_LDO_OFF, PMU_LP_MODE_ON, PMU_DSLEEP_MODE_OFF); //disable VCODEC

    val = pmu_vcodec_mv_to_val(vcodec_mv);
    pmu_module_set_volt(PMU_CODEC, val, val);

    // Init dig_act_ldo
    pmu_dig_set_volt(0, PMU_POWER_MODE_LDO);

#ifdef DIG_DCDC_MODE
    mode = PMU_POWER_MODE_DIG_DCDC;
#elif defined(ANA_DCDC_MODE)
    mode = PMU_POWER_MODE_ANA_DCDC;
#else // LDO_MODE
    mode = PMU_POWER_MODE_LDO;
#endif

    pmu_mode_change(mode);

#ifdef RTC_ENABLE
    pmu_rtc_restore_context();
#endif

#endif // PMU_INIT || (!FPGA && !PROGRAMMER)

    return 0;
}

void pmu_sleep(void)
{
    if (pmu_power_mode == PMU_POWER_MODE_DIG_DCDC) {
        pmu_active_power_mode = pmu_power_mode;
        pmu_mode_change(PMU_POWER_MODE_ANA_DCDC);
    }
}

void pmu_wakeup(void)
{
    if (pmu_active_power_mode != PMU_POWER_MODE_NONE) {
        pmu_mode_change(pmu_active_power_mode);
        pmu_active_power_mode = PMU_POWER_MODE_NONE;
    }
}

void pmu_codec_config(int enable)
{
    if (!vcodec_off) {
        if (enable) {
            pmu_module_config(PMU_CODEC,PMU_MANUAL_MODE,PMU_LDO_ON,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
        } else {
            pmu_module_config(PMU_CODEC,PMU_MANUAL_MODE,PMU_LDO_OFF,PMU_LP_MODE_ON,PMU_DSLEEP_MODE_OFF);
        }
    }
}

void pmu_codec_mic_bias_enable(uint32_t map)
{
    uint16_t val_a, val_b;

    // Increase LDO_RES if higher vmic is required
#ifdef DIGMIC_HIGH_VOLT
    val_a = PMU_MIC_LDO_RES(0xF);
#else
    val_a = PMU_MIC_LDO_RES(7);
#endif
    val_b = 0;
    if (map & AUD_VMIC_MAP_VMIC1) {
        val_a |= PMU_MIC_BIASA_EN | PMU_MIC_BIASA_ENLPF | PMU_MIC_BIASA_VSEL(0xF);
        val_b |= PMU_MIC_LDO_EN;
    }
    if (map & AUD_VMIC_MAP_VMIC2) {
        val_b |= PMU_MIC_BIASB_EN | PMU_MIC_BIASB_ENLPF | PMU_MIC_BIASB_VSEL(0xF) | PMU_MIC_LDO_EN;
    }
    pmu_write(PMU_REG_MIC_BIAS_A, val_a);
    pmu_write(PMU_REG_MIC_BIAS_B, val_b);
}

void pmu_codec_adc_pre_start(void)
{
}

void pmu_codec_adc_post_start(void (*delay_ms)(uint32_t))
{
}

void pmu_codec_dac_pre_start(void)
{
}

void pmu_codec_dac_post_start(void (*delay_ms)(uint32_t))
{
}

void pmu_flash_write_config(void)
{
#ifdef FLASH_WRITE_AT_HIGH_VCORE
    uint32_t lock;

    if (pmu_vcore_req & PMU_VCORE_FLASH_WRITE_ENABLED) {
        return;
    }

    lock = int_lock();
    pmu_vcore_req |= PMU_VCORE_FLASH_WRITE_ENABLED;
    int_unlock(lock);

    pmu_dig_set_volt(0, PMU_POWER_MODE_NONE);
#endif
}

void pmu_flash_read_config(void)
{
#ifdef FLASH_WRITE_AT_HIGH_VCORE
    uint32_t lock;

    if ((pmu_vcore_req & PMU_VCORE_FLASH_WRITE_ENABLED) == 0) {
        return;
    }

    lock = int_lock();
    pmu_vcore_req &= ~PMU_VCORE_FLASH_WRITE_ENABLED;
    int_unlock(lock);

    pmu_dig_set_volt(0, PMU_POWER_MODE_NONE);
#endif
}

void BOOT_TEXT_FLASH_LOC pmu_flash_freq_config(uint32_t freq)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
    uint32_t lock;

    lock = int_lock();
    pmu_vcore_req &= ~(PMU_VCORE_FLASH_FREQ_HIGH | PMU_VCORE_FLASH_FREQ_MEDIUM);
#ifndef OSC_26M_X4_AUD2BB
    if (freq > 52000000) {
#ifdef USB_HIGH_SPEED
        // The real max freq is 120M
        pmu_vcore_req |= PMU_VCORE_FLASH_FREQ_HIGH;
#else
        // The real max freq is about 90~100M. It seems to work at VCORE 0.9V.
        //pmu_vcore_req |= PMU_VCORE_FLASH_FREQ_MEDIUM;
#endif
    }
#endif
    int_unlock(lock);

    if (pmu_power_mode == PMU_POWER_MODE_NONE) {
        // PMU not init yet
        pmu_dig_init_volt();
        return;
    }

    pmu_dig_set_volt(0, PMU_POWER_MODE_NONE);
#endif
}

void BOOT_TEXT_FLASH_LOC pmu_psram_freq_config(uint32_t freq)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
    uint32_t lock;

    lock = int_lock();
    if (freq > 52000000) {
        pmu_vcore_req |= PMU_VCORE_PSRAM_FREQ_HIGH;
    } else {
        pmu_vcore_req &= ~PMU_VCORE_PSRAM_FREQ_HIGH;
    }
    int_unlock(lock);

    if (pmu_power_mode == PMU_POWER_MODE_NONE) {
        // PMU not init yet
        pmu_dig_init_volt();
        return;
    }

    pmu_dig_set_volt(0, PMU_POWER_MODE_NONE);
#endif
}

void pmu_anc_config(int enable)
{
}

void pmu_fir_high_speed_config(int enable)
{
}

void pmu_iir_freq_config(uint32_t freq)
{
    uint32_t lock;

    lock = int_lock();
    pmu_vcore_req &= ~(PMU_VCORE_IIR_FREQ_HIGH | PMU_VCORE_IIR_FREQ_MEDIUM);
    if (freq >= 52000000) {
        pmu_vcore_req |= PMU_VCORE_IIR_FREQ_HIGH;
    } else if (freq > 26000000) {
        pmu_vcore_req |= PMU_VCORE_IIR_FREQ_MEDIUM;
    }
    int_unlock(lock);

    pmu_dig_set_volt(0, PMU_POWER_MODE_NONE);
}

void BOOT_TEXT_SRAM_LOC pmu_sys_freq_config(enum HAL_CMU_FREQ_T freq)
{
#if defined(PMU_INIT) || (!defined(FPGA) && !defined(PROGRAMMER))
#ifndef OSC_26M_X4_AUD2BB
    uint32_t lock;
    enum PMU_VCORE_REQ_T old_req;
    bool update = false;

    lock = int_lock();
    old_req = pmu_vcore_req;
    if (freq > HAL_CMU_FREQ_104M) {
        pmu_vcore_req |= PMU_VCORE_SYS_FREQ_HIGH;
    } else {
        pmu_vcore_req &= ~PMU_VCORE_SYS_FREQ_HIGH;
    }
    if (old_req != pmu_vcore_req) {
        update = true;
    }
    int_unlock(lock);

    if (!update) {
        // Nothing changes
        return;
    }

    if (pmu_power_mode == PMU_POWER_MODE_NONE) {
        // PMU not init yet
        pmu_dig_init_volt();
        return;
    }

    pmu_dig_set_volt(0, PMU_POWER_MODE_NONE);
#endif
#endif
}

void pmu_usb_config(enum PMU_USB_CONFIG_TYPE_T type)
{
    unsigned short ldo_on, deep_sleep_on;
#ifdef USB_HIGH_SPEED
    uint16_t val;
    uint32_t lock;
#endif

    if (type == PMU_USB_CONFIG_TYPE_NONE) {
#ifdef USB_HIGH_SPEED
        pmu_read(PMU_REG_USBPHY, &val);
        val = (val & ~PMU_RESETN_USBPHY_REG) | PMU_RESETN_USBPHY_DR;
        pmu_write(PMU_REG_USBPHY, val);
#endif

        ldo_on = PMU_LDO_OFF;
        deep_sleep_on = PMU_DSLEEP_MODE_OFF;
    } else {
#ifdef USB_HIGH_SPEED
        pmu_read(PMU_REG_USBPHY, &val);
        val |= PMU_EN_USBDIGPHY_CLK;
        pmu_write(PMU_REG_USBPHY, val);
#endif

        ldo_on = PMU_LDO_ON;
        deep_sleep_on = PMU_DSLEEP_MODE_ON;
    }

    pmu_module_config(PMU_USB, PMU_MANUAL_MODE, ldo_on, PMU_LP_MODE_ON, deep_sleep_on);

#ifdef USB_HIGH_SPEED
    lock = int_lock();
    if (type == PMU_USB_CONFIG_TYPE_NONE) {
        pmu_vcore_req &= ~PMU_VCORE_USB_ENABLED;
    } else {
        pmu_vcore_req |= PMU_VCORE_USB_ENABLED;
    }
    int_unlock(lock);
#endif

    pmu_dig_set_volt(0, PMU_POWER_MODE_NONE);
}

static int pmu_usb_check_pin_status(enum PMU_USB_PIN_CHK_STATUS_T status)
{
    int dp, dm;

    pmu_usb_get_pin_status(&dp, &dm);

    //TRACE("[%X] %s: status=%d dp=%d dm=%d", hal_sys_timer_get(), __FUNCTION__, status, dp, dm);

    // HOST_RESUME: (resume) dp == 0 && dm == 1, (reset) dp == 0 && dm == 0

    if ( (status == PMU_USB_PIN_CHK_DEV_CONN && (dp == 1 && dm == 0)) ||
            (status == PMU_USB_PIN_CHK_DEV_DISCONN && (dp == 0 && dm == 0)) ||
            (status == PMU_USB_PIN_CHK_HOST_RESUME && dp == 0) ) {
        return 1;
    }

    return 0;
}

static void pmu_usb_pin_irq_handler(void)
{
    //TRACE("[%X] %s", hal_sys_timer_get(), __FUNCTION__);

    pmu_write(PMU_REG_INT_CLR, PMU_INT_CLR_USB_INSERT);

    if (usb_pin_callback) {
        if (pmu_usb_check_pin_status(usb_pin_status)) {
            pmu_usb_disable_pin_status_check();
            usb_pin_callback(usb_pin_status);
        }
    }
}

int pmu_usb_config_pin_status_check(enum PMU_USB_PIN_CHK_STATUS_T status, PMU_USB_PIN_CHK_CALLBACK callback, int enable)
{
    uint16_t val;
    uint32_t lock;

    //TRACE("[%X] %s: status=%d", hal_sys_timer_get(), __FUNCTION__, status);

    if (status >= PMU_USB_PIN_CHK_STATUS_QTY) {
        return 1;
    }

    NVIC_DisableIRQ(USB_PIN_IRQn);

    lock = int_lock();

    usb_pin_status = status;
    usb_pin_callback = callback;

    // Mask the irq
    pmu_read(PMU_REG_INT_MASK, &val);
    val &= ~PMU_INT_MASK_USB_INSERT;
    pmu_write(PMU_REG_INT_MASK, val);

    // Config pin check
    pmu_read(PMU_REG_USB_PIN_CFG, &val);
    val |= PMU_USB_DEBOUNCE_EN | PMU_USB_NOLS_MODE | PMU_USB_INSERT_DET_EN;
    pmu_write(PMU_REG_USB_PIN_CFG, val);

    pmu_read(PMU_REG_USB_PIN_POL, &val);
    val &= ~(PMU_USB_POL_RX_DP | PMU_USB_POL_RX_DM);
    if (status == PMU_USB_PIN_CHK_DEV_CONN) {
        // Check dp 0->1, dm x->0
    } else if (status == PMU_USB_PIN_CHK_DEV_DISCONN) {
        // Check dp 1->0, dm x->0
        val |= PMU_USB_POL_RX_DP;
    } else if (status == PMU_USB_PIN_CHK_HOST_RESUME) {
        // Check dp 1->0, dm 0->1 (resume) or dm 0->0 (reset)
        val |= PMU_USB_POL_RX_DP;
    }
    pmu_write(PMU_REG_USB_PIN_POL, val);

    if (status != PMU_USB_PIN_CHK_NONE && callback) {
        pmu_read(PMU_REG_INT_EN, &val);
        val |= PMU_INT_EN_USB_INSERT;
        pmu_write(PMU_REG_INT_EN, val);

        pmu_read(PMU_REG_INT_MASK, &val);
        val |= PMU_INT_MASK_USB_INSERT;
        pmu_write(PMU_REG_INT_MASK, val);

        pmu_write(PMU_REG_INT_CLR, PMU_INT_CLR_USB_INSERT);
    }

    int_unlock(lock);

    if (enable) {
        // Wait at least 10 cycles of 32K clock for the new status when signal checking polarity is changed
        hal_sys_timer_delay(5);
        pmu_usb_enable_pin_status_check();
    }

    return 0;
}

void pmu_usb_enable_pin_status_check(void)
{
    if (usb_pin_status != PMU_USB_PIN_CHK_NONE && usb_pin_callback) {
        pmu_write(PMU_REG_INT_CLR, PMU_INT_CLR_USB_INSERT);
        NVIC_ClearPendingIRQ(USB_PIN_IRQn);

        if (pmu_usb_check_pin_status(usb_pin_status)) {
            pmu_usb_disable_pin_status_check();
            usb_pin_callback(usb_pin_status);
            return;
        }

        NVIC_SetVector(USB_PIN_IRQn, (uint32_t)pmu_usb_pin_irq_handler);
        NVIC_SetPriority(USB_PIN_IRQn, IRQ_PRIORITY_NORMAL);
        NVIC_EnableIRQ(USB_PIN_IRQn);
    }
}

void pmu_usb_disable_pin_status_check(void)
{
    uint16_t val;
    uint32_t lock;

    NVIC_DisableIRQ(USB_PIN_IRQn);

    lock = int_lock();

    pmu_read(PMU_REG_INT_EN, &val);
    val &= ~PMU_INT_EN_USB_INSERT;
    pmu_write(PMU_REG_INT_EN, val);

    pmu_read(PMU_REG_USB_PIN_CFG, &val);
    val &= ~PMU_USB_INSERT_DET_EN;
    pmu_write(PMU_REG_USB_PIN_CFG, val);

    int_unlock(lock);
}

void pmu_usb_get_pin_status(int *dp, int *dm)
{
    uint16_t pol, val;

    pmu_read(PMU_REG_USB_PIN_POL, &pol);
    pmu_read(PMU_REG_USB_PIN_STATUS, &val);

    *dp = (!(pol & PMU_USB_POL_RX_DP)) ^ (!(val & PMU_USB_STATUS_RX_DP));
    *dm = (!(pol & PMU_USB_POL_RX_DM)) ^ (!(val & PMU_USB_STATUS_RX_DM));
}

void pmu_charger_init(void)
{
    unsigned short readval_cfg;
    uint32_t lock;

    lock = int_lock();
    pmu_read(PMU_REG_CHARGER_CFG, &readval_cfg);
    readval_cfg &= ~(PMU_CHARGE_IN_INTR_MSK | PMU_CHARGE_OUT_INTR_MSK |
        PMU_AC_ON_OUT_EN | PMU_AC_ON_IN_EN | PMU_CHARGE_INTR_EN);
    pmu_write(PMU_REG_CHARGER_CFG ,readval_cfg);
    int_unlock(lock);

    hal_sys_timer_delay(MS_TO_TICKS(1));

    lock = int_lock();
    pmu_read(PMU_REG_CHARGER_CFG, &readval_cfg);
    readval_cfg |= PMU_AC_ON_OUT_EN | PMU_AC_ON_IN_EN | PMU_CHARGE_INTR_EN;
    readval_cfg = SET_BITFIELD(readval_cfg, PMU_AC_ON_DB_VALUE, 8);
    pmu_write(PMU_REG_CHARGER_CFG ,readval_cfg);
    int_unlock(lock);
}

static void pmu_charger_irq_handler(void)
{
    enum PMU_CHARGER_STATUS_T status = PMU_CHARGER_UNKNOWN;
    unsigned short readval;
    uint32_t lock;

    lock = int_lock();
    pmu_read(PMU_REG_CHARGER_STATUS, &readval);
    pmu_write(PMU_REG_CHARGER_STATUS ,readval);
    int_unlock(lock);
    TRACE("%s REG_%02X=0x%04X", __func__, PMU_REG_CHARGER_STATUS, readval);

    if ((readval & (PMU_AC_ON_DET_IN_MASKED | PMU_AC_ON_DET_OUT_MASKED)) == 0){
        TRACE("%s SKIP", __func__);
        return;
    } else if ((readval & (PMU_AC_ON_DET_IN_MASKED | PMU_AC_ON_DET_OUT_MASKED)) ==
            (PMU_AC_ON_DET_IN_MASKED | PMU_AC_ON_DET_OUT_MASKED)) {
        TRACE("%s DITHERING", __func__);
        hal_sys_timer_delay(2);
    } else {
        TRACE("%s NORMAL", __func__);
    }

    status = pmu_charger_get_status();

    if (charger_irq_handler) {
        charger_irq_handler(status);
    }
}

void pmu_charger_set_irq_handler(PMU_CHARGER_IRQ_HANDLER_T handler)
{
    uint32_t lock;
    uint16_t val;

    charger_irq_handler = handler;

    lock = int_lock();
    pmu_read(PMU_REG_CHARGER_CFG, &val);
    val |= PMU_CHARGE_IN_INTR_MSK | PMU_CHARGE_OUT_INTR_MSK;
    pmu_write(PMU_REG_CHARGER_CFG, val);
    int_unlock(lock);

    if (handler) {
        NVIC_SetVector(CHARGER_IRQn, (uint32_t)pmu_charger_irq_handler);
        NVIC_SetPriority(CHARGER_IRQn, IRQ_PRIORITY_NORMAL);
        NVIC_ClearPendingIRQ(CHARGER_IRQn);
        NVIC_EnableIRQ(CHARGER_IRQn);
    } else {
        NVIC_DisableIRQ(CHARGER_IRQn);
    }
}

void pmu_charger_plugin_config(void)
{
    if (IO_VOLT_ACTIVE_RISE < PMU_IO_3_1V) {
        vio_act_rise = PMU_IO_3_1V;
    }
    if (IO_VOLT_SLEEP < PMU_IO_3_2V) {
        vio_lp = PMU_IO_3_2V;
    }
    pmu_viorise_req(PMU_VIORISE_REQ_USER_CHARGER, true);
}

void pmu_charger_plugout_config(void)
{
    vio_act_rise = IO_VOLT_ACTIVE_RISE;
    vio_lp = IO_VOLT_SLEEP;
    pmu_viorise_req(PMU_VIORISE_REQ_USER_CHARGER, false);
}

enum PMU_CHARGER_STATUS_T pmu_charger_get_status(void)
{
    unsigned short readval;
    enum PMU_CHARGER_STATUS_T status;

    pmu_read(PMU_REG_CHARGER_STATUS, &readval);
    if (readval & PMU_AC_ON)
        status = PMU_CHARGER_PLUGIN;
    else
        status = PMU_CHARGER_PLUGOUT;

    return status;
}

#ifdef RTC_ENABLE
void pmu_rtc_enable(void)
{
    uint16_t readval;
    uint32_t lock;

#ifdef SIMU
    // Set RTC counter to 1KHz
    pmu_write(PMU_REG_RTC_DIV_1HZ, 32 - 2);
#else
    // Set RTC counter to 1Hz
    pmu_write(PMU_REG_RTC_DIV_1HZ, 32000 - 2);
#endif

    lock = int_lock();
    pmu_read(PMU_REG_POWER_KEY_CFG, &readval);
    readval |= PMU_RTC_POWER_ON_EN | PMU_PU_LPO_DR | PMU_PU_LPO_REG;
    pmu_write(PMU_REG_POWER_KEY_CFG, readval);
    int_unlock(lock);
}

void pmu_rtc_disable(void)
{
    uint16_t readval;
    uint32_t lock;

    pmu_rtc_clear_alarm();

    lock = int_lock();
    pmu_read(PMU_REG_POWER_KEY_CFG, &readval);
    readval &= ~(PMU_RTC_POWER_ON_EN | PMU_PU_LPO_DR);
    pmu_write(PMU_REG_POWER_KEY_CFG, readval);
    int_unlock(lock);
}

int BOOT_TEXT_SRAM_LOC pmu_rtc_enabled(void)
{
    uint16_t readval;

    pmu_read(PMU_REG_POWER_KEY_CFG, &readval);

    return !!(readval & PMU_RTC_POWER_ON_EN);
}

void pmu_rtc_set(uint32_t seconds)
{
    uint16_t high, low;

    // Need 3 seconds to load a new value
    seconds += 3;

    high = seconds >> 16;
    low = seconds & 0xFFFF;

    pmu_write(PMU_REG_RTC_LOAD_LOW, low);
    pmu_write(PMU_REG_RTC_LOAD_HIGH, high);
}

uint32_t pmu_rtc_get(void)
{
    uint16_t high, low, high2;

    pmu_read(PMU_REG_RTC_VAL_HIGH, &high);
    pmu_read(PMU_REG_RTC_VAL_LOW, &low);
    // Handle counter wrap
    pmu_read(PMU_REG_RTC_VAL_HIGH, &high2);
    if (high != high2) {
        high = high2;
        pmu_read(PMU_REG_RTC_VAL_LOW, &low);
    }

    return (high << 16) | low;
}

void pmu_rtc_set_alarm(uint32_t seconds)
{
    uint16_t readval;
    uint16_t high, low;
    uint32_t lock;

    // Need 1 second to raise the interrupt
    if (seconds > 0) {
        seconds -= 1;
    }

    high = seconds >> 16;
    low = seconds & 0xFFFF;

    pmu_write(PMU_REG_INT_CLR, PMU_INT_CLR_RTC1);

    pmu_write(PMU_REG_RTC_MATCH1_LOW, low);
    pmu_write(PMU_REG_RTC_MATCH1_HIGH, high);

    lock = int_lock();
    pmu_read(PMU_REG_INT_EN, &readval);
    readval |= PMU_INT_EN_RTC1;
    pmu_write(PMU_REG_INT_EN, readval);
    int_unlock(lock);
}

uint32_t BOOT_TEXT_SRAM_LOC pmu_rtc_get_alarm(void)
{
    uint16_t high, low;

    pmu_read(PMU_REG_RTC_MATCH1_LOW, &low);
    pmu_read(PMU_REG_RTC_MATCH1_HIGH, &high);

    // Compensate the alarm offset
    return (uint32_t)((high << 16) | low) + 1;
}

void pmu_rtc_clear_alarm(void)
{
    uint16_t readval;
    uint32_t lock;

    lock = int_lock();
    pmu_read(PMU_REG_INT_EN, &readval);
    readval &= ~PMU_INT_EN_RTC1;
    pmu_write(PMU_REG_INT_EN, readval);
    int_unlock(lock);

    pmu_write(PMU_REG_INT_CLR, PMU_INT_CLR_RTC1);
}

int BOOT_TEXT_SRAM_LOC pmu_rtc_alarm_status_set(void)
{
    uint16_t readval;

    pmu_read(PMU_REG_INT_EN, &readval);

    return !!(readval & PMU_INT_EN_RTC1);
}

int pmu_rtc_alarm_alerted()
{
    uint16_t readval;

    pmu_read(PMU_REG_INT_STATUS, &readval);

    return !!(readval & PMU_INT_STATUS_RTC1);
}

static void pmu_rtc_irq_handler(void)
{
    uint32_t seconds;

    if (pmu_rtc_alarm_alerted()) {
        pmu_rtc_clear_alarm();

        if (rtc_irq_handler) {
            seconds = pmu_rtc_get();
            rtc_irq_handler(seconds);
        }
    }
}

void pmu_rtc_set_irq_handler(PMU_RTC_IRQ_HANDLER_T handler)
{
    uint16_t readval;
    uint32_t lock;

    rtc_irq_handler = handler;

    lock = int_lock();
    pmu_read(PMU_REG_INT_MASK, &readval);
    if (handler) {
        readval |= PMU_INT_MASK_RTC1;
    } else {
        readval &= ~PMU_INT_MASK_RTC1;
    }
    pmu_write(PMU_REG_INT_MASK, readval);
    int_unlock(lock);

    if (handler) {
        NVIC_SetVector(RTC_IRQn, (uint32_t)pmu_rtc_irq_handler);
        NVIC_SetPriority(RTC_IRQn, IRQ_PRIORITY_NORMAL);
        NVIC_ClearPendingIRQ(RTC_IRQn);
        NVIC_EnableIRQ(RTC_IRQn);
    } else {
        NVIC_DisableIRQ(RTC_IRQn);
    }
}
#endif

void pmu_viorise_req(enum PMU_VIORISE_REQ_USER_T user, bool rise)
{
    uint32_t lock;

    lock = int_lock();
    if (rise) {
        if (vio_risereq_map == 0) {
            pmu_module_set_volt(PMU_IO,vio_lp,vio_act_rise);
        }
        vio_risereq_map |= (1 << user);
    } else {
        vio_risereq_map &= ~(1 << user);
        if (vio_risereq_map == 0) {
            pmu_module_set_volt(PMU_IO,vio_lp,vio_act_normal);
        }
    }
    int_unlock(lock);
}

enum PMU_POWER_ON_CAUSE_T pmu_get_power_on_cause(void)
{
    uint16_t val;

    pmu_read(0x52, &val);
    val = (val >> 4) & 0x3;
    return (enum PMU_POWER_ON_CAUSE_T)val;
}

int pmu_debug_config_ana(uint16_t volt)
{
    return 0;
}

int pmu_debug_config_codec(uint16_t volt)
{
#ifdef ANC_PROD_TEST
    if (volt == 1500 || volt == 1600 || volt == 1800 || volt == 2500 || volt == 3500) {
        vcodec_mv = volt;
    } else {
        return 1;
    }

    if (volt <= 1800) {
        if (volt == 1500) {
            ana_act_dcdc = PMU_DCDC_ANA_1_5V;
        } else if (volt == 1600) {
            ana_act_dcdc = PMU_DCDC_ANA_1_6V;
        } else if (volt == 1800) {
            ana_act_dcdc = PMU_DCDC_ANA_1_8V;
        }
        vcodec_off = true;
    } else {
        vcodec_off = false;
    }
#endif
    return 0;
}

int pmu_debug_config_vcrystal(bool on)
{
#ifdef ANC_PROD_TEST
    vcrystal_off = !on;
#endif
    return 0;
}

int pmu_debug_config_audio_output(bool diff)
{
#ifdef ANC_PROD_TEST
    audio_output_diff = diff;
#endif
    return 0;
}

void pmu_debug_reliability_test(int stage)
{
    uint16_t volt;

    if (stage == 0) {
        volt = PMU_DCDC_ANA_1_8V;
    } else {
        volt = PMU_DCDC_ANA_1_9V;
    }
    pmu_dcdc_ana_set_volt(volt, ana_lp_dcdc);
}

#if defined(CHIP_HAS_TEMP_SNSR)
static uint8_t s_temp_sensor_rdy;

uint8_t pmu_temp_sensor_ready (void)
{
	return s_temp_sensor_rdy;
}

void pmu_temp_sensor_enable (uint8_t en)
{
	uint16_t val = 0;

	TRACE("temp_snsr_enable: %d", en);

	if (en) {	
		// dig_iptat_core_en = 1;
		pmu_read(0x34, &val);
		val |= (1<<4);		
		pmu_write(0x34, val);
		
		// reg_pu_avdd25_ana = 1
		pmu_read(0x0d, &val);
		val |= (1<<1);		
		pmu_write(0x0d, val);
		
		// sar_therm_gain = 2
		pmu_read(0x17, &val);
		val &= ~(3<<3);
		val |= 2<<3; 	
		pmu_write(0x17, val);

	} else {
		pmu_read(0x0d, &val);
		val &= ~(1<<1);	// reg_pu_avdd25_ana = 0
		pmu_write(0x0d, val);

		pmu_read(0x34, &val);
		val &= ~(1<<4);	// dig_iptat_core_en = 0
		pmu_write(0x34, val);
	}
}

static uint16_t s_current_temp_code;
static uint16_t s_temp_snsr_val_rdy;

//static uint16_t s_dco_calib_code;
//static uint16_t s_temp_calib_code;

static int32_t s_temp_code_offset;
static int32_t s_dco_code_offset;
static int32_t s_dco_final_code;
void get_temp_snr_val (uint16_t irq_val, HAL_GPADC_MV_T vol)
{
	vol = vol;
	s_current_temp_code = irq_val;
	s_temp_snsr_val_rdy = 1;

	TRACE("%s: cur_teml_code[%d]", __func__, s_current_temp_code);
}

void pmu_dco_temp_calib (uint16_t* dco_code_ptr)
{
//#define TEMP_FREQ_SLOPE_X		-48125
#define TEMP_FREQ_SLOPE_X		4812500
#define TEMP_FREQ_SLOPE_Y		(181161*96)

#define TEMP_SENSOR_THRESHOLD_LOW		300
#define TEMP_SENSOR_THRESHOLD_HIGH		800

	uint16_t calib_temp_code = 0;
	int32_t temp_code_offset = 0;
	int32_t dco_code_offset = 0;

#ifdef USR_DCO_CALIB_CODE
	*dco_code_ptr = USR_DCO_CALIB_CODE;
#endif

	TRACE("%s: dco_calib_code[%d]", __func__, *dco_code_ptr);

	pmu_temp_sensor_enable(1);

	hal_gpadc_open(HAL_GPADC_CHAN_0, HAL_GPADC_ATP_ONESHOT, get_temp_snr_val);
	TRACE("-> waiting for GPADC intr, rdy[%d]", s_temp_snsr_val_rdy);
	while(s_temp_snsr_val_rdy == 0);

	if ((s_current_temp_code > TEMP_SENSOR_THRESHOLD_HIGH) || (s_current_temp_code < TEMP_SENSOR_THRESHOLD_LOW)) 
	{
		s_temp_sensor_rdy = 0;
		TRACE("-> invalid temp code");
		return;
	}
	
	pmu_temp_sensor_enable(0);
	s_temp_sensor_rdy = 1;
	
	pmu_get_efuse(0x0f, &calib_temp_code);
#ifdef USR_CALIB_TEMP_CODE
	calib_temp_code = USR_CALIB_TEMP_CODE;
#endif
	//s_temp_calib_code = calib_temp_code;
	TRACE("-> calib_temp_code[%d]", calib_temp_code);
	if ((calib_temp_code < TEMP_SENSOR_THRESHOLD_LOW) || (calib_temp_code > TEMP_SENSOR_THRESHOLD_HIGH)) 
	{
		TRACE("-> invalid calib_temp_code");
		return;
	}

	temp_code_offset = (int32_t)(s_current_temp_code) - calib_temp_code;
	dco_code_offset = (TEMP_FREQ_SLOPE_X * temp_code_offset + (TEMP_FREQ_SLOPE_Y>>1)) / TEMP_FREQ_SLOPE_Y;

	s_temp_code_offset = temp_code_offset;
	s_dco_code_offset = dco_code_offset;

	TRACE("-> temp_code_offset[%d], dco_calib_code[%d], dco_code_offset[%d]", temp_code_offset, *dco_code_ptr, dco_code_offset);

	dco_code_offset <<= 3;
	dco_code_offset -= *dco_code_ptr;
	TRACE("-> dco_code_offset calibed[%d]", dco_code_offset);
	
	*dco_code_ptr = (uint16_t)(-dco_code_offset);
	s_dco_final_code = *dco_code_ptr;
	TRACE("-> final_dco_code: %d", *dco_code_ptr);
}
#endif


