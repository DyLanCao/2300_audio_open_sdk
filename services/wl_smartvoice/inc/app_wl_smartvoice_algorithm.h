#ifndef __APP_WL_SMARTVOICE_ALGORITHM_H
#define __APP_WL_SMARTVOICE_ALGORITHM_H

#include "hal_trace.h"

#define APP_WL_SMARTVOICE_WEBRTC_NSX_BUFF_SIZE    	(14000)

#ifdef __cplusplus
extern "C" {
#endif

void app_wl_smartvoice_algorithm_init(uint8_t* nsx_heap);

void app_wl_smartvoice_algorithm_deinit(void);

void app_wl_smartvoice_algorithm_process(uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif