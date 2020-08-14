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
#ifndef __BT_IF_H_
#define __BT_IF_H_
#include "btif_sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif                          /*  */

    enum pair_event {
        PAIR_EVENT_NUMERIC_REQ,
        PAIR_EVENT_COMPLETE,
    };

    typedef void (*pairing_callback_t) (enum pair_event evt, int err_code);
    typedef void (*authing_callback_t) (void);

    int bt_stack_initilize(void);
    int bt_pairing_init(pairing_callback_t pair_cb);
#ifdef BTIF_SECURITY
    int bt_authing_init(authing_callback_t auth_cb);
#else
    static inline int bt_authing_init(authing_callback_t auth_cb) {
        return 0;
    }
#endif
    int a2dp_codec_init(void);
    int bt_stack_config(void);
    int bt_set_local_dev_name(const unsigned char *dev_name, u8 len);
    void bt_process_stack_events(void);

    uint8_t bt_get_sco_number(void);
    void bt_set_sco_number(uint8_t sco_num);
	
#if defined(IBRT)
uint32_t btif_save_app_bt_device_ctx(uint8_t *ctx_buffer,uint8_t psm_context_mask);
uint32_t btif_set_app_bt_device_ctx(uint8_t *ctx_buffer,uint8_t psm_context_mask,uint8_t bt_devices_idx, uint8_t rm_detbl_idx, uint8_t avd_ctx_device_idx);
#endif

#ifdef __cplusplus
}
#endif /*  */

#endif /*__BT_IF_H_*/
