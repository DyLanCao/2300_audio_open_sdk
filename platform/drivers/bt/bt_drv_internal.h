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
#ifndef __BT_DRV_INTERNAL_H__
#define __BT_DRV_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"

uint8_t btdrv_rf_init(void);
void btdrv_test_mode_rf_txpwr_init(void);
void btdrv_ins_patch_init(void);
void btdrv_data_patch_init(void);
void btdrv_patch_en(uint8_t en);
void btdrv_config_init(void);
void btdrv_testmode_config_init(void);
void btdrv_bt_spi_rawbuf_init(void);
void btdrv_bt_spi_xtal_init(void);
void btdrv_sync_config(void);
void btdrv_rf_rx_gain_adjust_req(uint32_t user, bool lowgain);
#ifdef BT_50_FUNCTION
void btdrv_config_init_ble5(void);
void btdrv_ins_patch_init_50(void);
void btdrv_data_patch_init_50(void);

#endif
#ifdef __cplusplus
}
#endif

#endif
