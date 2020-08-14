#ifndef __APP_WL_SMARTVOICE_SPP_H
#define __APP_WL_SMARTVOICE_SPP_H

#include "hal_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    APP_WL_SMARTVOICE_SPP_CONNECTED_EVENT = 0x01,
    APP_WL_SMARTVOICE_SPP_DISCONNECTED_EVENT,
    APP_WL_SMARTVOICE_SPP_TX_DONE,
};

typedef void (*APP_WL_SMARTVOICE_SPP_EVENT_HANDLER)(int, void*);

typedef void (*APP_WL_SMARTVOICE_SPP_DATA_RECEIVED_HANDLER)(uint8_t*, uint16_t);

void app_wl_smartvoice_spp_key_code_send(uint8_t* data, uint16_t length);

void app_wl_smartvoice_spp_command_send(uint8_t* data, uint16_t length);

void app_wl_smartvoice_spp_stream_send(uint8_t* data, uint16_t length);

void app_wl_smartvoice_spp_event_subscribe(APP_WL_SMARTVOICE_SPP_EVENT_HANDLER cb);

void app_wl_smartvoice_spp_cmd_received_subscribe(APP_WL_SMARTVOICE_SPP_DATA_RECEIVED_HANDLER cb);

#ifdef __cplusplus
}
#endif

#endif