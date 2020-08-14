#ifndef GSOUND_ANCS_PROXY_SERVER_H
#define GSOUND_ANCS_PROXY_SERVER_H

#include "attm.h"
#include "prf.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "rwip_config.h"


typedef enum {
  ANCS_PROXY_SVC,

  ANCS_PROXY_NS_CHAR,
  ANCS_PROXY_NS_VAL,
  ANCS_PROXY_NS_CFG,

  ANCS_PROXY_READY_CHAR,
  ANCS_PROXY_READY_VAL,
  ANCS_PROXY_READY_CFG,

  ANCS_PROXY_DS_CHAR,
  ANCS_PROXY_DS_VAL,
  ANCS_PROXY_DS_CFG,

  ANCS_PROXY_CP_CHAR,
  ANCS_PROXY_CP_VAL,

  ANCS_PROXY_NUM,
} ancs_gatt_handles;

enum ancs_proxy_msg_id {
  ANCS_PROXY_IND_EVT = TASK_FIRST_MSG(TASK_ID_ANCSP),
};

const struct prf_task_cbs* ancs_proxy_prf_itf_get(void);
void ancs_proxy_set_ready_flag(int handle_ns_char, int handle_ns_val,
                               int handle_ns_cfg, int handle_ds_char,
                               int handle_ds_val, int handle_ds_cfg,
                               int handle_cp_char, int handle_cp_val);

#endif /* GSOUND_ANCS_PROXY_SERVER_H */
