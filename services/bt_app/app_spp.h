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
#ifndef __INTERCONNECTION__

#ifndef __APP_SPP_H__
#define __APP_SPP_H__

#define SPP_RECV_BUFFER_SIZE   3072

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*app_spp_tx_done_t)(void);
void app_spp_register_tx_done(app_spp_tx_done_t callback);
void app_spp_init(void);
uint32_t app_spp_read_data(uint8_t* buff, uint16_t* maxBytes);
void app_sv_send_cmd_via_spp(uint8_t* ptrData, uint32_t length);
void app_sv_send_data_via_spp(uint8_t* ptrData, uint32_t length);

uint16_t app_spp_tx_buf_size(void);
void app_spp_init_tx_buf(uint8_t* ptr);
uint8_t* app_spp_fill_data_into_tx_buf(uint8_t* ptrData, uint32_t dataLen);


#ifdef __cplusplus
}
#endif

#endif

#endif
