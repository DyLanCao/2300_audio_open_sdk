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
#ifndef __APP_TOTA_H__
#define __APP_TOTA_H__

#include "app_tota_cmd_code.h"

#define APP_TOTA_PATH_TYPE_SPP	(1 << 0)

#define APP_TOTA_PATH_TYPE		(APP_TOTA_PATH_TYPE_SPP)
	
#define APP_TOTA_CONNECTED             (1 << 0)    
#define APP_TOTA_DISCONNECTED          (~(1 << 0))

#ifdef __cplusplus
extern "C" {
#endif

void app_tota_init(void);
void app_tota_connected(uint8_t connType);
void app_tota_disconnected(uint8_t disconnType);
bool app_is_in_tota_mode(void);
void app_tota_update_datapath(APP_TOTA_TRANSMISSION_PATH_E dataPath);
APP_TOTA_TRANSMISSION_PATH_E app_tota_get_datapath(void);

#ifdef __cplusplus
}
#endif

#endif

