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
#ifndef APP_H_
#define APP_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#ifdef  BLE_APP_PRESENT

#include <stdint.h>          // Standard Integer Definition
#include <co_bt.h>           // Common BT Definitions
#include "arch.h"            // Platform Definitions
#include "gapc.h"            // GAPC Definitions
#include "gapm_task.h"
#if (NVDS_SUPPORT)
#include "nvds.h"
#endif // (NVDS_SUPPORT)

#if defined(CFG_APP_SEC)
#if defined(CFG_SEC_CON)
#define BLE_AUTHENTICATION_LEVEL		GAP_AUTH_REQ_SEC_CON_BOND
#else
#define BLE_AUTHENTICATION_LEVEL		GAP_AUTH_REQ_MITM_BOND
#endif
#else
#define BLE_AUTHENTICATION_LEVEL		GAP_AUTH_REQ_NO_MITM_NO_BOND
#endif

/*
 * DEFINES
 ****************************************************************************************
 */
/// Maximal length of the Device Name value
#define APP_DEVICE_NAME_MAX_LEN      (24)

// Advertising mode
#define APP_FAST_ADV_MODE  (1)
#define APP_SLOW_ADV_MODE  (2)
#define APP_STOP_ADV_MODE  (3)
#define APP_MAX_TX_OCTETS	251
#define APP_MAX_TX_TIME		400

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
typedef enum
{
    BLE_CONN_PARAM_MODE_DEFAULT = 0,
    BLE_CONN_PARAM_MODE_A2DP_ON,
    BLE_CONN_PARAM_MODE_HFP_ON,
    BLE_CONN_PARAM_MODE_OTA,
    BLE_CONN_PARAM_MODE_NUM,
} BLE_CONN_PARAM_MODE_E;

typedef enum
{
    BLE_CONN_PARAM_PRIORITY_NORMAL = 0,
    BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL,
    BLE_CONN_PARAM_PRIORITY_HIGH ,    
} BLE_CONN_PARAM_PRIORITY_E;

typedef struct
{
    uint8_t     ble_conn_param_mode;
    uint8_t     priority;
    uint16_t    conn_param_interval;    // in the unit of 1.25ms
} BLE_CONN_PARAM_CONFIG_T;

#define BLE_CONN_PARAM_SLAVE_LATENCY_CNT        0
#define BLE_CONN_PARAM_SUPERVISE_TIMEOUT_MS     6000

/// Application environment structure
typedef struct
{
    /// Connection handle
    uint16_t conhdl;
    uint8_t isConnected;
    uint8_t isFeatureExchanged;
    /// Bonding status
    uint8_t bonded;
	uint8_t peerAddrType;
	uint8_t isBdAddrResolvingInProgress;
	uint8_t isGotSolvedBdAddr;
	uint8_t bdAddr[BD_ADDR_LEN];
    uint8_t solvedBdAddr[BD_ADDR_LEN];

} APP_BLE_CONN_CONTEXT_T;

/// Application environment structure
struct app_env_tag
{
	uint8_t conn_cnt;
    /// Last initialized profile
    uint8_t next_svc;

    /// Device Name length
    uint8_t dev_name_len;
    /// Device Name
    uint8_t dev_name[APP_DEVICE_NAME_MAX_LEN];

    /// Local device IRK
    uint8_t loc_irk[KEY_LEN];

	APP_BLE_CONN_CONTEXT_T context[BLE_CONNECTION_MAX];
};

// TODO: 
typedef struct
{
   uint8_t role                         :2;
   uint8_t earSide                      :1;
   uint8_t isConnectedWithMobile        :1;
   uint8_t isConnectedWithTWS           :1;
   uint8_t reserved                     :3;
}__attribute__((__packed__)) BLE_ADV_CURRENT_STATE_T;

// max adv data length is 31, and 3 byte is used for adv type flag(0x01)
#define ADV_DATA_MAX_LEN                            (28)

#if defined(__INTERCONNECTION__)

#define CUSTOMIZED_ADV_TYPE                         (0x16)
#define UUID_LEN                                    (2)
#define UUID_VALUE                                  "\xee\xfd"

#define MODEL_ID                                    "\x00\x00\x01"
#define MODEL_ID_LEN                                (3)

// for debug
#define SUB_MODEL_ID                                (0x01)

// data length before extersible data is 15 byte according to spec from huawei
// data structure is defined as CUSTOMIZED_ADV_DATA_T
#define FIXED_CUSTOMIZED_ADV_DATA_LEN               (15)
#define EXTENSIBLE_DATA_MAX_LEN                     (ADV_DATA_MAX_LEN-FIXED_CUSTOMIZED_ADV_DATA_LEN)

// 2 byte to describe paired device
#define PAIRED_DEVICE_ID_LEN                        (2)

// 2 byte to describe dual model device
#define DUAL_MODEL_DEVICE_ID_LEN                    (2)

// max length of per extensible service is 3 byte
#define EXTENSIBLE_SERVICE_MAX_LEN                  (3)

typedef enum
{
    SERVICE_ID_NEARBY                               = 0x01,
    SERVICE_ID_ICONNECT                             = 0x01,
    SERVICE_ID_RSSI_REFERENCE_VALUE                 = 0x02,
    SERVICE_ID_MODEL_ID                             = 0x03,
    SERVICE_ID_SUB_MODEL_ID                         = 0x04,
    SERVICE_ID_PAIRED_DEVICE_ID                     = 0x05,
    SERVICE_ID_CONNECTED_DEVICE_NUM                 = 0x06,
    SERVICE_ID_PAIRED_DEVICE_NUM                    = 0x07,
    SERVICE_ID_MAX_CONNECT_DEVICE_NUM               = 0x08,
    SERVICE_ID_MAX_PAIR_DEVICE_NUM                  = 0x09,
    SERVICE_ID_DUAL_MODE_DEVICE_IDENTIFICATION      = 0x0a,
    SERVICE_ID_TOTAL_BATTERY                        = 0x0b,
    SERVICE_ID_LEFT_EAR_BATTERY                     = 0x0c,
    SERVICE_ID_RIGHT_EAR_BATTERY                    = 0x0d,
    SERVICE_ID_CHARGER_BOX_BATTERY                  = 0x0e,
} NEARBY_SERIVCE_ID_E;

typedef enum
{
    RECONNECTABLE_NOT_DISCOVERABLE_NOT              = 0x00,
    RECONNECTABLE_NOT_DISCOVERABLE_YES              = 0x01,
    RECONNECTABLE_YES_DISCOVERABLE_NOT              = 0x02,
    RECONNECTABLE_YES_DISCOVERABLE_YES              = 0X03,
}ICONNECT_STATUS_E;

typedef struct
{
    uint8_t             advLength;                      // 1 byte
    uint8_t             advType;                        // 1 byte
    uint8_t             uuidValue[UUID_LEN];            // 2 byte
    NEARBY_SERIVCE_ID_E nearbyServiceID;                // 1 byte
    NEARBY_SERIVCE_ID_E iconnectServiceID;              // 1 byte
    ICONNECT_STATUS_E   iconnectStatus;                 // 1 byte
    NEARBY_SERIVCE_ID_E rssiReferenceValueServiceID;    // 1 byte
    int8_t              rssiReferenceValue;             // 1 byte
    NEARBY_SERIVCE_ID_E modelIDServiceID;               // 1 byte
    uint8_t             modelID[MODEL_ID_LEN];          // 3 byte
    NEARBY_SERIVCE_ID_E subModelServiceID;              // 1 byte
    uint8_t             subModelID;                     // 1 byte
    // total length of data above is 15 byte
}__attribute__((__packed__)) CUSTOMIZED_ADV_DATA_HEAD_T;

typedef struct 
{
    uint8_t isCharging : 1;
    uint8_t batteryLevel : 7;
}BATTERY_INFO_T;

#endif // #if defined(__INTERCONNECTION__) 

/*
 * GLOBAL VARIABLE DECLARATION
 ****************************************************************************************
 */

/// Application environment
extern struct app_env_tag app_env;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize the BLE demo application.
 ****************************************************************************************
 */
void appm_init(void);

/**
 ****************************************************************************************
 * @brief Add a required service in the database
 ****************************************************************************************
 */
bool appm_add_svc(void);

/**
 ****************************************************************************************
 * @brief Put the device in general discoverable and connectable mode
 ****************************************************************************************
 */
void appm_start_advertising(uint8_t advType,
	uint32_t advMaxIntervalInMs, uint32_t advMinIntervalInMs);

/**
 ****************************************************************************************
 * @brief Put the device in non discoverable and non connectable mode
 ****************************************************************************************
 */
void appm_stop_advertising(void);

/**
 ****************************************************************************************
 * @brief Send to request to update the connection parameters
 ****************************************************************************************
 */
void appm_update_param(uint8_t conidx, struct gapc_conn_param *conn_param);

/**
 ****************************************************************************************
 * @brief Send a disconnection request
 ****************************************************************************************
 */
void appm_disconnect(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Retrieve device name
 *
 * @param[out] device name
 *
 * @return name length
 ****************************************************************************************
 */
uint8_t appm_get_dev_name(uint8_t* name);

uint8_t appm_is_connected();

/**
 ****************************************************************************************
 * @brief Return if the device is currently bonded
 ****************************************************************************************
 */
bool app_sec_get_bond_status(void);

void app_connected_evt_handler(const uint8_t* pPeerBdAddress);

void app_disconnected_evt_handler(void);

void l2cap_update_param(uint8_t conidx, struct gapc_conn_param *conn_param);

void appm_start_connecting(struct gap_bdaddr* ptBdAddr);

void appm_stop_connecting(void);

void appm_start_scanning(uint16_t intervalInMs, uint16_t windowInMs, uint32_t filtPolicy);

void appm_stop_scanning(void);

void appm_create_advertising(void);

void appm_create_connecting(void);

void app_advertising_stopped(void);

void app_advertising_starting_failed(void);

void app_scanning_stopped(void);

void app_connecting_stopped(void);

void appm_exchange_mtu(uint8_t conidx);

void app_ble_system_ready(void);

void app_adv_reported_scanned(struct gapm_adv_report_ind* ptInd);

void appm_set_private_bd_addr(uint8_t* bdAddr);

void appm_add_dev_into_whitelist(struct gap_bdaddr* ptBdAddr);

void app_scanning_started(void);

void app_advertising_started(void);

void app_connecting_stopped(void);

void app_connecting_started(void);

void appm_update_adv_data(uint8_t* pAdvData, uint32_t dataLength);

bool appm_resolve_random_ble_addr_from_nv(uint8_t conidx, uint8_t* randomAddr);

void appm_resolve_random_ble_addr_with_sepcific_irk(uint8_t conidx, uint8_t* randomAddr, uint8_t* pIrk);

void appm_random_ble_addr_solved(bool isSolvedSuccessfully, uint8_t* irkUsedForSolving);
uint8_t app_ble_connection_count(void);

bool app_is_arrive_at_max_ble_connections(void);

bool app_is_resolving_ble_bd_addr(void);

void app_enter_fastpairing_mode(void);
bool app_is_in_fastparing_mode(void);
void app_set_in_fastpairing_mode_flag(bool isEnabled);

uint16_t appm_get_conhdl_from_conidx(uint8_t conidx);

void appm_check_and_resolve_ble_address(uint8_t conidx);

uint8_t* appm_get_current_ble_addr(void);
void app_trigger_ble_service_discovery(uint8_t conidx, uint16_t shl, uint16_t ehl);

uint8_t* appm_get_local_identity_ble_addr(void);

void app_exchange_remote_feature(uint8_t conidx);

void app_ble_update_conn_param_mode_of_specific_connection(uint8_t conidx, BLE_CONN_PARAM_MODE_E mode, bool isEnable);

void app_ble_reset_conn_param_mode_of_specifc_connection(uint8_t conidx);

void app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_E mode, bool isEnable);

void app_ble_reset_conn_param_mode(uint8_t conidx);

#ifdef __cplusplus
}
#endif

/// @} APP

#endif //(BLE_APP_PRESENT)

#endif // APP_H_
