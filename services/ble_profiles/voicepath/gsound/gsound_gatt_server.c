#include "gsound_gatt_server.h"
#include "app.h"
#ifdef IS_MULTI_AI_ENABLED
#include "ai_manager.h"
#endif
#if (BLE_VOICEPATH)

#include "gsound_target_custom.h"
#include "ke_mem.h"

#define RD_PERM PERM(RD, ENABLE) | PERM(RP, AUTH)
#define WRITE_REQ_PERM PERM(WRITE_REQ, ENABLE) | PERM(WP, AUTH)
#define NTF_PERM PERM(NTF, ENABLE) | PERM(NP, AUTH)

/*
 * Bisto App Communications Service:                     fe59bfa8-7fe3-4a05-9d94-99fadc69faff
 * Control Channel TX Characteristic(outgoing protobuf): 69745240-ec29-4899-a2a8-cf78fd214303
 * Control Channel RX Characteristic(incoming protobuf): 104c022e-48d6-4dd2-8737-f8ac5489c5d4
 * Audio Channel TX Characteristic(audio data):          70efdf00-4375-4a9e-912d-63522566d947
 * Audio Channel RX Characteristic(audio sidechannel):   094f376d-9bbd-4690-8c38-3b3ebd9c9ead  Doesn't exist yet.
 */

/*
 * TODO(mfk): All except bisto_audio_rx_char_val_uuid_128_content (Audio Sidechannel) are also on 8670.
 * 8670 also has these two:
 * 0xEEA2E8A0-89F0-4985-A1E2-D91DC4A52632 (Bonded Status - readable unencrypted)
 * 0XA79E2BD1-D6E4-4D1E-8B4F-141D69011CBB (MAC Address Verification - writeable unencrypted)
 *
 * TODO(mfk): investigate what "sidechannel" is actually getting used for here.   Add bonded state and MAC address verification.
 */
#define bisto_service_uuid_128_content             {0xFF, 0xFA, 0x69, 0xDC, 0xFA, 0x99, 0x94, 0x9D, 0x05, 0x4A, 0xE3, 0x7F, 0xA8, 0xBF, 0x59, 0xFE}
#define bisto_control_rx_char_val_uuid_128_content {0xD4, 0xC5, 0x89, 0x54, 0xAC, 0xF8, 0x37, 0x87, 0xD2, 0x4D, 0xD6, 0x48, 0x2E, 0x02, 0x4C, 0x10}
#define bisto_control_tx_char_val_uuid_128_content {0x03, 0x43, 0x21, 0xFD, 0x78, 0xCF, 0xA8, 0xA2, 0x99, 0x48, 0x29, 0xEC, 0x40, 0x52, 0x74, 0x69}
#define bisto_audio_rx_char_val_uuid_128_content   {0xAD, 0x9E, 0x9C, 0xBD, 0x3E, 0x3B, 0x38, 0x8C, 0x90, 0x46, 0xBD, 0x9B, 0x6D, 0x37, 0x4F, 0x09}
#define bisto_audio_tx_char_val_uuid_128_content   {0x47, 0xD9, 0x66, 0x25, 0x52, 0x63, 0x2D, 0x91, 0x9E, 0x4A, 0x75, 0x43, 0x00, 0xDF, 0xEF, 0x70}
#define BISTO_MAX_LEN 244

#define ATT_DECL_PRIMARY_SERVICE_UUID       { 0x00, 0x28 }
#define ATT_DECL_CHARACTERISTIC_UUID        { 0x03, 0x28 }
#define ATT_DESC_CLIENT_CHAR_CFG_UUID       { 0x02, 0x29 }

static const uint8_t BISTO_SERVICE_UUID_128[ATT_UUID_128_LEN] = bisto_service_uuid_128_content;

typedef enum {
    BISTO_IDX_SVC,

    BISTO_IDX_CONTROL_TX_CHAR,
    BISTO_IDX_CONTROL_TX_VAL,
    BISTO_IDX_CONTROL_TX_NTF_CFG,

    BISTO_IDX_CONTROL_RX_CHAR,
    BISTO_IDX_CONTROL_RX_VAL,

    BISTO_IDX_AUDIO_TX_CHAR,
    BISTO_IDX_AUDIO_TX_VAL,
    BISTO_IDX_AUDIO_TX_NTF_CFG,

    BISTO_IDX_AUDIO_RX_CHAR,
    BISTO_IDX_AUDIO_RX_VAL,

    BISTO_IDX_NUM,
} bisto_gatt_handles;

/*
 * TODO(mfk): We should enable security on these characteristics.  Add
 * PERM(NP, AUTH) into these?  Or maybe PERM(NP, SEC_CON)?
 */

/*******************************************************************************
 *******************************************************************************
 * Full Bisto service Database Description
 *  - Used to add attributes into the database
 *
 *
 *******************************************************************************
 *******************************************************************************/

const struct attm_desc_128 gsound_att_db[] = {
    /**************************************************/
    // Service Declaration
    [BISTO_IDX_SVC] = {
        ATT_DECL_PRIMARY_SERVICE_UUID,
        PERM(RD, ENABLE),
        0,
        0
    },

    /**************************************************/
    // Control TX Characteristic Declaration
    [BISTO_IDX_CONTROL_TX_CHAR] = {
        ATT_DECL_CHARACTERISTIC_UUID,    /// 128 bits UUID LSB First
        RD_PERM,                /// Attribute Permissions (@see enum attm_perm_mask) - **Contians Characteristic Property, but ALSO seems to include Attribute Permission if NON-VALUE Declaration
        0,                               /// Attribute Extended Permissions (@see enum attm_value_perm_mask) - seems to be BES custom format, does not match any GATT bit fields
        0                                /// Attribute Max Size (note: for characteristic declaration contains handle offse) (note: for included service, contains target service handl)
    },

    // Control TX Characteristic Value
    [BISTO_IDX_CONTROL_TX_VAL] = {
        bisto_control_tx_char_val_uuid_128_content,           // <-- ATT UUID
        NTF_PERM,                                    // <-- notify access
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), // <-- Trigger Read Indication (0 = Value present in Database, 1 = Value not present in Database) + UUID Lenght
        BISTO_MAX_LEN                                         // <-- Attribute value max size
    },

    // Control TX Characteristic - Client Characteristic Configuration Descriptor
    [BISTO_IDX_CONTROL_TX_NTF_CFG] = {
        ATT_DESC_CLIENT_CHAR_CFG_UUID,
        RD_PERM | WRITE_REQ_PERM,         // <-- Allow read + write request
        10,
        0
    },

    /**************************************************/
    // Control RX Characteristic Declaration
    [BISTO_IDX_CONTROL_RX_CHAR] = {
        ATT_DECL_CHARACTERISTIC_UUID,
        RD_PERM,
        0,
        0
    },

    // Control RX Characteristic Value
    [BISTO_IDX_CONTROL_RX_VAL] = {
        bisto_control_rx_char_val_uuid_128_content,
        WRITE_REQ_PERM,
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128),
        BISTO_MAX_LEN
    },

    /**************************************************/
    // Audio TX Characteristic Declaration
    [BISTO_IDX_AUDIO_TX_CHAR] = {
        ATT_DECL_CHARACTERISTIC_UUID,
        RD_PERM,
        0,
        0
    },
    // Audio TX Characteristic Value
    [BISTO_IDX_AUDIO_TX_VAL] = {
        bisto_audio_tx_char_val_uuid_128_content,
        NTF_PERM,
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128),
        BISTO_MAX_LEN
    },
    // Audio TX Characteristic - Client Characteristic Configuration Descriptor
    [BISTO_IDX_AUDIO_TX_NTF_CFG] = {
        ATT_DESC_CLIENT_CHAR_CFG_UUID,
        RD_PERM | WRITE_REQ_PERM,
        0,
        0
    },

    /**************************************************/
    // Audio RX Characteristic Declaration
    [BISTO_IDX_AUDIO_RX_CHAR] = {
        ATT_DECL_CHARACTERISTIC_UUID,
        RD_PERM,
        0,
        0
    },

    // Audio RX Characteristic Value
    [BISTO_IDX_AUDIO_RX_VAL] = {
        bisto_audio_rx_char_val_uuid_128_content,
        WRITE_REQ_PERM | PERM(WRITE_COMMAND, ENABLE),
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128),
        BISTO_MAX_LEN
    },
};



static uint8_t gsound_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl, void* params) {
  uint8_t svc_perm = (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))
                   | PERM(SVC_MI, DISABLE) | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128);
  uint8_t status;

  GSOUND_TRACE("Gsound gatt init: start_handle=%d", *start_hdl);
  // Create GSound Bisto service
  status = attm_svc_create_db_128(start_hdl, BISTO_SERVICE_UUID_128, NULL,
    ARRAY_SIZE(gsound_att_db), NULL, env->task, gsound_att_db, svc_perm);

  GSOUND_TRACE("%s: status=%d, start_handle=%d", __func__, status, *start_hdl);

  if (status != ATT_ERR_NO_ERROR) {
    return status;
  }

  // Allocate Profile Environment Data
  struct gsound_env_tag *gsound_env = ke_malloc(sizeof(struct gsound_env_tag), KE_MEM_ATT_DB);
  ASSERT(gsound_env, " %s alloc error", __func__);
  gsound_env->prf_env.app_task = app_task | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
  gsound_env->prf_env.prf_task = env->task | 
      (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
  gsound_env->start_hdl = *start_hdl;

  // Initialize environment
  env->env = (prf_env_t *) gsound_env;
  env->id = TASK_ID_VOICEPATH;
  gsound_task_init(&(env->desc));
  ke_state_set(env->task, 0);

  return status;
}

static void gsound_destroy(struct prf_task_env* env) {
  struct gsound_env_tag *gsound_env = (struct gsound_env_tag *) env->env;
  env->env = NULL;
  ke_free(gsound_env);
}

static void gsound_create(struct prf_task_env* env, uint8_t conidx) {
  struct gsound_env_tag *gsound_env = (struct gsound_env_tag *) env->env;

  struct prf_svc gsound_svc = {
      gsound_env->start_hdl,
      gsound_env->start_hdl + ARRAY_SIZE(gsound_att_db)
  };

  GSOUND_TRACE("Gsound gatt create: conidx=%d start=%d - end=%d", conidx, gsound_svc.shdl, gsound_svc.ehdl);
}

static void gsound_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason) {
  /* Nothing to do */
  struct gsound_env_tag *gsound_env = (struct gsound_env_tag *) env->env;
  gsound_env->audio_connected[conidx] = 0;
  gsound_env->control_connected[conidx] = 0;
}

const struct prf_task_cbs gsound_itf = {
    (prf_init_fnct) gsound_init,
    gsound_destroy,
    gsound_create,
    gsound_cleanup,
};

const struct prf_task_cbs* voicepath_prf_itf_get(void) {
  return &gsound_itf;
}

static int gattc_write_req_ind_handler(ke_msg_id_t const msgid, struct gattc_write_req_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id);
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid, struct gattc_cmp_evt const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id);
static int gattc_read_req_ind_handler(ke_msg_id_t const msgid, struct gattc_read_req_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id);
static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid, struct gattc_att_info_req_ind *param, ke_task_id_t const dest_id, ke_task_id_t const src_id);
static int gsound_send_data_request_handler(ke_msg_id_t const msgid, struct gsound_gatt_tx_data_req_t *param, ke_task_id_t const dest_id, ke_task_id_t const src_id);
static int gsound_send_connection_cfm_handler(ke_msg_id_t const msgid,
                                              struct gsound_gatt_connection_event_t *param,
                                              ke_task_id_t const dest_id,
                                              ke_task_id_t const src_id);
static int gsound_send_disconnection_cfm_handler(ke_msg_id_t const msgid,
                                            struct gsound_gatt_connection_event_t *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id);

KE_MSG_HANDLER_TAB(gsound) {
  {GATTC_WRITE_REQ_IND, (ke_msg_func_t) gattc_write_req_ind_handler},
  {GATTC_READ_REQ_IND, (ke_msg_func_t) gattc_read_req_ind_handler},
  {GATTC_ATT_INFO_REQ_IND, (ke_msg_func_t) gattc_att_info_req_ind_handler},
  {GSOUND_SEND_DATA_REQUEST, (ke_msg_func_t) gsound_send_data_request_handler},
  {GSOUND_SEND_CONNECTION_CFM, (ke_msg_func_t) gsound_send_connection_cfm_handler},
  {GSOUND_SEND_DISCONNECTION_CFM, (ke_msg_func_t) gsound_send_disconnection_cfm_handler},
  {GATTC_CMP_EVT, (ke_msg_func_t) gattc_cmp_evt_handler},
};

void gsound_task_init(struct ke_task_desc *task_desc) {
  struct gsound_env_tag *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
  task_desc->msg_handler_tab = gsound_msg_handler_tab;
  task_desc->msg_cnt         = ARRAY_SIZE(gsound_msg_handler_tab);
  task_desc->state           = &(gsound_env->state);
  task_desc->idx_max         = BLE_CONNECTION_MAX;
}

static void gsound_update_connection(uint8_t conidx, struct gsound_env_tag *gsound_env,
                                     GSoundChannelType channel,
                                     bool is_connected) {
  if (channel == GSOUND_CHANNEL_CONTROL) {
    gsound_env->control_connected[conidx] = is_connected;
  }
  if (channel == GSOUND_CHANNEL_AUDIO) {
    gsound_env->audio_connected[conidx] = is_connected;
  }
}

static int gattc_write_req_ind_handler(ke_msg_id_t const msgid, struct gattc_write_req_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  struct gsound_env_tag *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
  uint8_t status;
  uint8_t conidx = KE_IDX_GET(src_id);
  int handle_idx = param->handle - gsound_env->start_hdl;
  GSOUND_TRACE("GSound GATT Write: handle_idx=%d conidx %d", handle_idx, conidx);

  if ((handle_idx == BISTO_IDX_CONTROL_TX_NTF_CFG) || (handle_idx == BISTO_IDX_AUDIO_TX_NTF_CFG)) {
    status = ATT_ERR_APP_ERROR;
    if (param->length == 2) {
      uint16_t cccd_val = (((uint16_t) param->value[1]) << 8) | ((uint16_t) param->value[0]);
      bool is_connected = ((cccd_val & PRF_CLI_START_NTF) != 0);
      GSoundChannelType channel = (handle_idx == BISTO_IDX_CONTROL_TX_NTF_CFG) ? GSOUND_CHANNEL_CONTROL : GSOUND_CHANNEL_AUDIO;
      bool was_connected = (channel == GSOUND_CHANNEL_CONTROL) ? gsound_env->control_connected[conidx] : gsound_env->audio_connected[conidx];
      if (is_connected != was_connected) {
        ke_msg_id_t msg_id = is_connected ? GSOUND_CHANNEL_CONNECTED : GSOUND_CHANNEL_DISCONNECTED;
        struct gsound_gatt_connection_event_t *ind = KE_MSG_ALLOC(msg_id,
          prf_dst_task_get(&gsound_env->prf_env, conidx),
          prf_src_task_get(&gsound_env->prf_env, conidx),
          gsound_gatt_connection_event_t);
        ind->channel = channel;
        ind->handle = param->handle;
        ke_msg_send(ind);

        gsound_env->conidx = conidx;
        if(is_connected) {
          // Transition from disconnected->connected, return here without
          // sending write cfm to remote client. Instead we need to process the
          // request and make sure it is allowed.
          GSOUND_TRACE("connected, cfm later");
          return KE_MSG_CONSUMED;
        } else {
#if 0
          GSOUND_TRACE("disconnected, cfm now");
          // If disconnecting, update and send response in this context
          gsound_update_connection(conidx, gsound_env, channel, is_connected);
#else
          // keep same flow with connection procedure, in disconnect handler to upgrade spec table.
          GSOUND_TRACE("disconnected, cfm later")
          return KE_MSG_CONSUMED;
#endif
        }
      }
      // If no change or disconnecting, just response OK to remote client
      // and move on
      status = GAP_ERR_NO_ERROR;
    }
  } else if ((handle_idx == BISTO_IDX_CONTROL_RX_VAL) || (handle_idx == BISTO_IDX_AUDIO_RX_VAL)) {
    ke_msg_id_t msg_id = (handle_idx == BISTO_IDX_CONTROL_RX_VAL) ? GSOUND_CONTROL_RECEIVED : GSOUND_AUDIO_RECEIVED;
    struct gsound_gatt_rx_data_ind_t *ind = KE_MSG_ALLOC_DYN(msg_id,
      prf_dst_task_get(&gsound_env->prf_env, conidx),
      prf_src_task_get(&gsound_env->prf_env, conidx),
      gsound_gatt_rx_data_ind_t,
      param->length);
    ind->length = param->length;
    memcpy(ind->data, param->value, param->length);
    ke_msg_send(ind);
    TRACE("gbuf + 0x%x", ind);
    status = GAP_ERR_NO_ERROR;

    if (GSOUND_CONTROL_RECEIVED == msg_id)
    {    // pending for the rx data handling is completed
        return KE_MSG_CONSUMED;
    }
  } else {
    status = ATT_ERR_APP_ERROR;
  }

  // Allocate message only if we are going to send it.
  struct gattc_write_cfm *cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
  cfm->status = status;
  cfm->handle = param->handle;
  ke_msg_send(cfm);
  return KE_MSG_CONSUMED;
}

static int gattc_read_req_ind_handler(ke_msg_id_t const msgid, struct gattc_read_req_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  struct gsound_env_tag *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
  struct gattc_read_cfm *cfm = NULL;
  uint8_t conidx = KE_IDX_GET(src_id);
  int handle_idx = param->handle - gsound_env->start_hdl;
  GSOUND_TRACE("GSound GATT Read: handle_idx=%d", handle_idx);

  if (handle_idx == BISTO_IDX_CONTROL_TX_NTF_CFG) {
    cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, 2);
    cfm->length = 2;
    cfm->value[0] = gsound_env->control_connected[conidx] ? 1 : 0;
    cfm->value[1] = 0;
    cfm->status = GAP_ERR_NO_ERROR;
  } else if (handle_idx == BISTO_IDX_AUDIO_TX_NTF_CFG) {
    cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, 2);
    cfm->length = 2;
    cfm->value[0] = gsound_env->audio_connected[conidx] ? 1 : 0;
    cfm->value[1] = 0;
    cfm->status = GAP_ERR_NO_ERROR;
  } else {
    cfm = KE_MSG_ALLOC(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm);
    cfm->length = 0;
    cfm->status = ATT_ERR_REQUEST_NOT_SUPPORTED;
  }

  cfm->handle = param->handle;
  ke_msg_send(cfm);
  ke_state_set(dest_id, 0);
  return KE_MSG_CONSUMED;
}

static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid, struct gattc_att_info_req_ind *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  struct gsound_env_tag *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
  struct gattc_att_info_cfm * cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
  cfm->handle = param->handle;

  if ((param->handle == (gsound_env->start_hdl + BISTO_IDX_CONTROL_TX_NTF_CFG)) ||
      (param->handle == (gsound_env->start_hdl + BISTO_IDX_AUDIO_TX_NTF_CFG))) {
    // CCC attribute length = 2
    cfm->length = 2;
    cfm->status = GAP_ERR_NO_ERROR;
  } else if ((param->handle == (gsound_env->start_hdl + BISTO_IDX_CONTROL_RX_VAL)) ||
             (param->handle == (gsound_env->start_hdl + BISTO_IDX_AUDIO_RX_VAL))) {
    cfm->length = 0;
    cfm->status = GAP_ERR_NO_ERROR;
  } else {
    cfm->length = 0;
    cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
  }

  ke_msg_send(cfm);
  return KE_MSG_CONSUMED;
}

static int gsound_send_connection_cfm_handler(ke_msg_id_t const msgid,
                                              struct gsound_gatt_connection_event_t *param,
                                              ke_task_id_t const dest_id,
                                              ke_task_id_t const src_id) {
  bool is_connected = (param->status == GAP_ERR_NO_ERROR);
  GSOUND_TRACE("GSound GATT Connection CFM: channel=%s",
      param->channel == GSOUND_CHANNEL_CONTROL ?
          "CONTROL" : "AUDIO");


  struct gsound_env_tag *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
  uint8_t conidx = KE_IDX_GET(dest_id);

  if (param->status == GAP_ERR_NO_ERROR) {
    gsound_update_connection(conidx, gsound_env, param->channel, is_connected);
    GSOUND_TRACE("ble cfm handler ok, update state %d %d msg:0x%02X",
        gsound_env->audio_connected[conidx], gsound_env->control_connected[conidx], param->status);
  } else {
    GSOUND_TRACE("ble cfm handler error, %d %d msg:0x%02X",
        gsound_env->audio_connected[conidx], gsound_env->control_connected[conidx], param->status);
  }

  struct gattc_write_cfm *cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM,
      KE_BUILD_ID(TASK_GATTC, conidx),
      prf_src_task_get(&gsound_env->prf_env, conidx),
      gattc_write_cfm);
  cfm->handle = param->handle;
  cfm->status = param->status;
  ke_msg_send(cfm);
  return KE_MSG_CONSUMED;
}

static int gsound_send_disconnection_cfm_handler(ke_msg_id_t const msgid,
                                          struct gsound_gatt_connection_event_t *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id) {
  bool is_connected = false;
  GSOUND_TRACE("GSound GATT Disconnection CFM: channel=%s",
      param->channel == GSOUND_CHANNEL_CONTROL ?
          "CONTROL" : "AUDIO");


  struct gsound_env_tag *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
  uint8_t conidx = KE_IDX_GET(dest_id);

  if (param->status == GAP_ERR_NO_ERROR) {
    gsound_update_connection(conidx, gsound_env, param->channel, is_connected);
    GSOUND_TRACE("ble cfm handler ok, update state %d %d msg:0x%02X",
        gsound_env->audio_connected[conidx], gsound_env->control_connected[conidx], param->status);
  } else {
    GSOUND_TRACE("ble cfm handler error, %d %d msg:0x%02X",
        gsound_env->audio_connected[conidx], gsound_env->control_connected[conidx], param->status);
  }

  struct gattc_write_cfm *cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM,
      KE_BUILD_ID(TASK_GATTC, conidx),
      prf_src_task_get(&gsound_env->prf_env, conidx),
      gattc_write_cfm);
  cfm->handle = param->handle;
  cfm->status = param->status;
  ke_msg_send(cfm);
  return KE_MSG_CONSUMED;
}

static int gsound_send_data_request_handler(ke_msg_id_t const msgid, struct gsound_gatt_tx_data_req_t *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  GSOUND_TRACE("GSound GATT Notification: channel=%s, length=%d", param->channel == GSOUND_CHANNEL_CONTROL ? "CONTROL" : "AUDIO", param->length);
  struct gsound_env_tag *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
  uint16_t handle_idx = (param->channel == GSOUND_CHANNEL_CONTROL) ? BISTO_IDX_CONTROL_TX_VAL : BISTO_IDX_AUDIO_TX_VAL;
  uint8_t conidx = KE_IDX_GET(dest_id);

  struct gattc_send_evt_cmd *cmd = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
          KE_BUILD_ID(TASK_GATTC, conidx),
          prf_src_task_get(&gsound_env->prf_env, conidx),
          gattc_send_evt_cmd,
          param->length);

  cmd->operation = GATTC_NOTIFY;
  cmd->handle    = gsound_env->start_hdl + handle_idx;
  cmd->length    = param->length;
  memcpy(cmd->value, param->data, param->length);
  ke_msg_send(cmd);

  return KE_MSG_CONSUMED;
}

/**
 * Complete event handler
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid, struct gattc_cmp_evt const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  struct gsound_env_tag *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
  uint8_t conidx = KE_IDX_GET(dest_id);

  if (param->operation == GATTC_NOTIFY) {
    if (param->status != 0) {
      GSOUND_TRACE("GSound GATT Notification Failed: status=%d", param->status);
    }
    struct gsound_gatt_tx_complete_ind_t *ind = KE_MSG_ALLOC(GSOUND_SEND_COMPLETE_IND,
      prf_dst_task_get(&gsound_env->prf_env, conidx),
      prf_src_task_get(&gsound_env->prf_env, conidx),
      gsound_gatt_tx_complete_ind_t);
    ind->success = (param->status == GAP_ERR_NO_ERROR);
    ke_msg_send(ind);
  }

  return KE_MSG_CONSUMED;
}

void gsound_send_control_rx_cfm(uint8_t conidx)
{
    struct gsound_env_tag *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
    if (gsound_env->control_connected[conidx])
    {
      struct gattc_write_cfm *cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM,
      KE_BUILD_ID(TASK_GATTC, conidx),
      prf_src_task_get(&gsound_env->prf_env, conidx),
          gattc_write_cfm);
      cfm->handle = gsound_env->start_hdl + BISTO_IDX_CONTROL_RX_VAL;
      cfm->status = ATT_ERR_NO_ERROR;
      ke_msg_send(cfm);
    }
}

void gsound_refresh_ble_database(void)
{
    struct gsound_env_tag *gsound_env = PRF_ENV_GET(VOICEPATH, gsound);
    app_trigger_ble_service_discovery(gsound_env->conidx, 
        gsound_env->start_hdl,
        gsound_env->start_hdl + BISTO_IDX_NUM);
}

#endif /* (BLE_VOICEPATH) */
