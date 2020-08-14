#ifndef BMS_GATT_HANDLER_H
#define BMS_GATT_HANDLER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*************************************
 * Platform independent BLE specified
 * definitions
 *************************************/
#define BLE_ATT_CCC_NTF_BIT 1
#define BLE_ATT_CCC_IND_BIT 2

#define BLE_ATT_OK                  0x00
#define BLE_ATT_INVALID_HANDLE      0x01
#define BLE_ATT_READ_NOT_ALLOWED    0x02
#define BLE_ATT_INVALID_ATT_VAL_EN  0x0D

/*************************************
 * BMS Specific
 *************************************/
#define BMS_SERVER_AAPP_SIZE          20
#define BMS_SERVER_READ_RESP_MAX      BMS_SERVER_AAPP_SIZE

/*************************************
 *
 *************************************/
typedef enum {
    BISTO_IDX_BMS_START,                // 0
    BISTO_IDX_BMS_SVC = BISTO_IDX_BMS_START,

    BISTO_IDX_BMS_ACTIVE_APP_CHAR,      // 1
    BISTO_IDX_BMS_ACTIVE_APP_VAL,       // 2
    BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG,   // 3

    BISTO_IDX_BMS_MEDIA_CMD_CHAR,       // 4
    BISTO_IDX_BMS_MEDIA_CMD_VAL,        // 5
    BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG,    // 6

    BISTO_IDX_BMS_MEDIA_STATUS_CHAR,    // 7
    BISTO_IDX_BMS_MEDIA_STATUS_VAL,     // 8
    BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG, // 9

    BISTO_IDX_BMS_BROADCAST_CHAR,       // 10
    BISTO_IDX_BMS_BROADCAST_VAL,        // 11
    BISTO_IDX_BMS_BROADCAST_NTF_CFG,    // 12

    BISTO_IDX_BMS_END = BISTO_IDX_BMS_BROADCAST_NTF_CFG,

    BISTO_IDX_BMS_NUM,
} BmsGattHandlers;

typedef struct {
  uint32_t status;
  uint32_t length;
  uint8_t  *value;
} BmsAttributeData;

/*
 * BMS State
 *
 * Contains:
 *    - CCCD storage space for each of the
 *      four BMS characteristics
 *
 *    - Active App value storage
 */
typedef struct {
  uint8_t aapp_cccd;
  uint8_t bc_cccd;
  uint8_t mc_cccd;
  uint8_t ms_cccd;

  uint8_t bms_server_aapp[BMS_SERVER_AAPP_SIZE];
} BmsState;

bool BmsIsHandleValid(uint32_t handle_idx);

/**
 * Returns CCCD of requested characteristic handle
 * given characteristic handle
 *
 * Note: Handle to either the CCCD or Characteristic Value will work
 */
uint8_t BmsGetCccd(uint32_t handle_idx);

/**
 * Sets CCCD of requested characteristic handle
 * given characteristic handle
 *
 * Note: Handle to either the CCCD or Characteristic Value will work
 */
void BmsSetCccd(uint32_t handle_idx, uint8_t cccd);

/**
 * Checks if CCCD of specific handle/characteristic is configured
 * to be notifiable i.e. client has indicated they want notification
 * to be sent
 */
bool BmsIsNotifiable(uint32_t handle_idx);

/**
 * Handles a Bms Read Request
 */
void BmsHandleReadReq(uint32_t handle_idx, BmsAttributeData * const att_result);

/**
 * Handles the BMS write request.
 *
 * Return
 *  - true:  if a notification is required - write req was on a Characteristic value
 *           and notification for that characterisit cas enabled in CCCD
 *  - false: otherwise
 */
bool BmsHandleWriteRequest(uint32_t handle_idx, uint8_t const * data,
    uint32_t length, uint32_t * const status);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BMS_GATT_HANDLER_H */
