#ifndef __APP_WL_SMARTVOICE_CHANNEL_H
#define __APP_WL_SMARTVOICE_CHANNEL_H

#include "hal_trace.h"
#include "cmsis_os.h"

#define APP_WL_SMARTVOICE_BLE_TX_QUOTA  (6)

#ifdef __cplusplus
extern "C" {
#endif

enum {
    APP_WL_SMARTVOICE_CHANNEL_INVALID = 0,
    APP_WL_SMARTVOICE_CHANNEL_BLE,
    APP_WL_SMARTVOICE_CHANNEL_SPP,
};

enum {
    APP_WL_SMARTVOICE_CHANNEL_EVENT_INVALID = 0,
    APP_WL_SMARTVOICE_CHANNEL_EVENT_CONNECTED,
    APP_WL_SMARTVOICE_CHANNEL_EVENT_DISCONNECTED,
};

typedef void (*CHANNEL_DATA_IND_CB)(uint8_t*, uint16_t, uint8_t);

typedef void (*CHANNEL_DATA_DONE_CB)(void);

typedef void (*CHANNEL_EVENT_IND_CB)(int);

typedef struct {
    CHANNEL_DATA_IND_CB data_ind_handler;
    CHANNEL_DATA_DONE_CB data_tx_done_handler;
    CHANNEL_EVENT_IND_CB event_handler;

    bool b_spp_connected;
    bool b_ble_connected;

    osMutexId tx_quota_mutex;
    uint8_t tx_quota;
} app_wl_smartvoice_channel_ctx_t;

void app_wl_smartvoice_channle_init(void);

void app_wl_smartvoice_send_key_status(uint8_t* data, uint16_t length);
void app_wl_smartvoice_send_control_data(uint8_t* data, uint16_t length);
void app_wl_smartvoice_send_voice_data(uint8_t* data, uint16_t length);

void app_wl_smartvoice_channel_data_subscribe(CHANNEL_DATA_IND_CB cb);

void app_wl_smartvoice_channel_data_tx_done_subscribe(CHANNEL_DATA_DONE_CB cb);

void app_wl_smartvoice_channel_event_subscribe(CHANNEL_EVENT_IND_CB cb);

uint8_t app_wl_smartvoice_current_channel_get(void);

#ifdef __cplusplus
}
#endif

#endif