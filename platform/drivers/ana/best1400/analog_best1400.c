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
#include "plat_types.h"
#include "analog.h"
#include "cmsis.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_codec.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "pmu.h"
#include "tgt_hardware.h"

#define ANALOG_TRACE(s,...)                 //TRACE(s, ##__VA_ARGS__)

#define VCM_ON

#define DAC_DC_CALIB_BIT_WIDTH              14

#define DEFAULT_ANC_FF_ADC_GAIN_DB          6
#define DEFAULT_ANC_FB_ADC_GAIN_DB          6
#define DEFAULT_VOICE_ADC_GAIN_DB           12

#ifndef ANALOG_ADC_A_GAIN_DB
#if defined(ANC_APP) && defined(ANC_FF_ENABLED) && ((ANC_FF_MIC_CH_L == AUD_CHANNEL_MAP_CH0) || (ANC_FF_MIC_CH_R == AUD_CHANNEL_MAP_CH0))
#define ANALOG_ADC_A_GAIN_DB                DEFAULT_ANC_FF_ADC_GAIN_DB
#elif defined(ANC_APP) && defined(ANC_FB_ENABLED) && ((ANC_FB_MIC_CH_L == AUD_CHANNEL_MAP_CH0) || (ANC_FB_MIC_CH_R == AUD_CHANNEL_MAP_CH0))
#define ANALOG_ADC_A_GAIN_DB                DEFAULT_ANC_FB_ADC_GAIN_DB
#else
#define ANALOG_ADC_A_GAIN_DB                DEFAULT_VOICE_ADC_GAIN_DB
#endif
#endif

#ifndef ANALOG_ADC_B_GAIN_DB
#if defined(ANC_APP) && defined(ANC_FF_ENABLED) && ((ANC_FF_MIC_CH_L == AUD_CHANNEL_MAP_CH1) || (ANC_FF_MIC_CH_R == AUD_CHANNEL_MAP_CH1))
#define ANALOG_ADC_B_GAIN_DB                DEFAULT_ANC_FF_ADC_GAIN_DB
#elif defined(ANC_APP) && defined(ANC_FB_ENABLED) && ((ANC_FB_MIC_CH_L == AUD_CHANNEL_MAP_CH1) || (ANC_FB_MIC_CH_R == AUD_CHANNEL_MAP_CH1))
#define ANALOG_ADC_B_GAIN_DB                DEFAULT_ANC_FB_ADC_GAIN_DB
#else
#define ANALOG_ADC_B_GAIN_DB                DEFAULT_VOICE_ADC_GAIN_DB
#endif
#endif

#ifndef ANALOG_ADC_C_GAIN_DB
#if defined(ANC_APP) && defined(ANC_FF_ENABLED) && ((ANC_FF_MIC_CH_L == AUD_CHANNEL_MAP_CH2) || (ANC_FF_MIC_CH_R == AUD_CHANNEL_MAP_CH2))
#define ANALOG_ADC_C_GAIN_DB                DEFAULT_ANC_FF_ADC_GAIN_DB
#elif defined(ANC_APP) && defined(ANC_FB_ENABLED) && ((ANC_FB_MIC_CH_L == AUD_CHANNEL_MAP_CH2) || (ANC_FB_MIC_CH_R == AUD_CHANNEL_MAP_CH2))
#define ANALOG_ADC_C_GAIN_DB                DEFAULT_ANC_FB_ADC_GAIN_DB
#else
#define ANALOG_ADC_C_GAIN_DB                DEFAULT_VOICE_ADC_GAIN_DB
#endif
#endif

#ifndef ANALOG_ADC_D_GAIN_DB
#if defined(ANC_APP) && defined(ANC_FF_ENABLED) && ((ANC_FF_MIC_CH_L == AUD_CHANNEL_MAP_CH3) || (ANC_FF_MIC_CH_R == AUD_CHANNEL_MAP_CH3))
#define ANALOG_ADC_D_GAIN_DB                DEFAULT_ANC_FF_ADC_GAIN_DB
#elif defined(ANC_APP) && defined(ANC_FB_ENABLED) && ((ANC_FB_MIC_CH_L == AUD_CHANNEL_MAP_CH3) || (ANC_FB_MIC_CH_R == AUD_CHANNEL_MAP_CH3))
#define ANALOG_ADC_D_GAIN_DB                DEFAULT_ANC_FB_ADC_GAIN_DB
#else
#define ANALOG_ADC_D_GAIN_DB                DEFAULT_VOICE_ADC_GAIN_DB
#endif
#endif

#ifndef ANALOG_ADC_E_GAIN_DB
#if defined(ANC_APP) && defined(ANC_FF_ENABLED) && ((ANC_FF_MIC_CH_L == AUD_CHANNEL_MAP_CH4) || (ANC_FF_MIC_CH_R == AUD_CHANNEL_MAP_CH4))
#define ANALOG_ADC_E_GAIN_DB                DEFAULT_ANC_FF_ADC_GAIN_DB
#elif defined(ANC_APP) && defined(ANC_FB_ENABLED) && ((ANC_FB_MIC_CH_L == AUD_CHANNEL_MAP_CH4) || (ANC_FB_MIC_CH_R == AUD_CHANNEL_MAP_CH4))
#define ANALOG_ADC_E_GAIN_DB                DEFAULT_ANC_FB_ADC_GAIN_DB
#else
#define ANALOG_ADC_E_GAIN_DB                DEFAULT_VOICE_ADC_GAIN_DB
#endif
#endif

#ifndef LINEIN_ADC_GAIN_DB
#define LINEIN_ADC_GAIN_DB                  0
#endif

#ifndef CFG_HW_AUD_MICKEY_DEV
#define CFG_HW_AUD_MICKEY_DEV               (AUD_VMIC_MAP_VMIC1)
#endif

// 61
#define REG_CODEC_EN_ADCA                   (1 << 0)
#define REG_CODEC_ADCA_DITHER_EN            (1 << 1)
#define REG_CODEC_ADCA_CH_SEL_SHIFT         2
#define REG_CODEC_ADCA_CH_SEL_MASK          (0x3 << REG_CODEC_ADCA_CH_SEL_SHIFT)
#define REG_CODEC_ADCA_CH_SEL(n)            BITFIELD_VAL(REG_CODEC_ADCA_CH_SEL, n)
#define CFG_RESET_PGAA_DR                   (1 << 7)
#define REG_CODEC_ADCA_GAIN_BIT_SHIFT       8
#define REG_CODEC_ADCA_GAIN_BIT_MASK        (0x7 << REG_CODEC_ADCA_GAIN_BIT_SHIFT)
#define REG_CODEC_ADCA_GAIN_BIT(n)          BITFIELD_VAL(REG_CODEC_ADCA_GAIN_BIT, n)
#define REG_CODEC_ADCA_GAIN_UPDATE          (1 << 11)


// 64
#define REG_CODEC_ADC_IBSEL_REG_SHIFT       0
#define REG_CODEC_ADC_IBSEL_REG_MASK        (0xF << REG_CODEC_ADC_IBSEL_REG_SHIFT)
#define REG_CODEC_ADC_IBSEL_REG(n)          BITFIELD_VAL(REG_CODEC_ADC_IBSEL_REG, n)
#define REG_CODEC_ADC_IBSEL_VCOMP_SHIFT     4
#define REG_CODEC_ADC_IBSEL_VCOMP_MASK      (0xF << REG_CODEC_ADC_IBSEL_VCOMP_SHIFT)
#define REG_CODEC_ADC_IBSEL_VCOMP(n)        BITFIELD_VAL(REG_CODEC_ADC_IBSEL_VCOMP, n)
#define REG_CODEC_ADC_IBSEL_VREF_SHIFT      8
#define REG_CODEC_ADC_IBSEL_VREF_MASK       (0xF << REG_CODEC_ADC_IBSEL_VREF_SHIFT)
#define REG_CODEC_ADC_IBSEL_VREF(n)         BITFIELD_VAL(REG_CODEC_ADC_IBSEL_VREF, n)
#define REG_CODEC_ADC_LPFVCM_SW_SHIFT       12
#define REG_CODEC_ADC_LPFVCM_SW_MASK        (0xF << REG_CODEC_ADC_LPFVCM_SW_SHIFT)
#define REG_CODEC_ADC_LPFVCM_SW(n)          BITFIELD_VAL(REG_CODEC_ADC_LPFVCM_SW, n)

// 65
#define REG_CODEC_ADC_OP1_IBIT_SHIFT        0
#define REG_CODEC_ADC_OP1_IBIT_MASK         (0x3 << REG_CODEC_ADC_OP1_IBIT_SHIFT)
#define REG_CODEC_ADC_OP1_IBIT(n)           BITFIELD_VAL(REG_CODEC_ADC_OP1_IBIT, n)
#define REG_CODEC_ADC_OP1_R_SEL             (1 << 2)
#define REG_CODEC_ADC_OP2_IBIT_SHIFT        3
#define REG_CODEC_ADC_OP2_IBIT_MASK         (0x3 << REG_CODEC_ADC_OP2_IBIT_SHIFT)
#define REG_CODEC_ADC_OP2_IBIT(n)           BITFIELD_VAL(REG_CODEC_ADC_OP2_IBIT, n)
#define REG_CODEC_ADC_OP2_R_SEL             (1 << 5)
#define REG_CODEC_ADC_OP3_IBIT_SHIFT        6
#define REG_CODEC_ADC_OP3_IBIT_MASK         (0x3 << REG_CODEC_ADC_OP3_IBIT_SHIFT)
#define REG_CODEC_ADC_OP3_IBIT(n)           BITFIELD_VAL(REG_CODEC_ADC_OP3_IBIT, n)
#define REG_CODEC_ADC_OP3_R_SEL             (1 << 8)
#define REG_CODEC_ADC_OP4_IBIT_SHIFT        9
#define REG_CODEC_ADC_OP4_IBIT_MASK         (0x3 << REG_CODEC_ADC_OP4_IBIT_SHIFT)
#define REG_CODEC_ADC_OP4_IBIT(n)           BITFIELD_VAL(REG_CODEC_ADC_OP4_IBIT, n)
#define REG_CODEC_ADC_OP4_R_SEL             (1 << 11)
#define REG_CODEC_ADC_VREF_SEL_SHIFT        12
#define REG_CODEC_ADC_VREF_SEL_MASK         (0xF << REG_CODEC_ADC_VREF_SEL_SHIFT)
#define REG_CODEC_ADC_VREF_SEL(n)           BITFIELD_VAL(REG_CODEC_ADC_VREF_SEL, n)

// 66
#define REG_CODEC_BIAS_IBSEL_SHIFT          0
#define REG_CODEC_BIAS_IBSEL_MASK           (0xF << REG_CODEC_BIAS_IBSEL_SHIFT)
#define REG_CODEC_BIAS_IBSEL(n)             BITFIELD_VAL(REG_CODEC_BIAS_IBSEL, n)
#define REG_CODEC_BIAS_IBSEL_TX_SHIFT       4
#define REG_CODEC_BIAS_IBSEL_TX_MASK        (0xF << REG_CODEC_BIAS_IBSEL_TX_SHIFT)
#define REG_CODEC_BIAS_IBSEL_TX(n)          BITFIELD_VAL(REG_CODEC_BIAS_IBSEL_TX, n)
#define REG_CODEC_BIAS_IBSEL_VOICE_SHIFT    8
#define REG_CODEC_BIAS_IBSEL_VOICE_MASK     (0xF << REG_CODEC_BIAS_IBSEL_VOICE_SHIFT)
#define REG_CODEC_BIAS_IBSEL_VOICE(n)       BITFIELD_VAL(REG_CODEC_BIAS_IBSEL_VOICE, n)
#define REG_CODEC_BIAS_LOWV                 (1 << 12)
#define REG_CODEC_BIAS_LOWV_LP              (1 << 13)
#define REG_CODEC_BUF_LOWPOWER              (1 << 14)
#define REG_CODEC_BUF_LOWPOWER2             (1 << 15)

// 67
#define REG_CODEC_ADC_REG_VSEL_SHIFT        0
#define REG_CODEC_ADC_REG_VSEL_MASK         (0x7 << REG_CODEC_ADC_REG_VSEL_SHIFT)
#define REG_CODEC_ADC_REG_VSEL(n)           BITFIELD_VAL(REG_CODEC_ADC_REG_VSEL, n)
#define REG_CODEC_ADC_RES_SEL_SHIFT         3
#define REG_CODEC_ADC_RES_SEL_MASK          (0x7 << REG_CODEC_ADC_RES_SEL_SHIFT)
#define REG_CODEC_ADC_RES_SEL(n)            BITFIELD_VAL(REG_CODEC_ADC_RES_SEL, n)
#define REG_CODEC_BUF_LOWVCM_SHIFT          6
#define REG_CODEC_BUF_LOWVCM_MASK           (0x7 << REG_CODEC_BUF_LOWVCM_SHIFT)
#define REG_CODEC_BUF_LOWVCM(n)             BITFIELD_VAL(REG_CODEC_BUF_LOWVCM, n)
#define REG_CODEC_EN_BIAS                   (1 << 9)
#define REG_CODEC_EN_BIAS_LP                (1 << 10)
#define REG_CODEC_EN_TX_EXT                 (1 << 11)
#define REG_CODEC_DAC_CLK_EDGE_SEL          (1 << 12)
#define CFG_TX_CH0_MUTE                     (1 << 13)
#define CFG_TX_CH1_MUTE                     (1 << 14)

// 68
#define REG_CODEC_EN_VCM                    (1 << 0)
#define REG_CODEC_VCM_EN_LPF                (1 << 1)
#define REG_CODEC_LP_VCM                    (1 << 2)
#define REG_CODEC_VCM_LOW_VCM_SHIFT         3
#define REG_CODEC_VCM_LOW_VCM_MASK          (0xF << REG_CODEC_VCM_LOW_VCM_SHIFT)
#define REG_CODEC_VCM_LOW_VCM(n)            BITFIELD_VAL(REG_CODEC_VCM_LOW_VCM, n)
#define REG_CODEC_VCM_LOW_VCM_LP_SHIFT      7
#define REG_CODEC_VCM_LOW_VCM_LP_MASK       (0xF << REG_CODEC_VCM_LOW_VCM_LP_SHIFT)
#define REG_CODEC_VCM_LOW_VCM_LP(n)         BITFIELD_VAL(REG_CODEC_VCM_LOW_VCM_LP, n)
#define REG_CODEC_VCM_LOW_VCM_LPF_SHIFT     11
#define REG_CODEC_VCM_LOW_VCM_LPF_MASK      (0xF << REG_CODEC_VCM_LOW_VCM_LPF_SHIFT)
#define REG_CODEC_VCM_LOW_VCM_LPF(n)        BITFIELD_VAL(REG_CODEC_VCM_LOW_VCM_LPF, n)
#define REG_CODEC_EN_VCM_BUFFER             (1 << 15)

// 69
#define REG_CODEC_RX_EN_VTOI                (1 << 0)
#define REG_CODEC_RX_VTOI_I_DAC2_SHIFT      1
#define REG_CODEC_RX_VTOI_I_DAC2_MASK       (0x7 << REG_CODEC_RX_VTOI_I_DAC2_SHIFT)
#define REG_CODEC_RX_VTOI_I_DAC2(n)         BITFIELD_VAL(REG_CODEC_RX_VTOI_I_DAC2, n)
#define REG_CODEC_RX_VTOI_IDAC_SEL_SHIFT    4
#define REG_CODEC_RX_VTOI_IDAC_SEL_MASK     (0xF << REG_CODEC_RX_VTOI_IDAC_SEL_SHIFT)
#define REG_CODEC_RX_VTOI_IDAC_SEL(n)       BITFIELD_VAL(REG_CODEC_RX_VTOI_IDAC_SEL, n)
#define REG_CODEC_RX_VTOI_VCS_SEL_SHIFT     8
#define REG_CODEC_RX_VTOI_VCS_SEL_MASK      (0x1F << REG_CODEC_RX_VTOI_VCS_SEL_SHIFT)
#define REG_CODEC_RX_VTOI_VCS_SEL(n)        BITFIELD_VAL(REG_CODEC_RX_VTOI_VCS_SEL, n)
#define REG_CODEC_TEST_SEL_SHIFT            13
#define REG_CODEC_TEST_SEL_MASK             (0x7 << REG_CODEC_TEST_SEL_SHIFT)
#define REG_CODEC_TEST_SEL(n)               BITFIELD_VAL(REG_CODEC_TEST_SEL, n)

// 6A
#define REG_CODEC_TX_DAC_MUTEL              (1 << 0)
#define REG_CODEC_TX_DAC_MUTER              (1 << 1)
#define REG_CODEC_TX_DAC_SWR_SHIFT          2
#define REG_CODEC_TX_DAC_SWR_MASK           (0x3 << REG_CODEC_TX_DAC_SWR_SHIFT)
#define REG_CODEC_TX_DAC_SWR(n)             BITFIELD_VAL(REG_CODEC_TX_DAC_SWR, n)
#define REG_CODEC_TX_DAC_VREF_L_SHIFT       4
#define REG_CODEC_TX_DAC_VREF_L_MASK        (0xF << REG_CODEC_TX_DAC_VREF_L_SHIFT)
#define REG_CODEC_TX_DAC_VREF_L(n)          BITFIELD_VAL(REG_CODEC_TX_DAC_VREF_L, n)
#define REG_CODEC_TX_DAC_VREF_R_SHIFT       8
#define REG_CODEC_TX_DAC_VREF_R_MASK        (0xF << REG_CODEC_TX_DAC_VREF_R_SHIFT)
#define REG_CODEC_TX_DAC_VREF_R(n)          BITFIELD_VAL(REG_CODEC_TX_DAC_VREF_R, n)
#define REG_CODEC_TX_EAR_DIS_SHIFT          12
#define REG_CODEC_TX_EAR_DIS_MASK           (0x3 << REG_CODEC_TX_EAR_DIS_SHIFT)
#define REG_CODEC_TX_EAR_DIS(n)             BITFIELD_VAL(REG_CODEC_TX_EAR_DIS, n)

// 6B
#define REG_CODEC_TX_EAR_COMP_L_SHIFT       0
#define REG_CODEC_TX_EAR_COMP_L_MASK        (0x7F << REG_CODEC_TX_EAR_COMP_L_SHIFT)
#define REG_CODEC_TX_EAR_COMP_L(n)          BITFIELD_VAL(REG_CODEC_TX_EAR_COMP_L, n)
#define REG_CODEC_TX_EAR_COMP_R_SHIFT       7
#define REG_CODEC_TX_EAR_COMP_R_MASK        (0x7F << REG_CODEC_TX_EAR_COMP_R_SHIFT)
#define REG_CODEC_TX_EAR_COMP_R(n)          BITFIELD_VAL(REG_CODEC_TX_EAR_COMP_R, n)

//6C
#define REG_CODEC_TX_EAR_DOUBLEBIAS         (1 << 0)
#define REG_CODEC_TX_EAR_DR_EN              (1 << 1)
#define REG_CODEC_TX_EAR_DR_ST_SHIFT        2
#define REG_CODEC_TX_EAR_DR_ST_MASK         (0x7 << REG_CODEC_TX_EAR_DR_ST_SHIFT)
#define REG_CODEC_TX_EAR_DR_ST(n)           BITFIELD_VAL(REG_CODEC_TX_EAR_DR_ST, n)
#define REG_CODEC_TX_EAR_ENBIAS             (1 << 5)
#define REG_CODEC_TX_EAR_FBCAP_SHIFT        6
#define REG_CODEC_TX_EAR_FBCAP_MASK         (0x3 << REG_CODEC_TX_EAR_FBCAP_SHIFT)
#define REG_CODEC_TX_EAR_FBCAP(n)           BITFIELD_VAL(REG_CODEC_TX_EAR_FBCAP, n)
#define REG_CODEC_TX_EAR_IBSEL_SHIFT        8
#define REG_CODEC_TX_EAR_IBSEL_MASK         (0x3 << REG_CODEC_TX_EAR_IBSEL_SHIFT)
#define REG_CODEC_TX_EAR_IBSEL(n)           BITFIELD_VAL(REG_CODEC_TX_EAR_IBSEL, n)

// 6D
#define REG_CODEC_TX_EAR_DRE_GAIN_L_SHIFT   0
#define REG_CODEC_TX_EAR_DRE_GAIN_L_MASK    (0x1F << REG_CODEC_TX_EAR_DRE_GAIN_L_SHIFT)
#define REG_CODEC_TX_EAR_DRE_GAIN_L(n)      BITFIELD_VAL(REG_CODEC_TX_EAR_DRE_GAIN_L, n)
#define REG_CODEC_TX_EAR_DRE_GAIN_R_SHIFT   5
#define REG_CODEC_TX_EAR_DRE_GAIN_R_MASK    (0x1F << REG_CODEC_TX_EAR_DRE_GAIN_R_SHIFT)
#define REG_CODEC_TX_EAR_DRE_GAIN_R(n)      BITFIELD_VAL(REG_CODEC_TX_EAR_DRE_GAIN_R, n)
#define DRE_GAIN_SEL_L                      (1 << 10)
#define DRE_GAIN_SEL_R                      (1 << 11)
#define REG_CODEC_TX_EAR_DRE_GAIN_L_UPDATE  (1 << 12)
#define REG_CODEC_TX_EAR_DRE_GAIN_R_UPDATE  (1 << 13)
#define REG_CODEC_TX_EAR_GAIN_SHIFT         14
#define REG_CODEC_TX_EAR_GAIN_MASK          (0x3 << REG_CODEC_TX_EAR_GAIN_SHIFT)
#define REG_CODEC_TX_EAR_GAIN(n)            BITFIELD_VAL(REG_CODEC_TX_EAR_GAIN, n)

// 6E
#define REG_CODEC_TX_EAR_LCAL               (1 << 0)
#define REG_CODEC_TX_EAR_RCAL               (1 << 1)
#define REG_CODEC_TX_EAR_LPBIAS             (1 << 2)
#define REG_CODEC_TX_EAR_OCEN               (1 << 3)
#define REG_CODEC_TX_EAR_OFFEN              (1 << 4)
#define REG_CODEC_TX_EAR_OUTPUTSEL_SHIFT    5
#define REG_CODEC_TX_EAR_OUTPUTSEL_MASK     (0xF << REG_CODEC_TX_EAR_OUTPUTSEL_SHIFT)
#define REG_CODEC_TX_EAR_OUTPUTSEL(n)       BITFIELD_VAL(REG_CODEC_TX_EAR_OUTPUTSEL, n)
#define REG_CODEC_TX_EAR_SOFTSTART_SHIFT    9
#define REG_CODEC_TX_EAR_SOFTSTART_MASK     (0x3F << REG_CODEC_TX_EAR_SOFTSTART_SHIFT)
#define REG_CODEC_TX_EAR_SOFTSTART(n)       BITFIELD_VAL(REG_CODEC_TX_EAR_SOFTSTART, n)
#define CFG_TX_TREE_EN                      (1 << 15)

// 6F
#define REG_CODEC_TX_EAR_LOWGAINL_SHIFT     0
#define REG_CODEC_TX_EAR_LOWGAINL_MASK      (0x3 << REG_CODEC_TX_EAR_LOWGAINL_SHIFT)
#define REG_CODEC_TX_EAR_LOWGAINL(n)        BITFIELD_VAL(REG_CODEC_TX_EAR_LOWGAINL, n)
#define REG_CODEC_TX_EAR_OFF_BITL_SHIFT     2
#define REG_CODEC_TX_EAR_OFF_BITL_MASK      (0x3FFF << REG_CODEC_TX_EAR_OFF_BITL_SHIFT)
#define REG_CODEC_TX_EAR_OFF_BITL(n)        BITFIELD_VAL(REG_CODEC_TX_EAR_OFF_BITL, n)

// 70
#define REG_CODEC_TX_EAR_LOWGAINR_SHIFT     0
#define REG_CODEC_TX_EAR_LOWGAINR_MASK      (0x3 << REG_CODEC_TX_EAR_LOWGAINR_SHIFT)
#define REG_CODEC_TX_EAR_LOWGAINR(n)        BITFIELD_VAL(REG_CODEC_TX_EAR_LOWGAINR, n)
#define REG_CODEC_TX_EAR_OFF_BITR_SHIFT     2
#define REG_CODEC_TX_EAR_OFF_BITR_MASK      (0x3FFF << REG_CODEC_TX_EAR_OFF_BITR_SHIFT)
#define REG_CODEC_TX_EAR_OFF_BITR(n)        BITFIELD_VAL(REG_CODEC_TX_EAR_OFF_BITR, n)

// 71
#define REG_CODEC_TX_EN_DACLDO              (1 << 0)
#define REG_CODEC_TX_EN_EARPA_L             (1 << 1)
#define REG_CODEC_TX_EN_EARPA_R             (1 << 2)
#define REG_CODEC_TX_EN_LCLK                (1 << 3)
#define REG_CODEC_TX_EN_RCLK                (1 << 4)
#define REG_CODEC_TX_EN_LDAC                (1 << 5)
#define REG_CODEC_TX_EN_RDAC                (1 << 6)
#define REG_CODEC_TX_EN_LPPA                (1 << 7)
#define REG_CODEC_TX_EN_S1PA                (1 << 8)
#define REG_CODEC_TX_EN_S2PA                (1 << 9)
#define REG_CODEC_TX_EN_S3PA                (1 << 10)
#define REG_CODEC_TX_EN_S4PA                (1 << 11)
#define REG_CODEC_TX_EN_S5PA                (1 << 12)
#define REG_IDETLEAR_EN                     (1 << 13)
#define REG_IDETREAR_EN                     (1 << 14)
#define REG_CODEC_TX_SW_MODE                (1 << 15)

// 72
#define REG_CODEC_TX_RVREF_CAP_BIT          (1 << 0)
#define REG_DAC_LDO0P9_VSEL_SHIFT           1
#define REG_DAC_LDO0P9_VSEL_MASK            (0x1F << REG_DAC_LDO0P9_VSEL_SHIFT)
#define REG_DAC_LDO0P9_VSEL(n)              BITFIELD_VAL(REG_DAC_LDO0P9_VSEL, n)
#define REG_BYPASS_TX_REGULATOR             (1 << 6)
#define REG_PU_TX_REGULATOR                 (1 << 7)
#define REG_TX_REGULATOR_BIT_SHIFT          8
#define REG_TX_REGULATOR_BIT_MASK           (0xF << REG_TX_REGULATOR_BIT_SHIFT)
#define REG_TX_REGULATOR_BIT(n)             BITFIELD_VAL(REG_TX_REGULATOR_BIT, n)
#define REG_TX_REG_CAP_BIT                  (1 << 12)
#define REG_CODEC_TX_EAR_VCM_BIT_SHIFT      13
#define REG_CODEC_TX_EAR_VCM_BIT_MASK       (0x3 << REG_CODEC_TX_EAR_VCM_BIT_SHIFT)
#define REG_CODEC_TX_EAR_VCM_BIT(n)         BITFIELD_VAL(REG_CODEC_TX_EAR_VCM_BIT, n)
#define REG_CODEC_TX_EAR_VCM_SEL            (1 << 15)

// 73
#define REG_CODEC_ADC_DITHER0P5_EN          (1 << 0)
#define REG_CODEC_ADC_DITHER0P25_EN         (1 << 1)
#define REG_CODEC_ADC_DITHER1_EN            (1 << 2)
#define REG_CODEC_ADC_DITHER2_EN            (1 << 3)
#define REG_CODEC_ADC_DITHER_PHASE_SEL      (1 << 4)
#define RX_TIMER_RSTN_BIT_SHIFT             5
#define RX_TIMER_RSTN_BIT_MASK              (0x3F << RX_TIMER_RSTN_BIT_SHIFT)
#define RX_TIMER_RSTN_BIT(n)                BITFIELD_VAL(RX_TIMER_RSTN_BIT, n)
#define REG_PU_OSC                          (1 << 11)

// 164
#define CFG_TX_PEAK_OFF_ADC_EN              (1 << 0)
#define CFG_TX_PEAK_OFF_DAC_EN              (1 << 1)
#define CFG_PEAK_DET_DR                     (1 << 2)
#define CFG_TX_PEAK_OFF_ADC                 (1 << 3)
#define CFG_TX_PEAK_OFF_DAC                 (1 << 4)
#define CFG_PEAK_DET_DB_DELAY_SHIFT         5
#define CFG_PEAK_DET_DB_DELAY_MASK          (0x7 << CFG_PEAK_DET_DB_DELAY_SHIFT)
#define CFG_PEAK_DET_DB_DELAY(n)            BITFIELD_VAL(CFG_PEAK_DET_DB_DELAY, n)
#define REG_CODEC_TX_PEAK_DET_BIT           (1 << 8)
#define REG_CODEC_TX_PEAK_NL_EN             (1 << 9)
#define REG_CODEC_TX_PEAK_NR_EN             (1 << 10)
#define REG_CODEC_TX_PEAK_PL_EN             (1 << 11)
#define REG_CODEC_TX_PEAK_PR_EN             (1 << 12)
#define CFG_TX_CLK_INV                      (1 << 13)
#define CFG_CODEC_DIN_L_RST                 (1 << 14)
#define CFG_CODEC_DIN_R_RST                 (1 << 15)

// C4
#define CODEC_RESAMPLE_CLK_BUF_PU           (1 << 2)

// 16A
#define CFG_GMC_POWER_ON                    (1 << 0)
#define CFG_GMC_DR                          (1 << 1)
#define CFG_GMC_RESET                       (1 << 2)
#define CFG_GMC_CLK_EN                      (1 << 3)
#define EN_CYCLE_SHIFT                      4
#define EN_CYCLE_MASK                       (0x1F << EN_CYCLE_SHIFT)
#define EN_CYCLE(n)                         BITFIELD_VAL(EN_CYCLE, n)
#define RST_CYCLE_SHIFT                     9
#define RST_CYCLE_MASK                      (0x1F << RST_CYCLE_SHIFT)
#define RST_CYCLE(n)                        BITFIELD_VAL(RST_CYCLE, n)
#define RESERVED_DIG_1_0_SHIFT              14
#define RESERVED_DIG_1_0_MASK               (0x3 << RESERVED_DIG_1_0_SHIFT)
#define RESERVED_DIG_1_0(n)                 BITFIELD_VAL(RESERVED_DIG_1_0, n)

// 16C
#define REG_CODEC_EN_ADCE_GMC               (1 << 0)
#define REG_CODEC_ADCE_GMC_RST              (1 << 1)
#define REG_CODEC_ADCE_GMC_EN               (1 << 2)
#define REG_CODEC_ADCE_GMC_EN_I_MIRROR      (1 << 3)
#define REG_CODEC_ADCE_DIG_GMC_CLK_FAST     (1 << 4)
#define REG_CODEC_ADCE_GMC_LVL_PD_SW        (1 << 5)
#define REG_CODEC_ADCE_GMC_PU_CLIP          (1 << 6)
#define REG_CODEC_ADCE_GMC_PU_COFF          (1 << 7)
#define REG_CODEC_ADCE_GMC_PU_OTA1          (1 << 8)
#define REG_CODEC_ADCE_GMC_PU_OTA234        (1 << 9)
#define REG_CODEC_ADCE_GMC_PU_PATA          (1 << 10)
#define REG_CODEC_ADCE_GMC_PU_INPUT_SW      (1 << 11)
#define REG_CODEC_GMC_LDOA_EN               (1 << 12)
#define REG_CODEC_GMC_LDOD_EN               (1 << 13)
#define RESERVED_DIG_5_4_SHIFT              14
#define RESERVED_DIG_5_4_MASK               (0x3 << RESERVED_DIG_5_4_SHIFT)
#define RESERVED_DIG_5_4(n)                 BITFIELD_VAL(RESERVED_DIG_5_4, n)

// 171
#define CFG_VOICE_POWER_ON                  (1 << 0)
#define CFG_VOICE_TRIG_ENABLE               (1 << 1)
#define CFG_VOICE_RESET                     (1 << 2)
#define CFG_VOICE_DR                        (1 << 3)
#define CFG_VOICE_PU_DELAY_SHIFT            4
#define CFG_VOICE_PU_DELAY_MASK             (0x7 << CFG_VOICE_PU_DELAY_SHIFT)
#define CFG_VOICE_PU_DELAY(n)               BITFIELD_VAL(CFG_VOICE_PU_DELAY, n)
#define CFG_VOICE_PRECH_DELAY_SHIFT         7
#define CFG_VOICE_PRECH_DELAY_MASK          (0x1F << CFG_VOICE_PRECH_DELAY_SHIFT)
#define CFG_VOICE_PRECH_DELAY(n)            BITFIELD_VAL(CFG_VOICE_PRECH_DELAY, n)
#define REG_VOICE_GAIN_SHIFT                12
#define REG_VOICE_GAIN_MASK                 (0x3 << REG_VOICE_GAIN_SHIFT)
#define REG_VOICE_GAIN(n)                   BITFIELD_VAL(REG_VOICE_GAIN, n)
#define CFG_REG_PU_CLKMUX_LDO0P9            (1 << 14)
#define CFG_REG_CODEC_EN_BIAS_LP            (1 << 15)

// 172
#define REG_VOICE_CLK_EDGE_SEL              (1 << 0)
#define REG_VOICE_PRECH                     (1 << 1)
#define REG_VOICE_PU                        (1 << 2)
#define REG_VOICE_PU_LDO                    (1 << 3)
#define REG_VOICE_SEL_VCM0P3_BIN_SHIFT      4
#define REG_VOICE_SEL_VCM0P3_BIN_MASK       (0x7 << REG_VOICE_SEL_VCM0P3_BIN_SHIFT)
#define REG_VOICE_SEL_VCM0P3_BIN(n)         BITFIELD_VAL(REG_VOICE_SEL_VCM0P3_BIN, n)
#define REG_VOICE_SEL_VCM_BIN_SHIFT         7
#define REG_VOICE_SEL_VCM_BIN_MASK          (0x7 << REG_VOICE_SEL_VCM_BIN_SHIFT)
#define REG_VOICE_SEL_VCM_BIN(n)            BITFIELD_VAL(REG_VOICE_SEL_VCM_BIN, n)
#define REG_VOICE_SEL_VCMIN_BIN_SHIFT       10
#define REG_VOICE_SEL_VCMIN_BIN_MASK        (0x7 << REG_VOICE_SEL_VCMIN_BIN_SHIFT)
#define REG_VOICE_SEL_VCMIN_BIN(n)          BITFIELD_VAL(REG_VOICE_SEL_VCMIN_BIN, n)
#define REG_VOICE_SEL_VREF_BIN_SHIFT        13
#define REG_VOICE_SEL_VREF_BIN_MASK         (0x7 << REG_VOICE_SEL_VREF_BIN_SHIFT)
#define REG_VOICE_SEL_VREF_BIN(n)           BITFIELD_VAL(REG_VOICE_SEL_VREF_BIN, n)

enum ANA_REG_T {
    ANA_REG_61 = 0x61,
    ANA_REG_62,
    ANA_REG_63,
    ANA_REG_64,
    ANA_REG_65,
    ANA_REG_66,
    ANA_REG_67,
    ANA_REG_68,
    ANA_REG_69,
    ANA_REG_6A,
    ANA_REG_6B,
    ANA_REG_6C,
    ANA_REG_6D,
    ANA_REG_6E,
    ANA_REG_6F,
    ANA_REG_70,
    ANA_REG_71,
    ANA_REG_72,
    ANA_REG_73,
    ANA_REG_74,

    ANA_REG_161 = 0x161,
    ANA_REG_162,
    ANA_REG_163,
    ANA_REG_164,
    ANA_REG_165,
    ANA_REG_166,
    ANA_REG_167,
    ANA_REG_168,
    ANA_REG_169,
    ANA_REG_16A,
};

enum ANA_CODEC_USER_T {
    ANA_CODEC_USER_DAC,
    ANA_CODEC_USER_ADC,

    ANA_CODEC_USER_CODEC,
    ANA_CODEC_USER_MICKEY,

    ANA_CODEC_USER_ANC_FF,
    ANA_CODEC_USER_ANC_FB,

    ANA_CODEC_USER_ANC_INIT,

    ANA_CODEC_USER_QTY
};

typedef uint8_t CODEC_USER_MAP;
STATIC_ASSERT(sizeof(CODEC_USER_MAP) * 8 >= ANA_CODEC_USER_QTY, "Invalid codec user map type size");

struct ANALOG_PLL_CFG_T {
    uint32_t freq;
    uint8_t div;
    uint16_t val[3];
    int32_t total_delta;
};

#define OPT_TYPE                        const

static OPT_TYPE uint16_t vcodec_mv = (uint16_t)(VCODEC_VOLT * 1000);

static bool ana_spk_req;
static bool ana_spk_muted;
static bool ana_spk_enabled;

static CODEC_USER_MAP adc_map[MAX_ANA_MIC_CH_NUM];
static CODEC_USER_MAP vmic_map[MAX_ANA_MIC_CH_NUM];
static CODEC_USER_MAP codec_common_map;
static CODEC_USER_MAP adda_common_map;
static CODEC_USER_MAP vcodec_map;


#ifdef ANC_APP
#ifndef DYN_ADC_GAIN
#define DYN_ADC_GAIN
#endif
#endif

#ifdef DYN_ADC_GAIN
static int8_t adc_gain_offset[MAX_ANA_MIC_CH_NUM];
#endif

static const int8_t adc_db[] = { -9, -6, -3, 0, 3, 6, 9, 12, };

static const int8_t tgt_adc_db[MAX_ANA_MIC_CH_NUM] = {
    ANALOG_ADC_A_GAIN_DB
};

void analog_aud_osc_clk_enable(bool enable)
{
    uint16_t val;

    if (enable) {
        analog_read(ANA_REG_73, &val);
        val |= REG_PU_OSC;
        analog_write(ANA_REG_73, val);
    } else {
        analog_read(ANA_REG_73, &val);
        val &= ~REG_PU_OSC;
        analog_write(ANA_REG_73, val);
    }
}

void analog_aud_pll_open(enum ANA_AUD_PLL_USER_T user)
{
    if (user >= ANA_AUD_PLL_USER_QTY) {
        return;
    }

    if (user == ANA_AUD_PLL_USER_CODEC &&
            hal_cmu_get_audio_resample_status()) {

        analog_aud_osc_clk_enable(true);
    }
}

void analog_aud_pll_close(enum ANA_AUD_PLL_USER_T user)
{
    if (user >= ANA_AUD_PLL_USER_QTY) {
        return;
    }

    if (user == ANA_AUD_PLL_USER_CODEC &&
            hal_cmu_get_audio_resample_status()) {

        analog_aud_osc_clk_enable(false);
    }
}

static void analog_aud_enable_dac(uint32_t dac)
{
}

static void analog_aud_enable_dac_pa(uint32_t dac)
{
    uint16_t val_6c;
    uint16_t val_6E;
    uint16_t val_71;

    analog_read(ANA_REG_6C, &val_6c);
    analog_read(ANA_REG_6E, &val_6E);

    if (dac & (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) {
        val_6c |= REG_CODEC_TX_EAR_DR_EN | REG_CODEC_TX_EAR_ENBIAS;
        analog_write(ANA_REG_6C, val_6c);
        osDelay(1);
        val_6E |= CFG_TX_TREE_EN;
        analog_write(ANA_REG_6E, val_6E);
        osDelay(1);

        val_71 = 0;
        if (vcodec_mv >= 1800) {
            val_71 |= REG_CODEC_TX_EN_S2PA | REG_CODEC_TX_EN_S3PA | REG_CODEC_TX_EN_S5PA | REG_CODEC_TX_SW_MODE;
        }
        if (dac & AUD_CHANNEL_MAP_CH0) {
            val_71 |= REG_CODEC_TX_EN_EARPA_L | REG_CODEC_TX_EN_LCLK | REG_CODEC_TX_EN_LDAC;
        }
        if (dac & AUD_CHANNEL_MAP_CH1) {
            val_71 |= REG_CODEC_TX_EN_EARPA_R | REG_CODEC_TX_EN_RCLK | REG_CODEC_TX_EN_RDAC
                   | REG_CODEC_TX_EN_LDAC;
        }
        val_71 |= REG_CODEC_TX_EN_DACLDO | REG_CODEC_TX_EN_LPPA;
        analog_write(ANA_REG_71, val_71);
        osDelay(1);
        val_71 |= REG_CODEC_TX_EN_S1PA;
        analog_write(ANA_REG_71, val_71);
        osDelay(1);
        val_71 |= REG_CODEC_TX_EN_S4PA;
        analog_write(ANA_REG_71, val_71);

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
        hal_codec_dac_sdm_reset_clear();
#endif
    } else {
#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
        hal_codec_dac_sdm_reset_set();
#endif

        analog_read(ANA_REG_71, &val_71);

        val_71 &= ~REG_CODEC_TX_EN_S4PA;
        analog_write(ANA_REG_71, val_71);
        osDelay(1);
        val_71 &= ~REG_CODEC_TX_EN_S1PA;
        analog_write(ANA_REG_71, val_71);
        osDelay(1);
        val_71 = 0;
        analog_write(ANA_REG_71, val_71);
        osDelay(1);

        val_6E &= ~CFG_TX_TREE_EN;
        analog_write(ANA_REG_6E, val_6E);
        osDelay(1);

        val_6c &= ~(REG_CODEC_TX_EAR_DR_EN | REG_CODEC_TX_EAR_ENBIAS);
        analog_write(ANA_REG_6C, val_6c);
    }
}

static void analog_aud_enable_adc(enum ANA_CODEC_USER_T user, enum AUD_CHANNEL_MAP_T ch_map, bool en)
{
    int i;
    uint16_t val_61, val_69;
    CODEC_USER_MAP old_map;
    bool set;
    bool global_update;

    ANALOG_TRACE("[%s] user=%d ch_map=0x%x", __func__, user, ch_map);

    global_update = false;

    analog_read(ANA_REG_61, &val_61);

    for (i = 0; i < MAX_ANA_MIC_CH_NUM; i++) {
        if (ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
            set = false;
            if (en) {
                if (adc_map[i] == 0) {
                    set = true;
                }
                adc_map[i] |= (1 << user);
            } else {
                old_map = adc_map[i];
                adc_map[i] &= ~(1 << user);
                if (old_map != 0 && adc_map[i] == 0) {
                    set = true;
                }
            }
            if (set) {
                global_update = true;
                if (adc_map[i]) {
                    val_61 |= (REG_CODEC_EN_ADCA << i);
                } else {
                    val_61 &= ~(REG_CODEC_EN_ADCA << i);
                }
            }
        }
    }

    analog_write(ANA_REG_61, val_61);

    if (global_update) {
        for (i = 0; i < MAX_ANA_MIC_CH_NUM; i++) {
            if (adc_map[i]) {
                break;
            }
        }
        analog_read(ANA_REG_69, &val_69);
        if (i < MAX_ANA_MIC_CH_NUM) {
            val_69 |= REG_CODEC_RX_EN_VTOI;
        } else {
            val_69 &= ~REG_CODEC_RX_EN_VTOI;
        }
        analog_write(ANA_REG_69, val_69);
    }
}

static uint32_t db_to_adc_gain(int db)
{
    int i;
    uint8_t cnt;
    const int8_t *list;

    list = adc_db;
    cnt = ARRAY_SIZE(adc_db);

    for (i = 0; i < cnt - 1; i++) {
        if (db < list[i + 1]) {
            break;
        }
    }

    if (i == cnt - 1) {
        return i;
    }
    else if (db * 2 < list[i] + list[i + 1]) {
        return i;
    } else {
        return i + 1;
    }
}

static uint16_t get_chan_adc_gain(uint32_t i)
{
    uint16_t gain;

    gain = tgt_adc_db[i];

#ifdef DYN_ADC_GAIN
    if (adc_gain_offset[i] < 0 && -adc_gain_offset[i] > gain) {
        gain = 0;
    } else {
        gain += adc_gain_offset[i];
    }
#endif

    return gain;
}

static void analog_aud_set_adc_gain(enum AUD_IO_PATH_T input_path, enum AUD_CHANNEL_MAP_T ch_map)
{
    int i;
    int gain;
    uint16_t gain_val;
    uint16_t val;

    analog_read(ANA_REG_61, &val);

    for (i = 0; i < MAX_ANA_MIC_CH_NUM - 1; i++) {
        if (ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
            if (0) {
            } else if (input_path == AUD_INPUT_PATH_LINEIN) {
                gain = LINEIN_ADC_GAIN_DB;
            } else {
                gain = get_chan_adc_gain(i);
            }
            gain_val = db_to_adc_gain(gain);
            val = (val & ~(REG_CODEC_ADCA_GAIN_BIT_MASK << 3 * i)) | (REG_CODEC_ADCA_GAIN_BIT(gain_val) << 3 * i);
        }
    }

    analog_write(ANA_REG_61, val);
}

void analog_aud_apply_anc_adc_gain_offset(enum ANC_TYPE_T type, int16_t offset_l, int16_t offset_r)
{
}

#ifdef DYN_ADC_GAIN
void analog_aud_apply_adc_gain_offset(enum AUD_CHANNEL_MAP_T ch_map, int16_t offset)
{
    enum AUD_CHANNEL_MAP_T map;
    int i;

    if (ch_map) {
        map = ch_map;

        while (map) {
            i = get_msb_pos(map);
            map &= ~(1 << i);
            if (i < MAX_ANA_MIC_CH_NUM) {
                adc_gain_offset[i] = offset;
            }
        }

        TRACE("ana: apply adc gain offset: ch_map=0x%X offset=%d", ch_map, offset);

        analog_aud_set_adc_gain(AUD_INPUT_PATH_MAINMIC, ch_map);
    }
}
#endif

void analog_aud_set_dac_gain(int32_t v)
{
}

uint32_t analog_codec_get_dac_gain(void)
{
    return 0;
}

uint32_t analog_codec_dac_gain_to_db(int32_t gain)
{
    return 0;
}

int32_t analog_codec_dac_max_attn_db(void)
{
    return 0;
}

void analog_aud_apply_anc_dac_gain_offset(int16_t offset)
{
}

static int POSSIBLY_UNUSED dc_calib_checksum_valid(uint32_t efuse)
{
    int i;
    uint32_t cnt = 0;
    uint32_t chksum_mask = (1 << (16 - DAC_DC_CALIB_BIT_WIDTH)) - 1;

    for (i = 0; i < DAC_DC_CALIB_BIT_WIDTH; i++) {
        if (efuse & (1 << i)) {
            cnt++;
        }
    }

    return (((~cnt) & chksum_mask) == ((efuse >> DAC_DC_CALIB_BIT_WIDTH) & chksum_mask));
}

void analog_aud_get_dc_calib_value(int16_t *dc_l, int16_t *dc_r)
{
    static const uint8_t EFUSE_PAGE_DIFF_1P7V[2] = { PMU_EFUSE_PAGE_DCCALIB2_L, PMU_EFUSE_PAGE_DCCALIB2_R, };
    static const uint8_t EFUSE_PAGE_DIFF_1P95V[2] = { PMU_EFUSE_PAGE_DCCALIB_L, PMU_EFUSE_PAGE_DCCALIB_R, };
    const uint8_t *page;
    uint16_t efuse;

    union DC_EFUSE_T {
        struct DC_VALUE_T {
            int16_t dc          : DAC_DC_CALIB_BIT_WIDTH;
            uint16_t checksum   : (16 - DAC_DC_CALIB_BIT_WIDTH);
        } val;
        uint16_t reg;
    };
    union DC_EFUSE_T dc;

    if (vcodec_mv >= 1900) {
        page = EFUSE_PAGE_DIFF_1P95V;
    } else {
        page = EFUSE_PAGE_DIFF_1P7V;
    }

    pmu_get_efuse(page[0], &efuse);
    if (dc_calib_checksum_valid(efuse)) {
        TRACE("Dc calib L OK: 0x%04x", efuse);
        dc.reg = efuse;
        *dc_l = dc.val.dc;
    } else {
        TRACE("Warning: Bad dc calib efuse L: 0x%04x", efuse);
        *dc_l = 0;
    }

    pmu_get_efuse(page[1], &efuse);
    if (dc_calib_checksum_valid(efuse)) {
        TRACE("Dc calib R OK: 0x%04x", efuse);
        dc.reg = efuse;
        *dc_r = dc.val.dc;
    } else {
        TRACE("Warning: Bad dc calib efuse R: 0x%04x", efuse);
        *dc_r = 0;
    }

    TRACE("ANA: DC CALIB L=%d R=%d", *dc_l, *dc_r);

#if defined(ANA_DC_CALIB_L) || defined(ANA_DC_CALIB_R)
#ifdef ANA_DC_CALIB_L
    *dc_l = ANA_DC_CALIB_L;
#endif
#ifdef ANA_DC_CALIB_R
    *dc_r = ANA_DC_CALIB_R;
#endif
    TRACE("ANA: OVERRIDE DC CALIB L=%d R=%d", *dc_l, *dc_r);
#endif

    return;
}

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
static void analog_aud_dc_calib_init(void)
{
    uint16_t val;
    int16_t dc_l, dc_r;

    analog_aud_get_dc_calib_value(&dc_l, &dc_r);

    analog_read(ANA_REG_6F, &val);
    val = SET_BITFIELD(val, REG_CODEC_TX_EAR_OFF_BITL, dc_l);
    analog_write(ANA_REG_6F, val);

    analog_read(ANA_REG_70, &val);
    val = SET_BITFIELD(val, REG_CODEC_TX_EAR_OFF_BITR, dc_r);
    analog_write(ANA_REG_70, val);
}

static void analog_aud_dc_calib_enable(bool en)
{
    uint16_t val;

    analog_read(ANA_REG_6E, &val);
    val |= REG_CODEC_TX_EAR_OFFEN;
    analog_write(ANA_REG_6E, val);
}
#endif

static void analog_aud_vcodec_enable(enum ANA_CODEC_USER_T user, bool en)
{
    uint32_t lock;
    bool set = false;

    lock = int_lock();
    if (en) {
        if (vcodec_map == 0) {
            set = true;
        }
        vcodec_map |= (1 << user);
    } else {
        vcodec_map &= ~(1 << user);
        if (vcodec_map == 0) {
            set = true;
        }
    }
    int_unlock(lock);

    if (set) {
        pmu_codec_config(!!vcodec_map);
    }
}

static void analog_aud_enable_common_internal(enum ANA_CODEC_USER_T user, bool en)
{
    uint32_t lock;
    uint16_t val_68;
    bool set = false;

    lock = int_lock();
    if (en) {
        if (codec_common_map == 0) {
            set = true;
        }
        codec_common_map |= (1 << user);
    } else {
        codec_common_map &= ~(1 << user);
        if (codec_common_map == 0) {
            set = true;
        }
    }
    int_unlock(lock);

    if (set) {
        analog_read(ANA_REG_68, &val_68);
        if (codec_common_map) {
            val_68 |= REG_CODEC_EN_VCM;
        } else {
            val_68 &= ~(REG_CODEC_EN_VCM | REG_CODEC_VCM_EN_LPF);
        }
        if (codec_common_map) {
            // Quick startup:
            // 1) Disable VCM LPF and target to a higher voltage than the required one
            // 2) Wait for a short time when VCM is in quick charge (high voltage)
            // 3) Enable VCM LPF and target to the required VCM LPF voltage
            analog_write(ANA_REG_68, SET_BITFIELD(val_68, REG_CODEC_VCM_LOW_VCM, 0));
            uint32_t delay;

#if defined(VCM_CAP_100NF)
            if (vcodec_mv >= 2500) {
                delay = 6;
            } else {
                delay = 10;
            }
#else
            if (vcodec_mv >= 2500) {
                delay = 10;
            } else {
                delay = 50;
            }
#endif
            osDelay(delay);
#if 0
            // Target to a voltage near the required one
            analog_write(ANA_REG_68, val_68);
            osDelay(10);
#endif
            val_68 |= REG_CODEC_VCM_EN_LPF;
        }
        analog_write(ANA_REG_68, val_68);
    }
}

static void analog_aud_enable_codec_common(enum ANA_CODEC_USER_T user, bool en)
{
#ifndef VCM_ON
    analog_aud_enable_common_internal(user, en);
#endif
}

static void analog_aud_enable_adda_common(enum ANA_CODEC_USER_T user, bool en)
{
    uint32_t lock;
#if PU_TX_REGULATOR
    uint16_t val_72;
#endif
    uint16_t val_67, val_68;
    bool set = false;

    lock = int_lock();
    if (en) {
        if (adda_common_map == 0) {
            set = true;
        }
        adda_common_map |= (1 << user);
    } else {
        adda_common_map &= ~(1 << user);
        if (adda_common_map == 0) {
            set = true;
        }
    }
    int_unlock(lock);

    if (set) {
        analog_read(ANA_REG_67, &val_67);
        analog_read(ANA_REG_68, &val_68);
#if PU_TX_REGULATOR
        analog_read(ANA_REG_72, &val_72);
#endif
        if (adda_common_map) {
            val_67 |= REG_CODEC_EN_BIAS;
            analog_write(ANA_REG_67, val_67);
            val_68 |= REG_CODEC_EN_VCM_BUFFER;
#if PU_TX_REGULATOR
            val_72 |= REG_PU_TX_REGULATOR;
            analog_write(ANA_REG_72, val_72);
            hal_sys_timer_delay_us(10);
            val_72 &= ~REG_BYPASS_TX_REGULATOR;
            analog_write(ANA_REG_72, val_72);
#endif
        } else {
#if PU_TX_REGULATOR
            val_72 |= REG_BYPASS_TX_REGULATOR;
            analog_write(ANA_REG_72, val_72);
            val_72 &= ~REG_PU_TX_REGULATOR;
            analog_write(ANA_REG_72, val_72);
#endif
            val_68 &= ~(REG_CODEC_EN_VCM_BUFFER);
            val_67 &= ~REG_CODEC_EN_BIAS;
            analog_write(ANA_REG_67, val_67);
        }
        analog_write(ANA_REG_68, val_68);
    }
}

static void analog_aud_enable_vmic(enum ANA_CODEC_USER_T user, uint32_t dev)
{
    uint32_t lock;
    CODEC_USER_MAP old_map;
    bool set = false;
    int i;
    uint32_t pmu_map = 0;

    lock = int_lock();

    for (i = 0; i < MAX_ANA_MIC_CH_NUM; i++) {
        if (dev & (AUD_VMIC_MAP_VMIC1 << i)) {
            if (vmic_map[i] == 0) {
                set = true;
            }
            vmic_map[i] |= (1 << user);
        } else {
            old_map = vmic_map[i];
            vmic_map[i] &= ~(1 << user);
            if (old_map != 0 && vmic_map[i] == 0) {
                set = true;
            }
        }

        if (vmic_map[i]) {
            pmu_map |= (AUD_VMIC_MAP_VMIC1 << i);
        }
    }

    int_unlock(lock);

    if (set) {
        pmu_codec_mic_bias_enable(pmu_map);
        if (pmu_map) {
            osDelay(1);
        }
    }
}

uint32_t analog_aud_get_max_dre_gain(void)
{
    if (vcodec_mv >= 2800) {
        return 0xC; // 6=-4dB
    } else if (vcodec_mv >= 2500) {
        return 0x11; // 0xB=-4dB
    } else if (vcodec_mv >= 1800) {
        return 0xE; // -4dB
    } else {
        return 0xF;
    }
}

int analog_reset(void)
{
    return 0;
}

void analog_open(void)
{
    uint16_t val;

    val = REG_CODEC_ADCA_GAIN_BIT(3)|REG_CODEC_ADCA_CH_SEL(1)|CFG_RESET_PGAA_DR;
    analog_write(ANA_REG_61, val);

#if 0
    val = REG_CODEC_ADC_IBSEL_REG(8) | REG_CODEC_ADC_IBSEL_VCOMP(8) | REG_CODEC_ADC_IBSEL_VREF(8) | REG_CODEC_ADC_LPFVCM_SW(7);
    analog_write(ANA_REG_64, val);
#endif

    val = REG_CODEC_ADC_OP1_IBIT(2) | REG_CODEC_ADC_VREF_SEL(7);
    analog_write(ANA_REG_65, val);

    val = REG_CODEC_BIAS_IBSEL_VOICE(8);
    if (vcodec_mv >= 2800) {
        val |= REG_CODEC_BIAS_IBSEL(6) | REG_CODEC_BIAS_IBSEL_TX(5);
    } else if (vcodec_mv >= 2500) {
        val |= REG_CODEC_BIAS_IBSEL(8) | REG_CODEC_BIAS_IBSEL_TX(8);
    } else if (vcodec_mv >= 1800) {
        val |= REG_CODEC_BIAS_IBSEL(8) | REG_CODEC_BIAS_IBSEL_TX(0xC);
    } else {
        val |= REG_CODEC_BIAS_IBSEL(8) | REG_CODEC_BIAS_IBSEL_TX(3);
    }
    analog_write(ANA_REG_66, val);

    if (vcodec_mv >= 1800) {
        val = REG_CODEC_ADC_REG_VSEL(2) | REG_CODEC_BUF_LOWVCM(4);
    } else {
        val = REG_CODEC_ADC_REG_VSEL(7) | REG_CODEC_BUF_LOWVCM(4)
                | REG_CODEC_EN_TX_EXT | REG_CODEC_DAC_CLK_EDGE_SEL;
    }
    analog_write(ANA_REG_67, val);

    uint16_t vcm, vcm_lpf;
    if (vcodec_mv >= 1800) {
        vcm = vcm_lpf = 7;
    } else {
        vcm = vcm_lpf = 7;
    }
    val = REG_CODEC_VCM_LOW_VCM(vcm) | REG_CODEC_VCM_LOW_VCM_LP(vcm) | REG_CODEC_VCM_LOW_VCM_LPF(vcm_lpf);
    analog_write(ANA_REG_68, val);

    val = REG_CODEC_RX_VTOI_I_DAC2(4);
    if (vcodec_mv >= 2500) {
        val |= REG_CODEC_RX_VTOI_IDAC_SEL(7) | REG_CODEC_RX_VTOI_VCS_SEL(0xC);
    } else if (vcodec_mv >= 1800) {
        val |= REG_CODEC_RX_VTOI_IDAC_SEL(7) | REG_CODEC_RX_VTOI_VCS_SEL(0xC);
    } else {
        val |= REG_CODEC_RX_VTOI_IDAC_SEL(7) | REG_CODEC_RX_VTOI_VCS_SEL(0x18);
    }
    analog_write(ANA_REG_69, val);

    if (vcodec_mv >= 2800) {
        val = REG_CODEC_TX_DAC_VREF_L(7) | REG_CODEC_TX_DAC_VREF_R(7);
    } else if (vcodec_mv >= 2500) {
        val = REG_CODEC_TX_DAC_VREF_L(1) | REG_CODEC_TX_DAC_VREF_R(1);
    } else if (vcodec_mv >= 1800) {
        val = REG_CODEC_TX_DAC_VREF_L(8) | REG_CODEC_TX_DAC_VREF_R(8);
    } else {
        val = REG_CODEC_TX_DAC_VREF_L(2) | REG_CODEC_TX_DAC_VREF_R(2);
    }
    analog_write(ANA_REG_6A, val);

    if (vcodec_mv >= 2500) {
        val = 0;
    } else if (vcodec_mv >= 1800) {
        val = REG_CODEC_TX_EAR_COMP_L(0x18) | REG_CODEC_TX_EAR_COMP_R(0x18);
    } else {
        val = 0;
    }
    analog_write(ANA_REG_6B, val);
    
    val = REG_CODEC_TX_EAR_FBCAP(3) | REG_CODEC_TX_EAR_IBSEL(1);
    if (vcodec_mv >= 1800) {
        val |= REG_CODEC_TX_EAR_DR_ST(2);
    } else {
        val |= REG_CODEC_TX_EAR_DR_ST(1);
    }
    analog_write(ANA_REG_6C, val);

    uint16_t dre_gain = analog_aud_get_max_dre_gain();
    val = REG_CODEC_TX_EAR_DRE_GAIN_L(dre_gain) | REG_CODEC_TX_EAR_DRE_GAIN_R(dre_gain) | REG_CODEC_TX_EAR_GAIN(1) |
        DRE_GAIN_SEL_L | DRE_GAIN_SEL_R;
#ifdef DAC_DRE_GAIN_DC_UPDATE
    val |= REG_CODEC_TX_EAR_DRE_GAIN_L_UPDATE | REG_CODEC_TX_EAR_DRE_GAIN_R_UPDATE;
#endif
    analog_write(ANA_REG_6D, val);

    val = REG_CODEC_TX_EAR_SOFTSTART(8);
    if (vcodec_mv >= 2500) {
        val |= REG_CODEC_TX_EAR_OCEN | REG_CODEC_TX_EAR_OUTPUTSEL(4);
    } else if (vcodec_mv >= 1800) {
        val |= REG_CODEC_TX_EAR_OCEN | REG_CODEC_TX_EAR_OUTPUTSEL(1);
    } else {
        val |= REG_CODEC_TX_EAR_OUTPUTSEL(4);
    }
    analog_write(ANA_REG_6E, val);

    val = REG_DAC_LDO0P9_VSEL(2) | REG_BYPASS_TX_REGULATOR | REG_CODEC_TX_EAR_VCM_SEL;
    if (vcodec_mv >= 1800) {
        val |= REG_TX_REGULATOR_BIT(5) | REG_CODEC_TX_EAR_VCM_BIT(2);
    } else {
        val |= REG_TX_REGULATOR_BIT(4) | REG_CODEC_TX_EAR_VCM_BIT(3);
    }
    analog_write(ANA_REG_72, val);

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
    analog_aud_dc_calib_init();
    analog_aud_dc_calib_enable(true);
#endif

#ifdef VCM_ON
    analog_aud_enable_common_internal(ANA_CODEC_USER_DAC, true);
#endif
}

void analog_sleep(void)
{
}

void analog_wakeup(void)
{
}

void analog_aud_codec_anc_enable(enum ANC_TYPE_T type, bool en)
{
}

void analog_aud_codec_anc_init_close(void)
{
}

void analog_aud_mickey_enable(bool en)
{
    if (en) {
        analog_aud_vcodec_enable(ANA_CODEC_USER_MICKEY, true);
        analog_aud_enable_codec_common(ANA_CODEC_USER_MICKEY, true);
        analog_aud_enable_vmic(ANA_CODEC_USER_MICKEY, CFG_HW_AUD_MICKEY_DEV);
    } else {
        analog_aud_enable_vmic(ANA_CODEC_USER_MICKEY, 0);
        analog_aud_enable_codec_common(ANA_CODEC_USER_MICKEY, false);
        analog_aud_vcodec_enable(ANA_CODEC_USER_MICKEY, false);
    }
}

void analog_aud_codec_adc_enable(enum AUD_IO_PATH_T input_path, enum AUD_CHANNEL_MAP_T ch_map, bool en)
{
    uint32_t dev;

    if (en) {
        dev = hal_codec_get_input_path_cfg(input_path);
        // Enable vmic first to overlap vmic stable time with codec vcm stable time
        analog_aud_enable_vmic(ANA_CODEC_USER_ADC, dev);
        analog_aud_enable_codec_common(ANA_CODEC_USER_ADC, true);
        analog_aud_enable_adda_common(ANA_CODEC_USER_ADC, true);
        analog_aud_set_adc_gain(input_path, ch_map);
        analog_aud_enable_adc(ANA_CODEC_USER_ADC, ch_map, true);
    } else {
        analog_aud_enable_adc(ANA_CODEC_USER_ADC, ch_map, false);
        analog_aud_enable_adda_common(ANA_CODEC_USER_ADC, false);
        analog_aud_enable_codec_common(ANA_CODEC_USER_ADC, false);
        analog_aud_enable_vmic(ANA_CODEC_USER_ADC, 0);
    }
}

static void analog_aud_codec_config_speaker(void)
{
    bool en;

    if (ana_spk_req && !ana_spk_muted) {
        en = true;
    } else {
        en = false;
    }

    if (ana_spk_enabled != en) {
        ana_spk_enabled = en;
        if (en) {
            analog_aud_enable_dac_pa(CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV);
        } else {
            analog_aud_enable_dac_pa(0);
        }
    }
}

void analog_aud_codec_speaker_enable(bool en)
{
    ana_spk_req = en;
    analog_aud_codec_config_speaker();
}

void analog_aud_codec_dac_enable(bool en)
{
    if (en) {
        analog_aud_enable_codec_common(ANA_CODEC_USER_DAC, true);
        analog_aud_enable_adda_common(ANA_CODEC_USER_DAC, true);
        analog_aud_enable_dac(CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV);
#if !defined(AUDIO_OUTPUT_DC_CALIB_ANA) && !defined(AUDIO_OUTPUT_DC_CALIB)
        osDelay(1);
        analog_aud_codec_speaker_enable(true);
#endif
    } else {
#if !defined(AUDIO_OUTPUT_DC_CALIB_ANA) && !defined(AUDIO_OUTPUT_DC_CALIB)
        analog_aud_codec_speaker_enable(false);
        osDelay(1);
#endif
        analog_aud_enable_dac(0);
        analog_aud_enable_adda_common(ANA_CODEC_USER_DAC, false);
        analog_aud_enable_codec_common(ANA_CODEC_USER_DAC, false);
    }
}

void analog_aud_codec_open(void)
{
    analog_aud_vcodec_enable(ANA_CODEC_USER_CODEC, true);

#ifdef _AUTO_SWITCH_POWER_MODE__
    //pmu_mode_change(PMU_POWER_MODE_DIG_DCDC);
#endif
}

void analog_aud_codec_close(void)
{
    static const enum AUD_CHANNEL_MAP_T all_ch = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1 |
        AUD_CHANNEL_MAP_CH2 | AUD_CHANNEL_MAP_CH3 | AUD_CHANNEL_MAP_CH4;

    analog_aud_codec_speaker_enable(false);
    osDelay(1);
    analog_aud_codec_dac_enable(false);

    analog_aud_codec_adc_enable(AUD_IO_PATH_NULL, all_ch, false);

#ifdef _AUTO_SWITCH_POWER_MODE__
    //pmu_mode_change(PMU_POWER_MODE_ANA_DCDC);
#endif

    analog_aud_vcodec_enable(ANA_CODEC_USER_CODEC, false);
}

void analog_aud_codec_mute(void)
{
#ifndef AUDIO_OUTPUT_DC_CALIB
    //analog_codec_tx_pa_gain_sel(0);
#endif

    ana_spk_muted = true;
    analog_aud_codec_config_speaker();
}

void analog_aud_codec_nomute(void)
{
    ana_spk_muted = false;
    analog_aud_codec_config_speaker();

#ifndef AUDIO_OUTPUT_DC_CALIB
    //analog_aud_set_dac_gain(dac_gain);
#endif
}

int analog_debug_config_audio_output(bool diff)
{
    return 0;
}

int analog_debug_config_codec(uint16_t mv)
{
#ifdef ANC_PROD_TEST
    int ret;

    ret = pmu_debug_config_codec(mv);
    if (ret) {
        return ret;
    }
 
    vcodec_mv = mv;
#endif
    return 0;
}

int analog_debug_config_low_power_adc(bool enable)
{
    return 0;
}

void analog_aud_pll_tune(float ratio)
{
}

