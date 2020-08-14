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
#ifndef __BT_SCO_CHAIN_H__
#define __BT_SCO_CHAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

int speech_init(int tx_sample_rate, int rx_sample_rate,
                     int tx_frame_ms, int rx_frame_ms,
                     int sco_frame_ms,
                     uint8_t *buf, int len);
int speech_deinit(void);

int speech_tx_process(short *pcm_buf, short *ref_buf, int *pcm_len);
int speech_rx_process(short *pcm_buf, int *pcm_len);

void *speech_get_ext_buff(int size);

#ifdef __cplusplus
}
#endif

#endif