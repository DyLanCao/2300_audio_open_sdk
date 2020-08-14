#include "app_wl_smartvoice_channel.h"
#include "app_wl_smartvoice_ble.h"
#include "app_wl_smartvoice_cmd.h"
#include "app_wl_smartvoice_spp.h"
#include "app_wl_smartvoice.h"
#include "string.h"


osMutexDef(tx_quota_mutex);
app_wl_smartvoice_channel_ctx_t _app_wl_smartvoice_channel_ctx;
app_wl_smartvoice_channel_ctx_t* app_wl_smartvoice_channel_ctx = &_app_wl_smartvoice_channel_ctx;

static uint8_t app_wl_smartvoice_tx_quota_num_get(void)
{
    uint8_t tx_quota;
    osMutexWait(app_wl_smartvoice_channel_ctx->tx_quota_mutex, osWaitForever);
    tx_quota = app_wl_smartvoice_channel_ctx->tx_quota;
    osMutexRelease(app_wl_smartvoice_channel_ctx->tx_quota_mutex);

    return tx_quota;
}

static bool app_wl_smartvoice_tx_quota_available(void)
{
    uint8_t tx_quota;
    osMutexWait(app_wl_smartvoice_channel_ctx->tx_quota_mutex, osWaitForever);
    tx_quota = app_wl_smartvoice_channel_ctx->tx_quota;
    osMutexRelease(app_wl_smartvoice_channel_ctx->tx_quota_mutex);

    return tx_quota > 0;
}

static void app_wl_smartvoice_tx_quota_acquire(void)
{
    osMutexWait(app_wl_smartvoice_channel_ctx->tx_quota_mutex, osWaitForever);
    app_wl_smartvoice_channel_ctx->tx_quota--;
    osMutexRelease(app_wl_smartvoice_channel_ctx->tx_quota_mutex);
}


static void app_wl_smartvoice_tx_quota_release(void)
{
    osMutexWait(app_wl_smartvoice_channel_ctx->tx_quota_mutex, osWaitForever);
    app_wl_smartvoice_channel_ctx->tx_quota++;
    if (app_wl_smartvoice_channel_ctx->tx_quota > APP_WL_SMARTVOICE_BLE_TX_QUOTA) {
        app_wl_smartvoice_channel_ctx->tx_quota = APP_WL_SMARTVOICE_BLE_TX_QUOTA;
    }
    osMutexRelease(app_wl_smartvoice_channel_ctx->tx_quota_mutex);
}

static void app_wl_smartvoice_tx_quota_reset(void)
{
    osMutexWait(app_wl_smartvoice_channel_ctx->tx_quota_mutex, osWaitForever);
    app_wl_smartvoice_channel_ctx->tx_quota = APP_WL_SMARTVOICE_BLE_TX_QUOTA;
    osMutexRelease(app_wl_smartvoice_channel_ctx->tx_quota_mutex);
}

static void app_wl_smartvoice_channel_event_post(int event)
{
    if (app_wl_smartvoice_channel_ctx->event_handler) {
        app_wl_smartvoice_channel_ctx->event_handler(event);
    }
}

static void app_wl_smartvoice_ble_event_handler(int event, void* info)
{
    WL_SV_FUNC_ENTER();

    WL_SV_DEBUG("### event: %d ###", event);
    switch (event)
    {
    case APP_WL_SMARTVOICE_BLE_CONNECTED_EVENT:
        app_wl_smartvoice_channel_ctx->b_ble_connected = true;
        app_wl_smartvoice_tx_quota_reset();
        if (APP_WL_SMARTVOICE_CHANNEL_BLE == app_wl_smartvoice_current_channel_get()) {
            app_wl_smartvoice_channel_event_post(APP_WL_SMARTVOICE_CHANNEL_EVENT_CONNECTED);
        }
        break;
    
    case APP_WL_SMARTVOICE_BLE_DISCONNECTED_EVENT:
        if (APP_WL_SMARTVOICE_CHANNEL_BLE == app_wl_smartvoice_current_channel_get()) {
            app_wl_smartvoice_channel_event_post(APP_WL_SMARTVOICE_CHANNEL_EVENT_DISCONNECTED);
        }
        app_wl_smartvoice_channel_ctx->b_ble_connected = false;

        break;

    case APP_WL_SMARTVOICE_BLE_KEYCODE_TX_ENABLE_EVENT:
        break;

    case APP_WL_SMARTVOICE_BLE_COMMAND_TX_ENABLE_EVENT:
        break;

    case APP_WL_SMARTVOICE_BLE_STREAM_TX_ENABLE_EVENT:
        break;

    case APP_WL_SMARTVOICE_BLE_TX_DONE:
        app_wl_smartvoice_tx_quota_release();
        if (app_wl_smartvoice_channel_ctx->data_tx_done_handler) {
            app_wl_smartvoice_channel_ctx->data_tx_done_handler();
        }
        break;
    
    default:
        break;
    }

    WL_SV_FUNC_EXIT();
}

POSSIBLY_UNUSED static void app_wl_smartvoice_spp_event_handler(int event, void* info)
{
    switch (event)
    {

    case APP_WL_SMARTVOICE_SPP_CONNECTED_EVENT:
        app_wl_smartvoice_channel_ctx->b_spp_connected = true;
        app_wl_smartvoice_tx_quota_reset();
        if (APP_WL_SMARTVOICE_CHANNEL_SPP == app_wl_smartvoice_current_channel_get()) {
            app_wl_smartvoice_channel_event_post(APP_WL_SMARTVOICE_CHANNEL_EVENT_CONNECTED);
        }
        break;

    case APP_WL_SMARTVOICE_SPP_DISCONNECTED_EVENT:
        if (APP_WL_SMARTVOICE_CHANNEL_SPP == app_wl_smartvoice_current_channel_get()) {
            app_wl_smartvoice_channel_event_post(APP_WL_SMARTVOICE_CHANNEL_EVENT_DISCONNECTED);
        }
        app_wl_smartvoice_channel_ctx->b_spp_connected = false;
        break;

    case APP_WL_SMARTVOICE_SPP_TX_DONE:
        app_wl_smartvoice_tx_quota_release();
        if (app_wl_smartvoice_channel_ctx->data_tx_done_handler) {
            app_wl_smartvoice_channel_ctx->data_tx_done_handler();
        }
        break;

    default:
        break;
    }
}

static void app_wl_smartvoice_data_received(uint8_t* data, uint16_t length)
{
    WL_SV_FUNC_ENTER();

    WL_SV_DEBUG("data received:");
    DUMP8("%02x ", data, length);

    if (app_wl_smartvoice_channel_ctx->data_ind_handler) {
        app_wl_smartvoice_channel_ctx->data_ind_handler(data, length, APP_WL_SMARTVOICE_CHANNEL_BLE);
    }

    WL_SV_FUNC_EXIT();
}

static void app_wl_smartvoice_spp_data_received(uint8_t* data, uint16_t length)
{
    WL_SV_FUNC_ENTER();

    WL_SV_DEBUG("data received:");
    DUMP8("%02x ", data, length);

    if (app_wl_smartvoice_channel_ctx->data_ind_handler) {
        app_wl_smartvoice_channel_ctx->data_ind_handler(data, length, APP_WL_SMARTVOICE_CHANNEL_SPP);
    }

    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_send_key_status(uint8_t* data, uint16_t length)
{
    WL_SV_FUNC_ENTER();

    if (app_wl_smartvoice_tx_quota_available()) {
        WL_SV_DEBUG("tx_quota: %d", app_wl_smartvoice_tx_quota_num_get());
        if (app_wl_smartvoice_channel_ctx->b_spp_connected) {
            app_wl_smartvoice_tx_quota_acquire();
            app_wl_smartvoice_spp_key_code_send(data, length);
        } else if (app_wl_smartvoice_channel_ctx->b_ble_connected) {
            app_wl_smartvoice_tx_quota_acquire();
            app_wl_smartvoice_ble_key_code_send(data, length);
        }
    } else {
        WL_SV_DEBUG("tx quota NOT Available");
    }



    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_send_control_data(uint8_t* data, uint16_t length)
{
    WL_SV_FUNC_ENTER();

    if (app_wl_smartvoice_tx_quota_available()) {
        WL_SV_DEBUG("tx_quota: %d", app_wl_smartvoice_tx_quota_num_get());
        if (app_wl_smartvoice_channel_ctx->b_spp_connected) {
            app_wl_smartvoice_tx_quota_acquire();
            app_wl_smartvoice_spp_command_send(data, length);
        } else if (app_wl_smartvoice_channel_ctx->b_ble_connected) {
            app_wl_smartvoice_tx_quota_acquire();
            app_wl_smartvoice_ble_command_send(data, length); 
        }
    } else {
        WL_SV_DEBUG("tx quota NOT Available");
    }

    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_send_voice_data(uint8_t* data, uint16_t length)
{
    static uint8_t buff[200];
    uint16_t out_len;

    app_wl_sv_build_voice_stream(data, length, buff, &out_len);

    if (app_wl_smartvoice_tx_quota_available()) {
        WL_SV_DEBUG("tx_quota: %d", app_wl_smartvoice_tx_quota_num_get());
        if (app_wl_smartvoice_channel_ctx->b_spp_connected) {
            app_wl_smartvoice_tx_quota_acquire();
            app_wl_smartvoice_spp_stream_send(buff, out_len);
        } else if (app_wl_smartvoice_channel_ctx->b_ble_connected) {
            app_wl_smartvoice_tx_quota_acquire();
            app_wl_smartvoice_ble_stream_send(buff, out_len);
        }
    } else {
        WL_SV_DEBUG("tx quota NOT Available");
    }
}

uint8_t app_wl_smartvoice_current_channel_get(void)
{
    if (app_wl_smartvoice_channel_ctx->b_spp_connected) {
        return APP_WL_SMARTVOICE_CHANNEL_SPP;
    }

    if (app_wl_smartvoice_channel_ctx->b_ble_connected) {
        return APP_WL_SMARTVOICE_CHANNEL_BLE;
    }

    return APP_WL_SMARTVOICE_CHANNEL_INVALID;
}

void app_wl_smartvoice_channel_data_subscribe(CHANNEL_DATA_IND_CB cb)
{
    app_wl_smartvoice_channel_ctx->data_ind_handler = cb;
}

void app_wl_smartvoice_channel_data_tx_done_subscribe(CHANNEL_DATA_DONE_CB cb)
{
    app_wl_smartvoice_channel_ctx->data_tx_done_handler = cb;
}

void app_wl_smartvoice_channel_event_subscribe(CHANNEL_EVENT_IND_CB cb)
{
    app_wl_smartvoice_channel_ctx->event_handler = cb;
}

void app_wl_smartvoice_channle_init(void)
{
    WL_SV_FUNC_ENTER();

    memset(app_wl_smartvoice_channel_ctx, 0x00, sizeof(app_wl_smartvoice_channel_ctx_t));

    app_wl_smartvoice_channel_ctx->tx_quota_mutex = osMutexCreate((osMutex(tx_quota_mutex)));
    app_wl_smartvoice_channel_ctx->tx_quota = APP_WL_SMARTVOICE_BLE_TX_QUOTA;

    app_wl_smartvoice_ble_event_subscribe(app_wl_smartvoice_ble_event_handler);

    app_wl_smartvoice_ble_cmd_received_subscribe(app_wl_smartvoice_data_received);

    app_wl_smartvoice_spp_event_subscribe(app_wl_smartvoice_spp_event_handler);

    app_wl_smartvoice_spp_cmd_received_subscribe(app_wl_smartvoice_spp_data_received);

    WL_SV_FUNC_EXIT();
}