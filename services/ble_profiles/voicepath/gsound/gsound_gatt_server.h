#ifndef GSOUND_GATT_SERVER_H
#define GSOUND_GATT_SERVER_H

#include "rwip_config.h"

#if (BLE_VOICEPATH)
#include "prf_types.h"
#include "prf.h"
#include "attm.h"
#include "prf_utils.h"

#include "gsound.h"

struct gsound_env_tag {
  // By making this the first struct element, gsound_env can be stored in the
  // 'env' field of prf_task_env.
  prf_env_t prf_env;
  uint8_t conidx;
  uint8_t control_connected[BLE_CONNECTION_MAX];
  uint8_t audio_connected[BLE_CONNECTION_MAX];
  uint16_t start_hdl; // Service Start Handle
  ke_state_t state; // State of different task instances
};

enum gsound_msg_id {
  GSOUND_CHANNEL_CONNECTED = TASK_FIRST_MSG(TASK_ID_VOICEPATH),
  GSOUND_CHANNEL_DISCONNECTED,
  GSOUND_CONTROL_RECEIVED,
  GSOUND_AUDIO_RECEIVED,
  GSOUND_SEND_DATA_REQUEST,
  GSOUND_SEND_CONNECTION_CFM,
  GSOUND_SEND_DISCONNECTION_CFM,
  GSOUND_SEND_COMPLETE_IND,
};

struct gsound_gatt_connection_event_t {
  GSoundChannelType channel;
  uint16_t handle;
  uint8_t status;
};

struct gsound_gatt_rx_data_ind_t {
  uint16_t length;
  uint8_t  data[0];
};

struct gsound_gatt_tx_data_req_t {
  GSoundChannelType channel;
  uint8_t  connectionIndex;
  uint16_t length;
  uint8_t  data[0];
};

struct gsound_gatt_tx_complete_ind_t {
  bool success;
};

const struct prf_task_cbs* gsound_prf_itf_get(void);
void gsound_task_init(struct ke_task_desc *task_desc);
void gsound_send_control_rx_cfm(uint8_t conidx);
void gsound_refresh_ble_database(void);

#endif /* (BLE_VOICEPATH) */

#endif /* GSOUND_GATT_SERVER_H */
