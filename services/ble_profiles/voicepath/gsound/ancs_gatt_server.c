/**
 * Handles interaction between BLE stack
 * and ANCS server
 *
 *  - This file is allowed to have dependencies SoC platform.
 */
#include "ancs_gatt_server.h"
#include "rwapp_config.h"
#include "gsound.h"
#include "gsound_target_custom.h"


#if (ANCS_PROXY_ENABLE)

#include "ancc.h"
#include "ancc_task.h"
#include "ke_mem.h"

#define UUID_ARRAY(uuid) \
  { (uuid & 0xff), ((uuid >> 8) & 0xff) }

#define PROXY_TASK_ID TASK_ID_ANCSP
#define PROXY_ENV_GET() ((ProxyEnv *)prf_env_get(PROXY_TASK_ID))
#define CLIENT_TASK_ID (TASK_ID_ANCC)
#define DBG_NAME "ANCSP "

// Connection indexes are always 0 unless PERM_MI is enabled
// (multi-instantiated).
#define EMPTY_CON_IDX 0

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
  int client_handles[ANCS_PROXY_NUM];
} ProxyEnv;

#define proxy_service_uuid_128_content                                      \
  {                                                                         \
    0x70, 0x23, 0xCA, 0x99, 0x28, 0x7B, 0xD8, 0xA6, 0x1B, 0x45, 0x3E, 0xDE, \
        0xAD, 0x46, 0xA8, 0x67                                              \
  }
#define PROXY_CHAR_READY                                                    \
  {                                                                         \
    0x42, 0xC1, 0x2D, 0x7B, 0xFC, 0x67, 0xED, 0xBA, 0xBB, 0x45, 0x84, 0xA5, \
        0x35, 0xED, 0x3E, 0x75                                              \
  }
#define PROXY_CHAR_NS                                                       \
  {                                                                         \
    0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C, 0xD9, 0x42, 0x01, 0x63, \
        0x0D, 0x12, 0xBF, 0x9F                                              \
  }
#define PROXY_CHAR_DS                                                       \
  {                                                                         \
    0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE, 0xB5, 0x4B, 0xD6, 0x24, \
        0xE9, 0xC6, 0xEA, 0x22                                              \
  }
#define PROXY_CHAR_CP                                                       \
  {                                                                         \
    0xD9, 0xD9, 0xAA, 0xFD, 0xBD, 0x9B, 0x21, 0x98, 0xA8, 0x49, 0xE1, 0x45, \
        0xF3, 0xD8, 0xD1, 0x69                                              \
  }

// TODO(mfk): what is the right number?  Should be set for each attribute
// independently.
#define BISTO_MAX_LEN 244

/*
 * TODO(mfk):  Per p63 of bluetooth book:
 *     - value of config descriptors are unique per connection.  So, if we allow
 *       more than one phone to bond with the proxy, this must be done.
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
#define NTF_PERM PERM(NTF, ENABLE) | PERM(NP, AUTH)

/*
 * TODO(mfk): According to the bluetooth book, the "value" field in the first
 *            row of the table below should be the UUID of the service.  Maybe
 *            BES implementation of attm_svc_create_db_128 does this for us?
 */

/*******************************************************************************
 *******************************************************************************
 * ANCS Service - 67A846AD-DE3E-451B-A6D8-7B2899CA2370
 *******************************************************************************
 *******************************************************************************/
static const struct attm_desc_128 proxy_att_db[] = {

    /**************************************************
     *  ANCS Service Declaration
     **************************************************/

    [ANCS_PROXY_SVC] = {UUID_ARRAY(ATT_DECL_PRIMARY_SERVICE), PERM(RD, ENABLE),
                        0, 0},

    /**************************************************
     * ANCS Notification Source
     *    - UUID: 9FBF120D-6301-42D9-8C58-25E699A21DBD
     *    - Notifiable (Direct proxy of Apple's ANCS characteristic and matches
     *its UUID and properties)
     **************************************************/
    // Characteristic Declaration
    [ANCS_PROXY_NS_CHAR] = {UUID_ARRAY(ATT_DECL_CHARACTERISTIC),
                            PERM(RD, ENABLE), 0, 0},
    // Characteristic Value
    [ANCS_PROXY_NS_VAL] = {PROXY_CHAR_NS, NTF_PERM,
                           PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128),
                           BISTO_MAX_LEN},
    // Client Characteristic Configuration Descriptor
    [ANCS_PROXY_NS_CFG] = {UUID_ARRAY(ATT_DESC_CLIENT_CHAR_CFG),
                           RD_PERM | WRITE_REQ_PERM, 10, 0},

    /**************************************************
     * ANCS Ready (Google specific)
     *    - UUID: FBE87F6C-3F1A-44B6-B577-0BAC731F6E85
     *    - provides the current state of ANCS (0 - not ready, 1 - ready)
     *    - read/notify characteristic
     **************************************************/
    // Characteristic Declaration
    [ANCS_PROXY_READY_CHAR] = {UUID_ARRAY(ATT_DECL_CHARACTERISTIC),
                               PERM(RD, ENABLE), 0, 0},
    // Characteristic Value
    [ANCS_PROXY_READY_VAL] = {PROXY_CHAR_READY, NTF_PERM | RD_PERM,
                              PERM(RI, ENABLE) |
                                  PERM_VAL(UUID_LEN, PERM_UUID_128),
                              BISTO_MAX_LEN},
    // Client Characteristic Configuration Descriptor
    [ANCS_PROXY_READY_CFG] = {UUID_ARRAY(ATT_DESC_CLIENT_CHAR_CFG),
                              RD_PERM | WRITE_REQ_PERM, 10, 0},

    /**************************************************
     * ANCS Data Source
     *    - UUID: 22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB
     *    - Notifiable (Direct proxy of Apple's ANCS characteristic and matches
     *its UUID and properties)
     **************************************************/
    // Characteristic Declaration
    [ANCS_PROXY_DS_CHAR] = {UUID_ARRAY(ATT_DECL_CHARACTERISTIC),
                            PERM(RD, ENABLE), 0, 0},
    // Characteristic Value
    [ANCS_PROXY_DS_VAL] = {PROXY_CHAR_DS, NTF_PERM,
                           PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128),
                           BISTO_MAX_LEN},
    // Client Characteristic Configuration Descriptor
    [ANCS_PROXY_DS_CFG] = {UUID_ARRAY(ATT_DESC_CLIENT_CHAR_CFG),
                           RD_PERM | WRITE_REQ_PERM, 0, 0},

    /**************************************************
     * ANCS Control Point
     *    - UUID: 69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9
     *    - Writeable with response (Direct proxy of Apple's ANCS characteristic
     *and matches its UUID and properties)
     **************************************************/
    // Characteristic Declaration
    [ANCS_PROXY_CP_CHAR] = {UUID_ARRAY(ATT_DECL_CHARACTERISTIC),
                            PERM(RD, ENABLE), 0, 0},
    // Characteristic Value
    [ANCS_PROXY_CP_VAL] = {PROXY_CHAR_CP, WRITE_REQ_PERM,
                           PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128),
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

static void ancs_proxy_notify_ready_flag() {
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
    cmd->handle = proxy_env->start_hdl + ANCS_PROXY_READY_VAL;
    cmd->length = 2;
    cmd->value[0] = proxy_env->ready ? PRF_CLI_START_NTF : 0;
    cmd->value[1] = 0;
    ke_msg_send(cmd);
  }
}

void ancs_proxy_set_ready_flag(int handle_ns_char, int handle_ns_val,
                               int handle_ns_cfg, int handle_ds_char,
                               int handle_ds_val, int handle_ds_cfg,
                               int handle_cp_char, int handle_cp_val) {
  // TODO(jkessinger): This is called with zeros if the service does not exist.
  // Maybe set ready = false.
  ProxyEnv *proxy_env = PROXY_ENV_GET();
  GSOUND_TRACE(DBG_NAME "%s entry. pvReady=%d enable=%d", __func__, proxy_env->ready,
        proxy_env->ready_ntf_enable);

  proxy_env->client_handles[ANCS_PROXY_NS_CHAR] = handle_ns_char;
  proxy_env->client_handles[ANCS_PROXY_NS_VAL] = handle_ns_val;
  proxy_env->client_handles[ANCS_PROXY_NS_CFG] = handle_ns_cfg;
  proxy_env->client_handles[ANCS_PROXY_DS_CHAR] = handle_ds_char;
  proxy_env->client_handles[ANCS_PROXY_DS_VAL] = handle_ds_val;
  proxy_env->client_handles[ANCS_PROXY_DS_CFG] = handle_ds_cfg;
  proxy_env->client_handles[ANCS_PROXY_CP_CHAR] = handle_cp_char;
  proxy_env->client_handles[ANCS_PROXY_CP_VAL] = handle_cp_val;
  proxy_env->ready =
      (handle_ns_char && handle_ns_val && handle_ns_cfg && handle_ds_char &&
       handle_ds_val && handle_ds_cfg && handle_cp_char && handle_cp_val);
  GSOUND_TRACE(DBG_NAME "%s done. ready=%d", __func__, proxy_env->ready);
  ancs_proxy_notify_ready_flag();
}

static int get_client_handle_from_server_handle_idx(int server_handle_idx) {
  if (server_handle_idx > ANCS_PROXY_NUM) return 0;

  ProxyEnv *proxy_env = PROXY_ENV_GET();
  return (proxy_env->client_handles[server_handle_idx]);
}

static int get_server_handle_from_client_handle(int client_handle) {
  ProxyEnv *proxy_env = PROXY_ENV_GET();
  for (int i = 0; i < ANCS_PROXY_NUM; i++) {
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
  env->id = TASK_ID_ANCSP;
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
  struct prf_svc proxy_svc = {proxy_env->start_hdl,
                              proxy_env->start_hdl + ARRAY_SIZE(proxy_att_db)};
  proxy_env->conidx = conidx;

  GSOUND_TRACE(DBG_NAME "%s create: conidx=%d start=0x%x end=0x%x", __func__, conidx,
        proxy_svc.shdl, proxy_svc.ehdl);
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

const struct prf_task_cbs *ancs_proxy_prf_itf_get(void) {
  GSOUND_TRACE(DBG_NAME "%s entry.", __func__);
  return &proxy_itf;
}

KE_MSG_HANDLER_TAB(proxy){
    // from GATTC
    {GATTC_WRITE_REQ_IND, (ke_msg_func_t)gattc_write_req_ind_handler},  // X
    {GATTC_READ_REQ_IND, (ke_msg_func_t)gattc_read_req_ind_handler},    // X
    // skipped from client
    {GATTC_ATT_INFO_REQ_IND, (ke_msg_func_t)gattc_att_info_req_ind_handler},
    {ANCS_PROXY_IND_EVT, (ke_msg_func_t)proxy_ind_evt_handler},
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
    case ANCS_PROXY_READY_CHAR:
    case ANCS_PROXY_READY_VAL:
      // Just break. These are errors.
      cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
      break;
    case ANCS_PROXY_READY_CFG:
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
        cmd = KE_MSG_ALLOC_DYN(ANCC_WRITE_CMD, KE_BUILD_ID(dest_task, proxy_env->conidx),
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

  GSOUND_TRACE(DBG_NAME "%s entry. conidx %d hdl=0x%4.4x, hdl_idx=%d, cliHdl=0x%x", __func__,
        conidx, param->handle, handle_idx, client_handle);

  switch (handle_idx) {
    case ANCS_PROXY_READY_VAL:
    case ANCS_PROXY_READY_CFG: {
      // process locally
      struct gattc_read_cfm *cfm =
          KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, 
          dest_id, gattc_read_cfm, 2);
      cfm->status = ATT_ERR_APP_ERROR;
      cfm->length = 2;
      if (handle_idx == ANCS_PROXY_READY_CFG) {
        GSOUND_TRACE(DBG_NAME "%s read cccd val=0x%x", __func__,
              proxy_env->ready_ntf_enable);
        cfm->value[0] = proxy_env->ready_ntf_enable ? PRF_CLI_START_NTF : 0;
        cfm->status = GAP_ERR_NO_ERROR;
      } else if (handle_idx == ANCS_PROXY_READY_VAL) {
        GSOUND_TRACE(DBG_NAME "%s read ready val=0x%x", __func__, proxy_env->ready);
        cfm->value[0] = proxy_env->ready ? PRF_CLI_START_NTF : 0;
        cfm->status = GAP_ERR_NO_ERROR;
      }
      cfm->value[1] = 0;
      cfm->handle = param->handle;
      ke_msg_send(cfm);
    } break;
    case ANCS_PROXY_NS_CFG:
    case ANCS_PROXY_DS_CFG: {
      if (proxy_env->ready) {
        struct gattc_read_cmd *cmd;
        ke_task_id_t dest_task = prf_get_task_from_id(CLIENT_TASK_ID);
        ke_task_id_t src_task = prf_get_task_from_id(PROXY_TASK_ID);
        cmd = KE_MSG_ALLOC(ANCC_READ_CMD, KE_BUILD_ID(dest_task, conidx), 
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
          KE_MSG_ALLOC(GATTC_READ_CFM, conidx, 
          dest_id, gattc_read_cfm);
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

  if ((param->handle == (proxy_env->start_hdl + ANCS_PROXY_NS_CFG)) ||
      (param->handle == (proxy_env->start_hdl + ANCS_PROXY_DS_CFG))) {
    // CCC attribute length = 2
    cfm->length = 2;
    cfm->status = GAP_ERR_NO_ERROR;
  } else if ((param->handle == (proxy_env->start_hdl + ANCS_PROXY_READY_VAL)) ||
             (param->handle == (proxy_env->start_hdl + ANCS_PROXY_CP_VAL))) {
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
  uint8_t conidx = KE_IDX_GET(src_id);

  GSOUND_TRACE(DBG_NAME "%s entry. cl_hdl=0x%4.4x, hdl=0x%4.4x, op=%d, len=%d",
        __func__, param->handle, server_handle, param->operation,
        param->length);

  struct gattc_send_evt_cmd *cmd = KE_MSG_ALLOC_DYN(
      GATTC_SEND_EVT_CMD, KE_BUILD_ID(TASK_GATTC, conidx),
      prf_src_task_get(&proxy_env->prf_env, conidx), gattc_send_evt_cmd,
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
