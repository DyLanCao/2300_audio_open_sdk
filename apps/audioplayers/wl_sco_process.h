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
#ifndef __WL_SCO_RPOCESS_H__
#define __WL_SCO_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

int app_sco_in_pcmbuff_init(uint8_t *buff, uint16_t len);
int app_sco_in_pcmbuff_put(uint8_t *buff, uint16_t len);
int app_sco_in_pcmbuff_get(uint8_t *buff, uint16_t len);

int wl_sco_process_init(uint32_t sco_sample_rate);
int wl_sco_tx_process_run(short *in_frame,short *ref_frame,uint32_t frame_len);
int wl_sco_rx_process_run(short *in_frame,uint32_t frame_len);
int wl_sco_process_exit(void);

#ifdef __cplusplus
}
#endif

#endif