/**
 * Handles interaction between BLE stack
 * and BMS server
 *
 *  - This file is allowed to have dependencies on both
 *    SoC platform + bms_gatt_handler.h
 */
#include "bms_gatt_server.h"
#include "bms_gatt_handler.h"

#if (BLE_VOICEPATH)

#include "ke_mem.h"

#define RD_PERM PERM(RD, ENABLE) | PERM(RP, AUTH)
#define WRITE_REQ_PERM PERM(WRITE_REQ, ENABLE) | PERM(WP, AUTH)
#define NTF_PERM PERM(NTF, ENABLE) | PERM(NP, AUTH)


typedef struct {
  // This field must be first since this struct
  // is used by stack as pointer to prf_env_t
  prf_env_t prf_env;

  uint16_t start_hdl; // Service Start Handle
  ke_state_t state; // State of different task instances
} BmsEnv;

#define BMS_SERVER_SERVICE          {0x3D, 0x21, 0x19, 0x8C, 0x64, 0x8A, 0xD6, 0xB6, 0xBD, 0x42, 0xEF, 0xAA, 0x9C, 0x0D, 0xC1, 0x91}
#define BMS_SERVER_ACTIVE_APP       {0xF1, 0xA1, 0xD1, 0x4D, 0xE6, 0x6B, 0xB4, 0x8F, 0xAA, 0x46, 0x17, 0x45, 0x4E, 0x06, 0xD1, 0x99}
#define BMS_SERVER_MEDIA_COMMAND    {0x85, 0x6E, 0x1F, 0x73, 0xAC, 0x0B, 0x77, 0xB5, 0xB6, 0x44, 0x1A, 0x3F, 0x6C, 0x7F, 0xE8, 0xFB}
#define BMS_SERVER_MEDIA_STATUS     {0x36, 0x11, 0x03, 0x14, 0x16, 0x37, 0x57, 0xB9, 0xD1, 0x4B, 0xF5, 0xBF, 0xC0, 0x91, 0x07, 0x42}
#define BMS_SERVER_BROADCAST        {0x68, 0xB7, 0xAC, 0x66, 0x30, 0x64, 0xE7, 0xA3, 0x87, 0x42, 0xF9, 0x30, 0x46, 0x5A, 0xEF, 0xE4}

#define BISTO_MAX_LEN 244

#define ATT_DECL_PRIMARY_SERVICE_UUID       { 0x00, 0x28 }
#define ATT_DECL_CHARACTERISTIC_UUID        { 0x03, 0x28 }
#define ATT_DESC_CLIENT_CHAR_CFG_UUID       { 0x02, 0x29 }

static const uint8_t bms_uuid_128[ATT_UUID_128_LEN] = BMS_SERVER_SERVICE;

static void bms_task_init(struct ke_task_desc *task_desc);

/***********************
 * BES Characteristic Property Flags
 ************************
 *   PERM_MASK_BROADCAST       /// Broadcast descriptor present
 *   PERM_MASK_RD              /// Read Access Mask
 *   PERM_MASK_WRITE_COMMAND   /// Write Command Enabled attribute Mask
 *   PERM_MASK_WRITE_REQ       /// Write Request Enabled attribute Mask
 *   PERM_MASK_NTF             /// Notification Access Mask
 *   PERM_MASK_IND             /// Indication Access Mask
 *   PERM_MASK_WRITE_SIGNED    /// Write Signed Enabled attribute Mask
 *   PERM_MASK_EXT             /// Extended properties descriptor present
 */


/*******************************************************************************
 *******************************************************************************
 * BMS Service
 *******************************************************************************
 *******************************************************************************/
const struct attm_desc_128 bms_att_db[] = {

    /**************************************************
     *  BMS Service Declaration
     **************************************************/

    [BISTO_IDX_BMS_SVC - BISTO_IDX_BMS_SVC] = {
        ATT_DECL_PRIMARY_SERVICE_UUID,
        RD_PERM,
        0,
        0
    },
    /**************************************************
     * BMS "Active App"
     *    - UUID: 99D1064E-4517-46AA-8FB4-6BE64DD1A1F1
     *    - read/write/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [BISTO_IDX_BMS_ACTIVE_APP_CHAR - BISTO_IDX_BMS_SVC] = {
        ATT_DECL_CHARACTERISTIC_UUID,
        RD_PERM,
        0,
        0
    },

    // Characteristic Value
    [BISTO_IDX_BMS_ACTIVE_APP_VAL - BISTO_IDX_BMS_SVC] = {
        BMS_SERVER_ACTIVE_APP,
        RD_PERM | WRITE_REQ_PERM | NTF_PERM,
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128),
        BISTO_MAX_LEN
    },

    // Client Characteristic Configuration Descriptor
    [BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG - BISTO_IDX_BMS_SVC] = {
        ATT_DESC_CLIENT_CHAR_CFG_UUID,
        RD_PERM | WRITE_REQ_PERM,
        0,
        0
    },

    /**************************************************
     * BMS "Media Command"
     *    - UUID: FBE87F6C-3F1A-44B6-B577-0BAC731F6E85
     *    - write/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [BISTO_IDX_BMS_MEDIA_CMD_CHAR - BISTO_IDX_BMS_SVC] = {
        ATT_DECL_CHARACTERISTIC_UUID,
        RD_PERM,
        0,
        0

    },
    // Characteristic Value
    [BISTO_IDX_BMS_MEDIA_CMD_VAL - BISTO_IDX_BMS_SVC] = {
        BMS_SERVER_MEDIA_COMMAND,
        WRITE_REQ_PERM | NTF_PERM,
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128),
        BISTO_MAX_LEN
    },

    // Client Characteristic Configuration Descriptor
    [BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG - BISTO_IDX_BMS_SVC] = {
        ATT_DESC_CLIENT_CHAR_CFG_UUID,
        RD_PERM | WRITE_REQ_PERM,
        0,
        0
    },

    /**************************************************
     * BMS "Media Status"
     *    - UUID: 420791C0-BFF5-4BD1-B957-371614031136
     *    - write/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [BISTO_IDX_BMS_MEDIA_STATUS_CHAR - BISTO_IDX_BMS_SVC] = {
        ATT_DECL_CHARACTERISTIC_UUID,
        RD_PERM | WRITE_REQ_PERM,
        0,
        0
    },

    // Characteristic Value
    [BISTO_IDX_BMS_MEDIA_STATUS_VAL - BISTO_IDX_BMS_SVC] = {
        BMS_SERVER_MEDIA_STATUS,
        WRITE_REQ_PERM | NTF_PERM,
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128),
        BISTO_MAX_LEN
    },

    // Client Characteristic Configuration Descriptor
    [BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG - BISTO_IDX_BMS_SVC] = {
        ATT_DESC_CLIENT_CHAR_CFG_UUID,
        RD_PERM | WRITE_REQ_PERM,
        0,
        0
    },

    /**************************************************
     * BMS "Broadcast"
     *    - UUID: E4EF5A46-30F9-4287-A3E7-643066ACB768
     *    - write/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [BISTO_IDX_BMS_BROADCAST_CHAR - BISTO_IDX_BMS_SVC] = {
        ATT_DECL_CHARACTERISTIC_UUID,
        RD_PERM,
        0,
        0
    },

    // Characteristic Value
    [BISTO_IDX_BMS_BROADCAST_VAL - BISTO_IDX_BMS_SVC] = {
        BMS_SERVER_BROADCAST,
        WRITE_REQ_PERM | NTF_PERM,
        PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128),
        BISTO_MAX_LEN
    },

    // Client Characteristic Configuration Descriptor
    [BISTO_IDX_BMS_BROADCAST_NTF_CFG - BISTO_IDX_BMS_SVC] = {
        ATT_DESC_CLIENT_CHAR_CFG_UUID,
        RD_PERM | WRITE_REQ_PERM,
        0,
        0
    },
};

/**
 *
 */
static uint8_t bms_init(struct prf_task_env* env, uint16_t* start_hdl,
    uint16_t app_task, uint8_t sec_lvl, void* params) {

  uint8_t svc_perm = (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))
                   | PERM(SVC_MI, DISABLE) | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128);
  uint8_t status;

  TRACE("BMS init: start_handle=%d", *start_hdl);
  // Create BMS service
  status = attm_svc_create_db_128(start_hdl, bms_uuid_128, NULL,
    ARRAY_SIZE(bms_att_db), NULL, env->task, bms_att_db, svc_perm);

  TRACE("BMS: status=%d, start_handle=%d", status, *start_hdl);

  if (status != ATT_ERR_NO_ERROR) {
    return status;
  }

  // Allocate Profile Environment Data
  BmsEnv *bms_env = ke_malloc(sizeof(BmsEnv),
      KE_MEM_ATT_DB);
  ASSERT(bms_env, " %s alloc error", __func__);

  bms_env->prf_env.app_task = app_task |
      (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
  bms_env->prf_env.prf_task = env->task | 
  		(PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
  bms_env->start_hdl = *start_hdl;

  // Initialize environment
  env->env = (prf_env_t *) bms_env;
  env->id = TASK_ID_BMS;
  bms_task_init(&(env->desc));
  ke_state_set(env->task, 0);

  return status;
}

/**
 *
 */
static void bms_destroy(struct prf_task_env* env) {
  struct bms_env_tag *bms_env = (struct bms_env_tag *) env->env;

  TRACE("BMS destroy");
  env->env = NULL;
  ke_free(bms_env);
}

/**
 *
 */
static void bms_create(struct prf_task_env* env, uint8_t conidx) {
  BmsEnv *bms_env = (BmsEnv *)env->env;

  struct prf_svc bms_svc = {bms_env->start_hdl,
      bms_env->start_hdl + ARRAY_SIZE(bms_att_db)};

  TRACE("BMS create: conidx=%d start=%d end=%d", conidx,
      bms_svc.shdl, bms_svc.ehdl);
}

static void bms_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason) {
  /* Nothing to do */
}

const struct prf_task_cbs bms_itf = {
    bms_init,
    bms_destroy,
    bms_create,
    bms_cleanup,
};

/**
 *
 */
const struct prf_task_cbs* bms_prf_itf_get(void) {
  return &bms_itf;
}

static int gattc_write_req_ind_handler(ke_msg_id_t const msgid, struct gattc_write_req_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id);
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid, struct gattc_cmp_evt const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id);
static int gattc_read_req_ind_handler(ke_msg_id_t const msgid, struct gattc_read_req_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id);
static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid, struct gattc_att_info_req_ind *param, ke_task_id_t const dest_id, ke_task_id_t const src_id);


KE_MSG_HANDLER_TAB(bms) {
  {GATTC_WRITE_REQ_IND,    (ke_msg_func_t) gattc_write_req_ind_handler},
  {GATTC_READ_REQ_IND,     (ke_msg_func_t) gattc_read_req_ind_handler},
  {GATTC_ATT_INFO_REQ_IND, (ke_msg_func_t) gattc_att_info_req_ind_handler},
  {GATTC_CMP_EVT,          (ke_msg_func_t) gattc_cmp_evt_handler},
};

static void bms_task_init(struct ke_task_desc *task_desc) {
  BmsEnv *bms_env = (BmsEnv *)prf_env_get(TASK_ID_BMS);
  task_desc->msg_handler_tab = bms_msg_handler_tab;
  task_desc->msg_cnt         = ARRAY_SIZE(bms_msg_handler_tab);
  task_desc->state           = &(bms_env->state);
  task_desc->idx_max         = BLE_CONNECTION_MAX;
}

static int gattc_write_req_ind_handler(ke_msg_id_t const msgid, struct gattc_write_req_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {
  BmsEnv *bms_env = (BmsEnv *)prf_env_get(TASK_ID_BMS);
  struct gattc_write_cfm *cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id,
      dest_id, gattc_write_cfm);
  uint8_t conidx = KE_IDX_GET(src_id);
  int handle_idx = param->handle - bms_env->start_hdl;

  TRACE("BMS: Write, handle_idx=%d", handle_idx);

  if (BmsIsHandleValid(handle_idx)) {
    uint32_t status;
    bool notifiable;
    notifiable = BmsHandleWriteRequest(handle_idx, param->value,
        param->length, &status);
    cfm->status = (uint8_t)status;
    if(status == ATT_ERR_NO_ERROR) {
      if(notifiable) {
        TRACE("BMS: send ntf %d:0x%02X conidx=%d", param->length,
              param->value[0], conidx);
        // Now echo back exactly what we received
        struct gattc_send_evt_cmd *cmd = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx),
            prf_src_task_get(&bms_env->prf_env, conidx),
            gattc_send_evt_cmd,
            param->length);

        cmd->operation = GATTC_NOTIFY;
        cmd->handle    = param->handle;
        cmd->length    = param->length;
        memcpy(cmd->value, param->value, param->length);
        ke_msg_send(cmd);
      }
    }
  } else {
    cfm->status = ATT_ERR_APP_ERROR;
  }

  cfm->handle = param->handle;
  ke_msg_send(cfm);
  return KE_MSG_CONSUMED;
}

static int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
    struct gattc_read_req_ind const *param, ke_task_id_t const dest_id,
    ke_task_id_t const src_id) {

  BmsEnv *bms_env = (BmsEnv *)prf_env_get(TASK_ID_BMS);
  struct gattc_read_cfm *cfm = NULL;
  int handle_idx = param->handle - bms_env->start_hdl;
  TRACE("BMS: Read, handle_idx=%d", handle_idx);

  if(BmsIsHandleValid(handle_idx)) {
    BmsAttributeData att;
    cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id,
        gattc_read_cfm, BMS_SERVER_READ_RESP_MAX);
    att.value = cfm->value;
    BmsHandleReadReq(handle_idx, &att);
    cfm->length = att.length;
    cfm->status = (uint8_t)att.status;
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

static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
    struct gattc_att_info_req_ind *param, ke_task_id_t const dest_id,
    ke_task_id_t const src_id) {

  BmsEnv *bms_env = (BmsEnv *)prf_env_get(TASK_ID_BMS);
  int handle_idx = param->handle - bms_env->start_hdl;
  struct gattc_att_info_cfm * cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id,
      gattc_att_info_cfm);

  TRACE("BMS: Info Req, handle_idx=%d", handle_idx);

  if (BmsIsHandleValid(handle_idx)) {
    // TODO(mosesd): will need to return len and status for each characteristic
  } else {
    cfm->length = 0;
    cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
  }

  cfm->handle = param->handle;
  ke_msg_send(cfm);
  return KE_MSG_CONSUMED;
}

/**
 * Complete event handler
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
    struct gattc_cmp_evt const *param, ke_task_id_t const dest_id,
    ke_task_id_t const src_id) {

  TRACE("BMS: Cmp Evt, operation=%d", param->operation);

  if (param->operation == GATTC_NOTIFY) {
    if (param->status != 0) {
      TRACE("BMS Notification Failed: status=%d", param->status);
    }
    // TODO
  }

  return KE_MSG_CONSUMED;
}

#endif /* (BLE_VOICEPATH) */
