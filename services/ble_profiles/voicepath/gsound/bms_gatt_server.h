#ifndef BMS_GATT_SERVER_H
#define BMS_GATT_SERVER_H

#include "rwip_config.h"
#include "prf_types.h"
#include "prf.h"
#include "attm.h"
#include "prf_utils.h"


/**
 * Will return interface to BMS GATT server
 */
const struct prf_task_cbs* bms_prf_itf_get(void);


#endif /* BMS_GATT_SERVER_H */
