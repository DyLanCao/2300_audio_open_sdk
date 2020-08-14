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
#ifndef __APP_BLE_MODE_SWITCH_H__
#define __APP_BLE_MODE_SWITCH_H__
#include "bluetooth.h"
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__INTERCONNECTION__)
#define BLE_FAST_ADVERTISING_INTERVAL               (32) // 20ms (32*0.625ms)
#define BLE_ADVERTISING_INTERVAL                    (160) // 100ms (160*0.625ms)
#define APP_INTERCONNECTION_FAST_ADV_TIMEOUT_IN_MS  60000 // 1MIN
#define APP_INTERCONNECTION_SLOW_ADV_TIMEOUT_IN_MS  120000 // 2MIN
#define APP_INTERCONNECTION_DISAPPEAR_ADV_IN_MS     5000 // 5sec
#define INTERCONNECTION_DISCOVERABLE_YES            (0)
#define INTERCONNECTION_DISCOVERABLE_NOT            (1)
#else
#define BLE_ADVERTISING_INTERVAL            (320) 
#define BLE_FAST_ADVERTISING_INTERVAL       (48)  
#endif

#define BLE_FASTPAIR_NORMAL_ADVERTISING_INTERVAL        (160) 
#define BLE_FASTPAIR_FAST_ADVERTISING_INTERVAL          (48)  

#define BLE_ADV_MANU_FLAG                   0xFF

#define GVA_AMA_CROSS_ADV_TIME_INTERVAL     (1500)

/**
 * @brief The states of the ble tws.
 *
 */
typedef enum
{
    BLE_STATE_IDLE = 0,
    BLE_STATE_ADVERTISING = 1,
    BLE_STATE_STARTING_ADV = 2,
    BLE_STATE_STOPPING_SCANNING = 3,
    BLE_STATE_SCANNING = 4,
    BLE_STATE_STARTING_SCANNING = 5,
    BLE_STATE_STOPPING_ADV = 6,
    BLE_STATE_CONNECTING = 7,
    BLE_STATE_STARTING_CONNECTING = 8,
    BLE_STATE_STOPPING_CONNECTING = 9,
    BLE_STATE_CONNECTED = 10,
} BLE_STATE_E;

typedef enum
{
    BLE_OP_IDLE = 0,
    BLE_OP_START_ADV,
    BLE_OP_START_SCANNING,
    BLE_OP_START_CONNECTING,
    BLE_OP_STOP_ADV,
    BLE_OP_STOP_SCANNING,
    BLE_OP_STOP_CONNECTING,
} BLE_OP_E;

typedef struct
{
    BLE_STATE_E state;

    // used to delay 5s for starting ble adv if the device is just powered up and in-chargerbox
    uint8_t     advType;
    uint16_t    advIntervalInMs;
    uint16_t    scanWindowInMs;
    uint16_t    scanIntervalInMs;
    uint8_t     scannedTwsCount;
    uint8_t     indexForNextScannedTws;
    uint8_t     bleAddrToConnect[BTIF_BD_ADDR_SIZE];
    uint8_t     ModeToStart;
    uint8_t     advMode;
#ifdef __INTERCONNECTION__
    uint8_t     interconnectionDiscoverable;
#endif
} __attribute__ ((__packed__))BLE_MODE_ENV_T;


void ble_stop_activities(void);
void app_start_ble_connecting(uint8_t* bdAddrToConnect);
void app_start_ble_advertising(uint8_t advType, uint16_t adv_interval_in_ms);
void app_start_ble_scanning(uint16_t scanning_window_in_ms, uint16_t scanning_interval_in_ms);
void app_restart_operation(void);
bool app_ble_is_in_connected_state(void);
void app_start_connectable_ble_adv(uint16_t adv_interval_in_ms);
void app_start_fast_connectable_ble_adv(uint16_t adv_interval_in_ms);
void app_update_ble_adv(uint8_t advType, uint16_t adv_interval_in_ms);

#ifdef __INTERCONNECTION__
void clear_discoverable_adv_timeout_flag(void);
void app_set_interconnection_discoverable(uint8_t discoverable);
uint8_t app_get_interconnection_discoverable(void);
void app_interconnection_start_disappear_adv(uint32_t advInterval, uint32_t advDuration);
void app_interceonnection_start_discoverable_adv(uint32_t advInterval, uint32_t advDuration);
#endif
bool app_ble_is_in_advertising_state(void);

#ifdef __cplusplus
    }
#endif

#endif // #ifndef __APP_BLE_MODE_SWITCH_H__

