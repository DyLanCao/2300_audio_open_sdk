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
#ifndef __HAL_I2S_TDM_H__
#define __HAL_I2S_TDM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define I2S_TDM_BUF_ALIGN __attribute__((aligned(0x100)))
#define I2S_TDM_FRAME_SIZE_MAX   128
#define I2S_TDM_FRAME_NUM_MAX    4
#define I2S_TDM_FRAME_SIZE       64
#define I2S_TDM_FRAME_NUM        2
#define I2S_TDM_TX_FRAME_NUM     1
#define I2S_TDM_TX_FRAME_SIZE    8

int32_t hal_i2s_tdm_open(enum HAL_I2S_ID_T i2s_id,enum AUD_SAMPRATE_T sample_rate,HAL_DMA_IRQ_HANDLER_T rx_handler,uint32_t rx_frame_num,uint8_t *rx_buff,uint32_t rx_buff_len);
int32_t hal_i2s_tdm_close(enum HAL_I2S_ID_T i2s_id);

#ifdef __cplusplus
}
#endif

#endif // __HAL_I2S_TDM_H__
