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
#ifndef APP_GFPS_H_
#define APP_GFPS_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 *
 * @brief Device Information Application Module Entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW Configuration

#if (BLE_APP_GFPS)

#include <stdint.h>
#ifndef BLE_BD_ADDR_T
///BD Address structure
typedef struct
{
    ///6-byte array address value
    uint8_t  addr[6];
} bd_addr_t;
#endif
/*
 * DEFINES
 ****************************************************************************************
 */

/// Manufacturer Name Value
#define APP_GFPS_MANUFACTURER_NAME       ("RivieraWaves SAS")
#define APP_GFPS_MANUFACTURER_NAME_LEN   (16)

/// Model Number String Value
#define APP_GFPS_MODEL_NB_STR            ("RW-BLE-1.0")
#define APP_GFPS_MODEL_NB_STR_LEN        (10)

/// Serial Number
#define APP_GFPS_SERIAL_NB_STR           ("1.0.0.0-LE")
#define APP_GFPS_SERIAL_NB_STR_LEN       (10)

/// Firmware Revision
#define APP_GFPS_FIRM_REV_STR            ("6.1.2")
#define APP_GFPS_FIRM_REV_STR_LEN        (5)

/// System ID Value - LSB -> MSB
#define APP_GFPS_SYSTEM_ID               ("\x12\x34\x56\xFF\xFE\x9A\xBC\xDE")
#define APP_GFPS_SYSTEM_ID_LEN           (8)

/// Hardware Revision String
#define APP_GFPS_HARD_REV_STR           ("1.0.0")
#define APP_GFPS_HARD_REV_STR_LEN       (5)

/// Software Revision String
#define APP_GFPS_SW_REV_STR              ("6.3.0")
#define APP_GFPS_SW_REV_STR_LEN          (5)

/// IEEE
#define APP_GFPS_IEEE                    ("\xFF\xEE\xDD\xCC\xBB\xAA")
#define APP_GFPS_IEEE_LEN                (6)

/**
 * PNP ID Value - LSB -> MSB
 *      Vendor ID Source : 0x02 (USB Implementer’s Forum assigned Vendor ID value)
 *      Vendor ID : 0x045E      (Microsoft Corp)
 *      Product ID : 0x0040
 *      Product Version : 0x0300
 */
#define APP_GFPS_PNP_ID               ("\x02\x5E\x04\x40\x00\x00\x03")
#define APP_GFPS_PNP_ID_LEN           (7)

#if (BLE_APP_HID)
#define APP_GFPS_FEATURES             (GFPSP_MANUFACTURER_NAME_CHAR_SUP |\
                                      GFPSP_MODEL_NB_STR_CHAR_SUP      |\
                                      GFPSP_SYSTEM_ID_CHAR_SUP         |\
                                      GFPSP_PNP_ID_CHAR_SUP)
#else
#define APP_GFPS_FEATURES             (GFPSP_ALL_FEAT_SUP)
#endif //(BLE_APP_HID)
typedef void (*gfps_enter_pairing_mode)(void);
typedef uint8_t (*gfps_bt_io_cap_set)(uint8_t mode);

struct app_gfps_env_tag
{
    uint8_t connectionIndex;
	uint8_t	isKeyBasedPairingNotificationEnabled;
	uint8_t	isPassKeyNotificationEnabled;
    bd_addr_t seeker_bt_addr;
    bd_addr_t local_le_addr;
    bd_addr_t local_bt_addr;
    uint8_t keybase_pair_key[16];
    gfps_enter_pairing_mode enter_pairing_mode;
    gfps_bt_io_cap_set     bt_set_iocap;
    uint8_t bt_iocap;
    uint8_t pendingLastResponse[16];
    uint8_t isLastResponsePending;
};

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */

/// Table of message handlers
extern const struct ke_state_handler app_gfps_table_handler;

/*
 * GLOBAL FUNCTIONS DECLARATION
 ****************************************************************************************
 */
#ifdef __cplusplus
	extern "C" {
#endif

/**
 ****************************************************************************************
 * @brief Initialize Device Information Service Application
 ****************************************************************************************
 */
void app_gfps_init(void);

/**
 ****************************************************************************************
 * @brief Add a Device Information Service instance in the DB
 ****************************************************************************************
 */
void app_gfps_add_gfps(void);

/**
 ****************************************************************************************
 * @brief Enable the Device Information Service
 ****************************************************************************************
 */
void app_gfps_enable_prf(uint16_t conhdl);

void app_gfps_connected_evt_handler(uint8_t conidx);
void app_gfps_disconnected_evt_handler(uint8_t conidx);

void app_gfps_set_bt_access_mode(gfps_enter_pairing_mode cb);
void app_gfps_set_io_cap(gfps_bt_io_cap_set cb);

void app_gfps_generate_accountkey_filter(uint8_t* accountKeyFilter, uint8_t* filterSize);

uint8_t app_gfps_generate_accountkey_data(uint8_t* outputData);
void app_exit_fastpairing_mode(void);


uint8_t* app_gfps_get_last_response(void);

void app_gfps_enter_connectable_mode_req_handler(uint8_t* response);

bool app_gfps_is_last_response_pending(void);
#ifdef __cplusplus
}
#endif

#endif //BLE_APP_GFPS

/// @} APP

#endif //APP_GFPS_H_
