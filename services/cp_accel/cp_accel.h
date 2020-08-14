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
#ifndef __CP_ACCEL_H__
#define __CP_ACCEL_H__

#include "plat_types.h"
#include "hal_location.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int (*CP_ACCEL_CP_MAIN)(void);

typedef unsigned int (*CP_ACCEL_EVT_HDLR)(uint32_t event);

int cp_accel_open(CP_ACCEL_CP_MAIN cp_main, CP_ACCEL_EVT_HDLR cp_hdlr, CP_ACCEL_EVT_HDLR mcu_hdlr);

int cp_accel_close(void);

int cp_accel_init_done(void);

int cp_accel_send_event_mcu2cp(uint32_t event);

int cp_accel_send_event_cp2mcu(uint32_t event);

#ifdef __cplusplus
}
#endif

#endif

