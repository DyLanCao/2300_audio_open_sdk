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
#ifndef __ANALOG_BEST3001_H__
#define __ANALOG_BEST3001_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ISPI_ANA_REG(reg)                   (((reg) & 0xFFF) | 0x1000)      // cs = 1, analog interface

#ifdef __1801_SUPPT__
#define ISPI_ANA1801_REG(reg)               (((reg) & 0xFFF) | 0x3000)      // cs = 3, external

#define analog1801_read(reg, val)       hal_analogif_reg_read(ISPI_ANA1801_REG(reg),val)
#define analog1801_write(reg, val)      hal_analogif_reg_write(ISPI_ANA1801_REG(reg),val)
#endif

#define MAX_ANA_MIC_CH_NUM                  2

void analog_dco_fine_tune_stop (void); 
void analog_dco_fine_tune_start (void);

void analog_aud_pll_set_dig_div(uint32_t);

void analog_set_dco_fine_tune_code (uint8_t with_temp_calib);

#ifdef __1801_SUPPT__
uint8_t analog1801_version (void);
#endif

uint8_t is_3001s_chip_d (void);

#ifdef EXTERN_HEADSET

typedef enum {
    EXT_HS_NOTIFY_EVT_NULL,
    EXT_HS_NOTIFY_EVT_PLUGIN,
    EXT_HS_NOTIFY_EVT_PLUGOUT,
    EXT_HS_NOTIFY_EVT_KEY_DN,
    EXT_HS_NOTIFY_EVT_KEY_UP,

    EXT_HS_NOTIFY_EVT_NUM,
} EXT_HS_NOTIFY_EVT_E;

typedef enum {
    EXT_HS_KEY_UNKNOWN,
    EXT_HS_KEY_VOL_PLUS,
    EXT_HS_KEY_VOL_MINUS,
    EXT_HS_KEY_FUNC,

    EXT_HS_KEY_NUM
} EXT_HS_KEY_E;

typedef enum {
    EXT_HS_TYPE_NO_HS,
    EXT_HS_TYPE_UNKNOWN,
    EXT_HS_TYPE_THREE_PORT,
    EXT_HS_TYPE_CTIA,
    EXT_HS_TYPE_OMTP,

    EXT_HS_TYPE_NUM
} EXT_HS_TYPE_E;

#define EXT_HS_TYPE_SHIFT       (0)
#define EXT_HS_TYPE_MASK        (0xf<<EXT_HS_TYPE_SHIFT)
#define EXT_HS_TYPE_SET(reg, val)   (((reg)&(~EXT_HS_TYPE_MASK)) | (((val)<<EXT_HS_TYPE_SHIFT)&EXT_HS_TYPE_MASK))
#define EXT_HS_TYPE_GET(reg)    (((reg)&EXT_HS_TYPE_MASK)>>EXT_HS_TYPE_SHIFT)

typedef enum {
    EXT_HS_IMP_UNKNOWN,
    EXT_HS_IMP_16_OHM,   // 16 ohm
    EXT_HS_IMP_32_OHM,   // 32 ohm
    EXT_HS_IMP_600_OHM,  // 600 ohm

    EXT_HS_IMP_NUM
} EXT_HS_IMP_TYPE_E;

#define EXT_HS_IMP_TYPE_SHIFT       (4)
#define EXT_HS_IMP_TYPE_MASK        (0xf<<EXT_HS_IMP_TYPE_SHIFT)
#define EXT_HS_IMP_TYPE_SET(reg, val)   (((reg)&(~EXT_HS_IMP_TYPE_MASK)) | (((val)<<EXT_HS_IMP_TYPE_SHIFT)&EXT_HS_IMP_TYPE_MASK))
#define EXT_HS_IMP_TYPE_GET(reg)    (((reg) & EXT_HS_IMP_TYPE_MASK) >> EXT_HS_IMP_TYPE_SHIFT)

typedef union {
    uint32_t    ext_hs_info;    // used for EXT_HS_NOTIFY_EVT_PLUGIN
    EXT_HS_KEY_E para;         // used for EXT_HS_NOTIFY_EVT_KEY_DN/EXT_HS_NOTIFY_EVT_KEY_UP
} EXT_HS_NOTIFY_EVT_PARAM_U;

typedef void (*FUNC_EXT_HS_IRQ_HDL) (EXT_HS_NOTIFY_EVT_E, EXT_HS_NOTIFY_EVT_PARAM_U);
void analog_ext_hs_irq_hdl_set(FUNC_EXT_HS_IRQ_HDL hdl);

#endif  // EXTERN_HEADSET

#ifdef __cplusplus
}
#endif

#endif

