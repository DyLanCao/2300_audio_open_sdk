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


#ifndef APP_SMARTVOICE_H_
#define APP_SMARTVOICE_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief Smart Voice Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_VOICEPATH)

#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
#ifdef __cplusplus
extern "C" {
#endif

#define SV_BLE_CONNECTION_INTERVAL_MIN_IN_MS			10
#define SV_BLE_CONNECTION_INTERVAL_MAX_IN_MS			20
#define SV_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS		20000
#define SV_BLE_CONNECTION_SLAVELATENCY					0

extern struct app_env_tag app_env;

/// health thermometer application environment structure
struct app_sv_env_tag
{
    uint8_t connectionIndex;
	uint8_t	isCmdNotificationEnabled;
	uint8_t	isDataNotificationEnabled;
	uint16_t mtu[BLE_CONNECTION_MAX];
};

typedef void(*app_sv_tx_done_t)(void);

typedef void(*app_sv_activity_stopped_t)(void);

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/// Health Thermomter Application environment
extern struct app_sv_env_tag app_sv_env;

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */
void app_sv_send_cmd_via_notification(uint8_t* ptrData, uint32_t length);

void app_sv_send_cmd_via_indication(uint8_t* ptrData, uint32_t length);

void app_sv_send_data_via_notification(uint8_t* ptrData, uint32_t length);

void app_sv_send_data_via_indication(uint8_t* ptrData, uint32_t length);

void app_sv_register_tx_done(app_sv_tx_done_t callback);

void app_sv_update_conn_parameter(uint8_t conidx, uint32_t min_interval_in_ms, uint32_t max_interval_in_ms,
		uint32_t supervision_timeout_in_ms, uint8_t slaveLantency);

#ifdef __cplusplus
	}
#endif


#endif //(BLE_APP_VOICEPATH)

/// @} APP

#endif // APP_SMARTVOICE_H_
