/**
 * Handles interaction between BLE stack
 * and AMS server
 *
 *  - This file is allowed to have dependencies on SoC platform.
 */
#include "ams_gatt_server.h"

#include "rwapp_config.h"

#if (ANCS_PROXY_ENABLE)

#include "amsc.h"
#include "amsc_task.h"
#include "ke_mem.h"
#include "gsound.h"
#include "gsound_target_custom.h"

#define UUID_ARRAY(uuid) \
  { (uuid & 0xff), ((uuid >> 8) & 0xff) }

#define PROXY_TASK_ID TASK_ID_AMSP
#define PROXY_ENV_GET() ((ProxyEnv *)prf_env_get(PROXY_TASK_ID))
#define CLIENT_TASK_ID (TASK_ID_AMSC)
#define DBG_NAME "AMSP "

// TODO(mfk): ready_ntf_enable must be non-volatile.
typedef struct {
  // By making this the first struct element, the proxy env can be stored in the
  // 'env' field of prf_task_env.
  prf_env_t prf_env;

  uint8_t conidx;
  bool ready;
  bool ready_ntf_enable;
  uint16_t start_hdl;  // Service Start Handle
  ke_state_t state;    // State of different task instances
  int client_handles[AMS_PROXY_NUM];
} ProxyEnv;

/*
 * 55F80AEF-D89F-41A4-9E36-0FFC88DC81CE
 Remote Command (UUID: 9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2) is a direct proxy
 of Apple's AMS Remote Command characteristic and matches its UUID and
 properties. AMS Ready (UUID: 3ADF41AF-F7A1-4E16-863E-53A188D5BF8D), read/notify
 which provides the current state of AMS (0 - not ready, 1 - ready) Entity
 Update (UUID: 2F7CABCE-808D-411F-9A0C-BB92BA96C102) is a direct proxy of
 Apple's AMS Entity Update characteristic and matches its UUID and properties.
 Entity Attribute (UUID: C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7) is a direct proxy
 of Apple's AMS Entity Attribute characteristic and matches its UUID and
 properties.
 */

#define proxy_service_uuid_128_content                                      \
  {                                                                         \
    0xCE, 0x81, 0xDC, 0x88, 0xFC, 0x0F, 0x36, 0x9e, 0xa4, 0x41, 0x9f, 0xd8, \
        0xef, 0x0a, 0xf8, 0x55                                              \
  }
#define PROXY_CHAR_READY                                                    \
  {                                                                         \
    0x8D, 0xBF, 0xD5, 0x88, 0xA1, 0x53, 0x3E, 0x86, 0x16, 0x4E, 0xA1, 0xF7, \
        0xAF, 0x41, 0xDF, 0x3A                                              \
  }
#define PROXY_CHAR_REMOTE_CMD                                               \
  {                                                                         \
    0xC2, 0x51, 0xCA, 0xF7, 0x56, 0x0E, 0xDF, 0xB8, 0x8A, 0x4A, 0xB1, 0x57, \
        0xD8, 0x81, 0x3C, 0x9B                                              \
  }
#define PROXY_CHAR_ENT_UPDATE                                               \
  {                                                                         \
    0x02, 0xC1, 0x96, 0xBA, 0x92, 0xBB, 0x0C, 0x9A, 0x1F, 0x41, 0x8D, 0x80, \
        0xCE, 0xAB, 0x7C, 0x2F                                              \
  }
#define PROXY_CHAR_ENT_ATTR                                                 \
  {                                                                         \
    0xD7, 0xD5, 0xBB, 0x70, 0xA8, 0xA3, 0xAB, 0xA6, 0xD8, 0x46, 0xAB, 0x23, \
        0x8C, 0xF3, 0xB2, 0xC6                                              \
  }

// TODO(mfk): what is the right number?  Actually, this is likely irrelevant
// since I think it's a protobuf max length.
#define BISTO_MAX_LEN 244

/*
 * TODO(mfk):  Per p63 of bluetooth book:
 *     - value of config descriptors are unique per connection.  So, if we allow
 *       more than one phone to bond with AMS proxy, this must be done.
 *     - value of config descriptors are preserved across connections with
 *       bonded devices.  Should use nvram to hold config values (notifications/
 *       indications on/off).
 */

static const uint8_t PROXY_SERVICE_UUID_128[ATT_UUID_128_LEN] =
    proxy_service_uuid_128_content;

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

#define RD_PERM PERM(RD, ENABLE) | PERM(RP, AUTH)
#define WRITE_REQ_PERM PERM(WRITE_REQ, ENABLE) | PERM(WP, AUTH)
#define WRITE_COMMAND_PERM PERM(WRITE_COMMAND, ENABLE) | PERM(WP, AUTH)
#define NTF_PERM PERM(NTF, ENABLE) | PERM(NP, AUTH)

/*
 * TODO(mfk): According to the bluetooth book, the "value" field in the first
 *            row of the table below should be the UUID of the service.  Maybe
 *            BES implementation of attm_svc_create_db_128 does this for us?
 */

/*******************************************************************************
 *******************************************************************************
 * AMS Service - 55F80AEF-D89F-41A4-9E36-0FFC88DC81CE
 *******************************************************************************
 *******************************************************************************/
static const struct attm_desc_128 proxy_att_db[] = {

    /**************************************************
     *  AMS Service Declaration
     **************************************************/

    [AMS_PROXY_SVC] = {UUID_ARRAY(ATT_DECL_PRIMARY_SERVICE), PERM(RD, ENABLE),
                       0, 0},

    /**************************************************
     * AMS Remote Command
     *    - UUID: 9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2
     *    - Writeable, notifiable (Direct proxy of Apple's AMS characteristic
     *and matches its UUID and properties)
     **************************************************/
    // Characteristic Declaration
    [AMS_PROXY_REMOTE_CMD_CHAR] = {UUID_ARRAY(ATT_DECL_CHARACTERISTIC),
                                   PERM(RD, ENABLE), 0, 0},
    // Characteristic Value
    [AMS_PROXY_REMOTE_CMD_VAL] =
        {PROXY_CHAR_REMOTE_CMD, NTF_PERM | WRITE_REQ_PERM | WRITE_COMMAND_PERM,
         PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), BISTO_MAX_LEN},
    // Client Characteristic Configuration Descriptor
    [AMS_PROXY_REMOTE_CMD_CFG] = {UUID_ARRAY(ATT_DESC_CLIENT_CHAR_CFG),
                                  RD_PERM | WRITE_REQ_PERM, 10, 0},

    /**************************************************
     * AMS Ready (Google specific)
     *    - UUID: 3ADF41AF-F7A1-4E16-863E-53A188D5BF8D
     *    - provides the current state of AMS (0 - not ready, 1 - ready)
     *    - read/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [AMS_PROXY_READY_CHAR] = {UUID_ARRAY(ATT_DECL_CHARACTERISTIC),
                              PERM(RD, ENABLE), 0, 0},
    // Characteristic Value
    [AMS_PROXY_READY_VAL] = {PROXY_CHAR_READY, NTF_PERM | RD_PERM,
                             PERM(RI, ENABLE) |
                                 PERM_VAL(UUID_LEN, PERM_UUID_128),
                             BISTO_MAX_LEN},
    // Client Characteristic Configuration Descriptor
    [AMS_PROXY_READY_CFG] = {UUID_ARRAY(ATT_DESC_CLIENT_CHAR_CFG),
                             RD_PERM | WRITE_REQ_PERM, 10, 0},

    /**************************************************
     * AMS Entity Update
     *    - UUID: 2F7CABCE-808D-411F-9A0C-BB92BA96C102
     *    - Writeable with response, notifiable (Direct proxy of Apple's AMS
     *characteristic and matches its UUID and properties)
     **************************************************/
    // Characteristic Declaration
    [AMS_PROXY_ENT_UPD_CHAR] = {UUID_ARRAY(ATT_DECL_CHARACTERISTIC),
                                PERM(RD, ENABLE), 0, 0},
    // Characteristic Value
    [AMS_PROXY_ENT_UPD_VAL] = {PROXY_CHAR_ENT_UPDATE, NTF_PERM | WRITE_REQ_PERM,
                               PERM(RI, ENABLE) |
                                   PERM_VAL(UUID_LEN, PERM_UUID_128),
                               BISTO_MAX_LEN},
    // Client Characteristic Configuration Descriptor
    [AMS_PROXY_ENT_UPD_CFG] = {UUID_ARRAY(ATT_DESC_CLIENT_CHAR_CFG),
                               RD_PERM | WRITE_REQ_PERM, 10, 0},

    /**************************************************
     * AMS Entity Attribute
     *    - UUID: C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7
     *    - readable, writeable (Direct proxy of Apple's AMS characteristic and
     *matches its UUID and properties)
     **************************************************/
    // Characteristic Declaration
    [AMS_PROXY_ENT_ATTR_CHAR] = {UUID_ARRAY(ATT_DECL_CHARACTERISTIC),
                                 PERM(RD, ENABLE), 0, 0},
    // Characteristic Value
    [AMS_PROXY_ENT_ATTR_VAL] = {PROXY_CHAR_ENT_ATTR,
                                RD_PERM | WRITE_REQ_PERM | WRITE_COMMAND_PERM,
                                PERM(RI, ENABLE) |
                                    PERM_VAL(UUID_LEN, PERM_UUID_128),
                                BISTO_MAX_LEN},
};

static int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_write_req_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id);
static int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id);
static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
                                          struct gattc_att_info_req_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id);
static int proxy_ind_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_send_evt_cmd *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id);
static int gattc_write_cfm_handler(ke_msg_id_t const msgid,
                                   struct gattc_write_cfm const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id);
static int gattc_read_cfm_handler(ke_msg_id_t const msgid,
                                  struct gattc_read_cfm const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id);

static void proxy_task_init(struct ke_task_desc *task_desc);

static void ams_proxy_notify_ready_flag() {
  ProxyEnv *proxy_env = PROXY_ENV_GET();
  GSOUND_TRACE(DBG_NAME "%s entry. pvReady=%d ntf_enabled=%d", __func__,
        proxy_env->ready, proxy_env->ready_ntf_enable);
  if (proxy_env->ready_ntf_enable) {
    BLE_APP_DBG(DBG_NAME "Notify Ready=%d", proxy_env->ready);
    struct gattc_send_evt_cmd *cmd = KE_MSG_ALLOC_DYN(
        GATTC_SEND_EVT_CMD, KE_BUILD_ID(TASK_GATTC, proxy_env->conidx),
        prf_src_task_get(&proxy_env->prf_env, proxy_env->conidx),
        gattc_send_evt_cmd, 2);
    cmd->operation = GATTC_NOTIFY;
    cmd->handle = proxy_env->start_hdl + AMS_PROXY_READY_VAL;
    cmd->length = 2;
    cmd->value[0] = proxy_env->ready ? PRF_CLI_START_NTF : 0;
    cmd->value[1] = 0;
    ke_msg_send(cmd);
  }
}

void ams_proxy_set_ready_flag(int handle_rem_cmd_char, int handle_rem_cmd_val,
                              int handle_rem_cmd_cfg, int handle_ent_upd_char,
                              int handle_ent_upd_val, int handle_ent_upd_cfg,
                              int handle_ent_attr_char,
                              int handle_ent_attr_val) {
  // TODO(jkessinger): This is called with zeros if the service does not exist.
  // Maybe set ready = false.
  ProxyEnv *proxy_env = PROXY_ENV_GET();
  GSOUND_TRACE(DBG_NAME "%s entry. pvReady=%d enable=%d", __func__, proxy_env->ready,
        proxy_env->ready_ntf_enable);
  proxy_env->client_handles[AMS_PROXY_REMOTE_CMD_CHAR] = handle_rem_cmd_char;
  proxy_env->client_handles[AMS_PROXY_REMOTE_CMD_VAL] = handle_rem_cmd_val;
  proxy_env->client_handles[AMS_PROXY_REMOTE_CMD_CFG] = handle_rem_cmd_cfg;
  proxy_env->client_handles[AMS_PROXY_ENT_UPD_CHAR] = handle_ent_upd_char;
  proxy_env->client_handles[AMS_PROXY_ENT_UPD_VAL] = handle_ent_upd_val;
  proxy_env->client_handles[AMS_PROXY_ENT_UPD_CFG] = handle_ent_upd_cfg;
  proxy_env->client_handles[AMS_PROXY_ENT_ATTR_CHAR] = handle_ent_attr_char;
  proxy_env->client_handles[AMS_PROXY_ENT_ATTR_VAL] = handle_ent_attr_val;
  proxy_env->ready =
      (handle_rem_cmd_char && handle_rem_cmd_val && handle_rem_cmd_cfg &&
       handle_ent_upd_char && handle_ent_upd_val && handle_ent_upd_cfg &&
       handle_ent_attr_char && handle_ent_attr_val);
  GSOUND_TRACE(DBG_NAME "%s done. ready=%d", __func__, proxy_env->ready);
  ams_proxy_notify_ready_flag();
}

static int get_client_handle_from_server_handle_idx(int server_handle_idx) {
  if (server_handle_idx > AMS_PROXY_NUM) return 0;

  ProxyEnv *proxy_env = PROXY_ENV_GET();
  return (proxy_env->client_handles[server_handle_idx]);
}

static int get_server_handle_from_client_handle(int client_handle) {
  ProxyEnv *proxy_env = PROXY_ENV_GET();
  for (int i = 0; i < AMS_PROXY_NUM; i++) {
    if (proxy_env->client_handles[i] == client_handle) {
      return proxy_env->start_hdl + i;
    }
  }

  // Not found.
  return 0;
}

/**
 *
 */
static uint8_t proxy_init(struct prf_task_env *env, uint16_t *start_hdl,
                          uint16_t app_task, uint8_t sec_lvl, void *params) {
  GSOUND_TRACE(DBG_NAME "%s entry.", __func__);
  uint8_t svc_perm =
      (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) |
      PERM(SVC_MI, DISABLE) | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128);
  uint8_t status = attm_svc_create_db_128(start_hdl, PROXY_SERVICE_UUID_128,
                                          NULL, ARRAY_SIZE(proxy_att_db), NULL,
                                          env->task, proxy_att_db, svc_perm);

  GSOUND_TRACE(DBG_NAME "%s: status=%d, start_handle=0x%4.4x", __func__, status,
        *start_hdl);

  if (status != ATT_ERR_NO_ERROR) {
    GSOUND_TRACE(DBG_NAME "%s error.", __func__);
    return status;
  }

  // Allocate Profile Environment Data
  ProxyEnv *proxy_env = ke_malloc(sizeof(ProxyEnv), KE_MEM_ATT_DB);
  ASSERT(proxy_env, "%s alloc error", __func__);
  memset(proxy_env, 0, sizeof(ProxyEnv));
  proxy_env->prf_env.app_task =
      app_task | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE)
                                            : PERM(PRF_MI, DISABLE));
  proxy_env->prf_env.prf_task = env->task | 
  	(PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
  proxy_env->start_hdl = *start_hdl;

  // Initialize environment
  env->env = (prf_env_t *)proxy_env;
  env->id = TASK_ID_AMSP;
  proxy_task_init(&(env->desc));
  ke_state_set(env->task, 0);

  GSOUND_TRACE(DBG_NAME "%s leaving. ready=%d", __func__, proxy_env->ready);
  return status;
}

/**
 *
 */
static void proxy_destroy(struct prf_task_env *env) {
  ProxyEnv *proxy_env = (ProxyEnv *)env->env;
  env->env = NULL;
  ke_free(proxy_env);
}

/**
 *
 */
static void proxy_create(struct prf_task_env *env, uint8_t conidx) {
  ProxyEnv *proxy_env = (ProxyEnv *)env->env;

  proxy_env->conidx = conidx;

}

static void proxy_cleanup(struct prf_task_env *env, uint8_t conidx,
                          uint8_t reason) {
  ProxyEnv *proxy_env = (ProxyEnv *)env->env;
  proxy_env->ready = false;
  proxy_env->ready_ntf_enable = false;
  memset(&proxy_env->client_handles[0], 0, sizeof(proxy_env->client_handles));
}

static const struct prf_task_cbs proxy_itf = {
    proxy_init,
    proxy_destroy,
    proxy_create,
    proxy_cleanup,
};

const struct prf_task_cbs *ams_proxy_prf_itf_get(void) {
  GSOUND_TRACE(DBG_NAME "%s entry.", __func__);
  return &proxy_itf;
}

KE_MSG_HANDLER_TAB(proxy){
    // from GATTC
    {GATTC_WRITE_REQ_IND, (ke_msg_func_t)gattc_write_req_ind_handler},  // X
    {GATTC_READ_REQ_IND, (ke_msg_func_t)gattc_read_req_ind_handler},    // X
    // skipped from client
    {GATTC_ATT_INFO_REQ_IND, (ke_msg_func_t)gattc_att_info_req_ind_handler},
    {AMS_PROXY_IND_EVT, (ke_msg_func_t)proxy_ind_evt_handler},
    {GATTC_READ_CFM, (ke_msg_func_t)gattc_read_cfm_handler},
    {GATTC_WRITE_CFM, (ke_msg_func_t)gattc_write_cfm_handler},
};

static void proxy_task_init(struct ke_task_desc *task_desc) {
  GSOUND_TRACE(DBG_NAME "%s entry.", __func__);
  ProxyEnv *proxy_env = PROXY_ENV_GET();
  task_desc->msg_handler_tab = proxy_msg_handler_tab;
  task_desc->msg_cnt = ARRAY_SIZE(proxy_msg_handler_tab);
  task_desc->state = &(proxy_env->state);
  task_desc->idx_max = BLE_CONNECTION_MAX;
  memset(&proxy_env->client_handles[0], 0, sizeof(proxy_env->client_handles));
}

static int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_write_req_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id) {
  ProxyEnv *proxy_env = PROXY_ENV_GET();
  int handle_idx = param->handle - proxy_env->start_hdl;
  int client_handle = get_client_handle_from_server_handle_idx(handle_idx);
  struct gattc_write_cfm *cfm =
      KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
  cfm->status = ATT_ERR_APP_ERROR;
  cfm->handle = param->handle;

  GSOUND_TRACE(DBG_NAME
        "%s entry. hdl=0x%4.4x,hdl_idx=%d,cliHdl=0x%x,offset=%d,val0=0x%x",
        __func__, param->handle, handle_idx, client_handle, param->length,
        param->value[0]);

  switch (handle_idx) {
    case AMS_PROXY_READY_CHAR:
    case AMS_PROXY_READY_VAL:
      // Just break. These are errors.
      cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
      break;
    case AMS_PROXY_READY_CFG:
      // process locally
      if (param->length == 2) {
        uint16_t cccd_val =
            (((uint16_t)param->value[1]) << 8) | ((uint16_t)param->value[0]);
        proxy_env->ready_ntf_enable = ((cccd_val & PRF_CLI_START_NTF) != 0);
        GSOUND_TRACE(DBG_NAME "write %s cccd_val=0x%x", __func__,
              proxy_env->ready_ntf_enable);
        cfm->status = GAP_ERR_NO_ERROR;
      }
      break;
    default: {
      // TODO(jkessinger): Maybe check for valid handles.
      if (proxy_env->ready) {
        struct gattc_write_cmd *cmd;
        ke_task_id_t dest_task = prf_get_task_from_id(CLIENT_TASK_ID);
        ke_task_id_t src_task = dest_id;
        cmd = KE_MSG_ALLOC_DYN(AMSC_WRITE_CMD, KE_BUILD_ID(dest_task, proxy_env->conidx), 
								src_task,
                               	gattc_write_cmd, param->length);

        cmd->auto_execute = true;
        cmd->cursor = 0;
        cmd->seq_num = 0;
        cmd->operation = GATTC_WRITE;

        cmd->handle = client_handle;
        cmd->offset = param->offset;
        cmd->length = param->length;
        memcpy(cmd->value, param->value, param->length);
        ke_msg_send(cmd);
        KE_MSG_FREE(cfm);
        cfm = NULL;
      } else {
        cfm->status =
            ATT_ERR_APP_ERROR;  // TODO(mfk): Is this the right error code?
      }
    } break;
  }
  if (cfm != NULL) {
    GSOUND_TRACE(DBG_NAME "%s send_write_cfm status=%d", __func__, cfm->status);
    ke_msg_send(cfm);
  }

  return KE_MSG_CONSUMED;
}

static int gattc_write_cfm_handler(ke_msg_id_t const msgid,
                                   struct gattc_write_cfm const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id) {
  int server_handle = get_server_handle_from_client_handle(param->handle);

  GSOUND_TRACE(DBG_NAME "%s entry. cl_hdl=0x%4.4x, hdl=0x%4.4x, status=%d", __func__,
        param->handle, server_handle, param->status);

  ke_task_id_t dest_task = KE_BUILD_ID(TASK_GATTC, KE_IDX_GET(dest_id));
  ke_task_id_t src_task = dest_id;

  // TODO(jkessinger): Consider ke_msg_forward
  struct gattc_write_cfm *cfm =
      KE_MSG_ALLOC(GATTC_WRITE_CFM, dest_task, src_task, gattc_write_cfm);
  cfm->status = param->status;
  cfm->handle = server_handle;
  ke_msg_send(cfm);
  return KE_MSG_CONSUMED;
}

static int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id) {
  ProxyEnv *proxy_env = PROXY_ENV_GET();
  int handle_idx = param->handle - proxy_env->start_hdl;
  int client_handle = get_client_handle_from_server_handle_idx(handle_idx);
  uint8_t conidx = KE_IDX_GET(src_id);

  GSOUND_TRACE(DBG_NAME "%s entry. hdl=0x%4.4x, hdl_idx=%d, cliHdl=0x%x", __func__,
        param->handle, handle_idx, client_handle);

  switch (handle_idx) {
    case AMS_PROXY_READY_VAL:
    case AMS_PROXY_READY_CFG: {
      // process locally
      struct gattc_read_cfm *cfm =
          KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, 2);
      cfm->status = ATT_ERR_APP_ERROR;
      cfm->length = 2;
      if (handle_idx == AMS_PROXY_READY_CFG) {
        GSOUND_TRACE(DBG_NAME "%s read cccd val=0x%x", __func__,
              proxy_env->ready_ntf_enable);
        cfm->value[0] = proxy_env->ready_ntf_enable ? PRF_CLI_START_NTF : 0;
        cfm->status = GAP_ERR_NO_ERROR;
      } else if (handle_idx == AMS_PROXY_READY_VAL) {
        GSOUND_TRACE(DBG_NAME "%s read ready val=0x%x", __func__, proxy_env->ready);
        cfm->value[0] = proxy_env->ready ? PRF_CLI_START_NTF : 0;
        cfm->status = GAP_ERR_NO_ERROR;
      }
      cfm->value[1] = 0;
      cfm->handle = param->handle;
      ke_msg_send(cfm);
    } break;
    case AMS_PROXY_REMOTE_CMD_CFG:
    case AMS_PROXY_ENT_UPD_CFG:
    case AMS_PROXY_ENT_ATTR_VAL: {
      if (proxy_env->ready) {
        struct gattc_read_cmd *cmd;
        ke_task_id_t dest_task = prf_get_task_from_id(CLIENT_TASK_ID);
        ke_task_id_t src_task = prf_get_task_from_id(PROXY_TASK_ID);
        cmd = KE_MSG_ALLOC(AMSC_READ_CMD, KE_BUILD_ID(dest_task, conidx), 
			src_task, gattc_read_cmd);

        cmd->operation = GATTC_READ;
        cmd->nb = 1;
        cmd->req.simple.handle = client_handle;
        cmd->req.simple.length = 0;  // not used
        cmd->req.simple.offset = 0;  // not used
        ke_msg_send(cmd);
      } else {
        struct gattc_read_cfm *cfm =
            KE_MSG_ALLOC(GATTC_READ_CFM, KE_BUILD_ID(TASK_GATTC, conidx),
                         prf_src_task_get(&proxy_env->prf_env, conidx),
                         gattc_read_cfm);
        cfm->status =
            ATT_ERR_APP_ERROR;  // TODO(mfk): Is this the right error code?
        cfm->handle = param->handle;
        cfm->length = 0;
        ke_msg_send(cfm);
      }
    } break;
    default: {
      struct gattc_read_cfm *cfm =
          KE_MSG_ALLOC(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm);
      cfm->status = ATT_ERR_INVALID_HANDLE;
      cfm->length = 0;
      ke_msg_send(cfm);
    } break;
  }

  return KE_MSG_CONSUMED;
}

static int gattc_read_cfm_handler(ke_msg_id_t const msgid,
                                  struct gattc_read_cfm const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id) {
  int server_handle = get_server_handle_from_client_handle(param->handle);

  GSOUND_TRACE(DBG_NAME "%s entry. cl_hdl=0x%4.4x, hdl=0x%4.4x, status=%d, len=%d",
        __func__, param->handle, server_handle, param->status,
        param->length);

  ke_task_id_t dest_task = KE_BUILD_ID(TASK_GATTC, KE_IDX_GET(dest_id));
  ke_task_id_t src_task = dest_id;

  struct gattc_read_cfm *cfm = KE_MSG_ALLOC_DYN(
      GATTC_READ_CFM, dest_task, src_task, gattc_read_cfm, param->length);
  cfm->status = param->status;
  cfm->handle = server_handle;
  cfm->length = param->length;
  memcpy(cfm->value, param->value, param->length);
  ke_msg_send(cfm);
  return KE_MSG_CONSUMED;
}

// TODO(jkessinger): Remove. Not called.
static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
                                          struct gattc_att_info_req_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id) {
  GSOUND_TRACE(DBG_NAME "%s entry. hdl=0x%4.4x", __func__, param->handle);
  ASSERT(0, DBG_NAME "%s not yet implemented", __func__);
  ProxyEnv *proxy_env = PROXY_ENV_GET();
  struct gattc_att_info_cfm *cfm =
      KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
  cfm->handle = param->handle;

  if ((param->handle == (proxy_env->start_hdl + AMS_PROXY_REMOTE_CMD_CFG)) ||
      (param->handle == (proxy_env->start_hdl + AMS_PROXY_ENT_UPD_CFG))) {
    // CCC attribute length = 2
    cfm->length = 2;
    cfm->status = GAP_ERR_NO_ERROR;
  } else if ((param->handle == (proxy_env->start_hdl + AMS_PROXY_READY_VAL)) ||
             (param->handle ==
              (proxy_env->start_hdl + AMS_PROXY_ENT_ATTR_VAL))) {
    cfm->length = 0;
    cfm->status = GAP_ERR_NO_ERROR;
  } else {
    cfm->length = 0;
    cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
  }

  ke_msg_send(cfm);
  return KE_MSG_CONSUMED;
}

static int proxy_ind_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_send_evt_cmd *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id) {
  ProxyEnv *proxy_env = PROXY_ENV_GET();
  int server_handle = get_server_handle_from_client_handle(param->handle);

  GSOUND_TRACE(DBG_NAME "%s entry. cl_hdl=0x%4.4x, hdl=0x%x, op=%d, len=%d",
        __func__, param->handle, server_handle, param->operation,
        param->length);

  // TODO(jkessinger): Forward the cmd.
  struct gattc_send_evt_cmd *cmd = KE_MSG_ALLOC_DYN(
      GATTC_SEND_EVT_CMD, KE_BUILD_ID(TASK_GATTC, proxy_env->conidx),
      prf_src_task_get(&proxy_env->prf_env, proxy_env->conidx), gattc_send_evt_cmd,
      param->length);
  cmd->handle = server_handle;
  cmd->operation = param->operation;
  cmd->seq_num = param->seq_num;
  cmd->length = param->length;
  memcpy(cmd->value, param->value, param->length);
  ke_msg_send(cmd);

  return KE_MSG_CONSUMED;
}

#endif /* (ANCS_PROXY_ENABLE) */
