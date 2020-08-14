/**
 * BMS GATT Service Handlers
 *
 *   - This file should have no
 *     dependencies on SoC platform
 *   - Currently only dependent on
 *     Trace and Assert mechanism of BES
 *     TODO: remove if file is to be reused
 *
 */
#include "bms_gatt_handler.h"

#include <string.h>
#include "gsound.h"
#include "gsound_target_custom.h"
#include "hal_trace.h"

static BmsState bms_state;

/**
 * Returns reference to CCCD of requested characteristic handle
 * given characteristic handle
 *
 * Note: Handle to either the CCCD or Characteristic Value will work
 */
static uint8_t *BmsGetCccdInternal(uint32_t handle_idx) {
  switch (handle_idx) {
    case BISTO_IDX_BMS_ACTIVE_APP_VAL:
    case BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG:
      return &bms_state.aapp_cccd;
    case BISTO_IDX_BMS_BROADCAST_VAL:
    case BISTO_IDX_BMS_BROADCAST_NTF_CFG:
      return &bms_state.bc_cccd;
    case BISTO_IDX_BMS_MEDIA_CMD_VAL:
    case BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG:
      return &bms_state.mc_cccd;
    case BISTO_IDX_BMS_MEDIA_STATUS_VAL:
    case BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG:
      return &bms_state.ms_cccd;
    default:
      return NULL;
  }
}

/**
 * Checks if handle is tied to BMS server
 */
bool BmsIsHandleValid(uint32_t handle_idx) {
  return (handle_idx>=BISTO_IDX_BMS_START &&
      handle_idx <= BISTO_IDX_BMS_END);
}

/**
 * Returns CCCD of requested characteristic handle
 * given characteristic handle
 *
 * Note: Handle to either the CCCD or Characteristic Value will work
 */
uint8_t BmsGetCccd(uint32_t handle_idx) {
  uint8_t *cccd_ref = BmsGetCccdInternal(handle_idx);
  GSOUND_ASSERT(NULL != cccd_ref);
  return *cccd_ref;
}

/**
 * Sets CCCD of requested characteristic handle
 * given characteristic handle
 *
 * Note: Handle to either the CCCD or Characteristic Value will work
 */
void BmsSetCccd(uint32_t handle_idx, uint8_t cccd) {
  uint8_t *cccd_ref = BmsGetCccdInternal(handle_idx);
  GSOUND_ASSERT(NULL != cccd_ref);
  *cccd_ref = (cccd & (BLE_ATT_CCC_NTF_BIT|BLE_ATT_CCC_IND_BIT));
}


/**
 * Checks if CCCD of specific handle/characteristic is configured
 * to be notifiable i.e. client has indicated they want notification
 * to be sent
 */
bool BmsIsNotifiable(uint32_t handle_idx) {
  return (((BmsGetCccd(handle_idx)) & BLE_ATT_CCC_NTF_BIT) != 0);
}

/**
 * Handles a Bms Read Request
 */
void BmsHandleReadReq(uint32_t handle_idx,
    BmsAttributeData * const att_result) {

  GSOUND_ASSERT((NULL != att_result) && (NULL != att_result->value));
  GSOUND_TRACE("BMS: read req %d", handle_idx);

  switch (handle_idx) {
    /* For reads of Active App, return the stored value */
    case BISTO_IDX_BMS_ACTIVE_APP_VAL:
      memcpy(att_result->value, bms_state.bms_server_aapp,
          sizeof(bms_state.bms_server_aapp));
      att_result->length = sizeof(bms_state.bms_server_aapp);
      att_result->status = BLE_ATT_OK;
      GSOUND_TRACE("BMS: read AAPP %d", att_result->length);
      break;

    /* The Broadcast Command, Media Command, and
     * Media Status characteristics should never be read */
    case BISTO_IDX_BMS_BROADCAST_VAL:
    case BISTO_IDX_BMS_MEDIA_CMD_VAL:
    case BISTO_IDX_BMS_MEDIA_STATUS_VAL:
      GSOUND_TRACE("BMS: %s: read not permitted", __FUNCTION__);
      att_result->length = 0;
      att_result->status = BLE_ATT_READ_NOT_ALLOWED;
      break;

    /* Handle client config (CCCD) reads correctly */
    case BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG:
    case BISTO_IDX_BMS_BROADCAST_NTF_CFG:
    case BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG:
    case BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG:
    {
      att_result->value[0] = BmsGetCccd(handle_idx);;
      att_result->value[1] = 0;
      att_result->status = BLE_ATT_OK;
      att_result->length = 2;  // CCCD reads always 2
      GSOUND_TRACE("BMS: read CCCD %d", att_result->value[0]);
      break;
    }

    /* Everything else is invalid */
    default:
      GSOUND_TRACE("BMS: read invalid");
      att_result->status = BLE_ATT_INVALID_HANDLE;
      att_result->length = 0;
      break;
  }
}

/**
 * Handles the BMS write request.
 *  For CCCD writes, store config locally
 *  For Value writes, store value locally and echo back to client
 *  via ATT notify
 *
 * Return
 *  - true:  if a notification is required - write req was on a Characteristic value
 *           and notification for that characteristic was enabled in CCCD
 *  - false: otherwise
 */
bool BmsHandleWriteRequest(uint32_t handle_idx, uint8_t const * data,
    uint32_t length, uint32_t * const status) {
  uint32_t result = BLE_ATT_OK;
  bool notifiable = false;

  GSOUND_TRACE("BMS: write req %d", handle_idx);
  switch (handle_idx) {

    /* Handle Active App Write Request */
    case BISTO_IDX_BMS_ACTIVE_APP_VAL:
      // Length must be the exact size of Active App
      if (length != BMS_SERVER_AAPP_SIZE) {
        result = BLE_ATT_INVALID_ATT_VAL_EN;
        GSOUND_TRACE("BMS: write AAPP invalid len %d", length);
      } else {
        memcpy(bms_state.bms_server_aapp, data, BMS_SERVER_AAPP_SIZE);
        notifiable = BmsIsNotifiable(handle_idx);
        GSOUND_TRACE("BMS: write AAPP");
      }
      break;

      /* Handle write request for other Characteristics */
    case BISTO_IDX_BMS_BROADCAST_VAL:
    case BISTO_IDX_BMS_MEDIA_CMD_VAL:
    case BISTO_IDX_BMS_MEDIA_STATUS_VAL:
      // Although we don't store anything, client must have sent a non-0 len
      if (length == 0) {
        result = BLE_ATT_INVALID_ATT_VAL_EN;
        GSOUND_TRACE("BMS: write Val invalid len %d", length);
      } else {
        notifiable = BmsIsNotifiable(handle_idx);
      }
      break;

    /* Handle client config (CCCD) write requests */
    case BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG:
    case BISTO_IDX_BMS_BROADCAST_NTF_CFG:
    case BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG:
    case BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG:
      // Store CCCD, unless we have invalid length
      if (length != 2) {
        result = BLE_ATT_INVALID_ATT_VAL_EN;
      } else {
        BmsSetCccd(handle_idx,
            (data[0] & (BLE_ATT_CCC_NTF_BIT|BLE_ATT_CCC_IND_BIT)));
        GSOUND_TRACE("BMS: write CCCD %d",
            (data[0] & (BLE_ATT_CCC_NTF_BIT|BLE_ATT_CCC_IND_BIT)));
      }
      break;

    default:
      GSOUND_TRACE("BMS: write invalid");
      result = BLE_ATT_INVALID_HANDLE;
      break;
  }

  if(status) {
    *status = result;
  }
  return notifiable;
}

