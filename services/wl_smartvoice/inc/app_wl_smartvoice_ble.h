#ifndef __APP_WL_SMARTVOICE_BLE_H
#define __APP_WL_SMARTVOICE_BLE_H

#include "hal_trace.h"


#define WL_SMARTVOICE_ADV_UUID           "\x03\x03\x00\x04"
#define WL_SMARTVOICE_ADV_UUID_LEN       (4)

#ifdef __cplusplus
extern "C" {
#endif

enum {
    APP_WL_SMARTVOICE_BLE_CONNECTED_EVENT = 0x01,
    APP_WL_SMARTVOICE_BLE_DISCONNECTED_EVENT,
    APP_WL_SMARTVOICE_BLE_KEYCODE_TX_ENABLE_EVENT,
    APP_WL_SMARTVOICE_BLE_COMMAND_TX_ENABLE_EVENT,
    APP_WL_SMARTVOICE_BLE_STREAM_TX_ENABLE_EVENT,
    APP_WL_SMARTVOICE_BLE_TX_DONE,
};

typedef void (*APP_WL_SMARTVOICE_EVENT_HANDLER)(int, void*);
typedef void (*APP_WL_SMARTVOICE_DATA_RECEIVED_HANDLER)(uint8_t*, uint16_t);


typedef struct {
    uint8_t conidx;
    bool b_keycode_tx_enable;
    bool b_command_tx_enable;
    bool b_stream_tx_enable;

    APP_WL_SMARTVOICE_EVENT_HANDLER event_handler;
    APP_WL_SMARTVOICE_DATA_RECEIVED_HANDLER data_handler;
} app_wl_smartvoice_ble_ctx_t;

void app_wl_smartvoice_ble_init(void);
void app_wl_smartvoice_ble_event_subscribe(APP_WL_SMARTVOICE_EVENT_HANDLER callback);
void app_wl_smartvoice_ble_cmd_received_subscribe(APP_WL_SMARTVOICE_DATA_RECEIVED_HANDLER callback);

void app_wl_smartvoice_ble_key_code_send(uint8_t* data, uint16_t length);
void app_wl_smartvoice_ble_command_send(uint8_t* data, uint16_t length);
void app_wl_smartvoice_ble_stream_send(uint8_t* data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif