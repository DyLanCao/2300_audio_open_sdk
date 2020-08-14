#ifndef GSOUND_AMS_PROXY_SERVER_H
#define GSOUND_AMS_PROXY_SERVER_H

#include "attm.h"
#include "prf.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "rwip_config.h"


typedef enum {
  AMS_PROXY_SVC,

  AMS_PROXY_REMOTE_CMD_CHAR,
  AMS_PROXY_REMOTE_CMD_VAL,
  AMS_PROXY_REMOTE_CMD_CFG,
  AMS_PROXY_READY_CHAR,
  AMS_PROXY_READY_VAL,
  AMS_PROXY_READY_CFG,

  AMS_PROXY_ENT_UPD_CHAR,
  AMS_PROXY_ENT_UPD_VAL,
  AMS_PROXY_ENT_UPD_CFG,

  AMS_PROXY_ENT_ATTR_CHAR,
  AMS_PROXY_ENT_ATTR_VAL,

  AMS_PROXY_NUM,
} ams_gatt_handles;

enum ams_proxy_msg_id {
  AMS_PROXY_IND_EVT = TASK_FIRST_MSG(TASK_ID_AMSP),
};

const struct prf_task_cbs* ams_proxy_prf_itf_get(void);
void ams_proxy_set_ready_flag(int handle_rem_cmd_char, int handle_rem_cmd_val,
                              int handle_rem_cmd_cfg, int handle_ent_upd_char,
                              int handle_ent_upd_val, int handle_ent_upd_cfg,
                              int handle_ent_attr_char,
                              int handle_ent_attr_val);

#endif /* GSOUND_AMS_PROXY_SERVER_H */
