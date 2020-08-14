#include "app_wl_smartvoice_ble.h"
#include "app_wl_smartvoice.h"

#include "app_ble_mode_switch.h"
#include "app_datapath_server.h"

#include "string.h"

static app_wl_smartvoice_ble_ctx_t _wl_smartvoice_ble_ctx;
static app_wl_smartvoice_ble_ctx_t* wl_smartvoice_ble_ctx = &_wl_smartvoice_ble_ctx;

static bool app_wl_smartvoice_is_ble_connected(app_wl_smartvoice_ble_ctx_t* ctx)
{
    return ctx->conidx != 0xFF;
}

static void app_wl_smartvoice_set_ble_connected(app_wl_smartvoice_ble_ctx_t* ctx, uint8_t conidx)
{
    ctx->conidx = conidx;
}

static void app_wl_smartvoice_set_ble_disconnected(app_wl_smartvoice_ble_ctx_t* ctx)
{
    ctx->conidx = 0xFF;
}

static bool app_wl_smartvoice_is_current_ble_link(app_wl_smartvoice_ble_ctx_t* ctx, uint8_t conidx)
{
    return ctx->conidx == conidx;
}

POSSIBLY_UNUSED uint8_t app_wl_smartvoice_ble_conidx_get(app_wl_smartvoice_ble_ctx_t* ctx)
{
    return ctx->conidx;
}

static void app_wl_smartvoice_ble_set_keycode_tx_enable(app_wl_smartvoice_ble_ctx_t* ctx, bool isEnable)
{
    ctx->b_keycode_tx_enable = isEnable;
}

static bool app_wl_smartvoice_ble_is_keycode_tx_enable(app_wl_smartvoice_ble_ctx_t* ctx)
{
    return ctx->b_keycode_tx_enable;
}

static void app_wl_smartvoice_ble_set_command_tx_enable(app_wl_smartvoice_ble_ctx_t* ctx, bool isEnable)
{
    ctx->b_command_tx_enable = isEnable;
}

static bool app_wl_smartvoice_ble_is_command_tx_enable(app_wl_smartvoice_ble_ctx_t* ctx)
{
    return ctx->b_command_tx_enable;
}

static void app_wl_smartvoice_ble_set_stream_tx_enable(app_wl_smartvoice_ble_ctx_t* ctx, bool isEnable)
{
    ctx->b_stream_tx_enable = isEnable;
}

static bool app_wl_smartvoice_ble_is_stream_tx_enable(app_wl_smartvoice_ble_ctx_t* ctx)
{
    return ctx->b_stream_tx_enable;
}

POSSIBLY_UNUSED static void app_wl_smartvoice_tx_done(void)
{
    if (wl_smartvoice_ble_ctx->event_handler) {
        wl_smartvoice_ble_ctx->event_handler(APP_WL_SMARTVOICE_BLE_TX_DONE, NULL);
    }
}

void app_wl_smartvoice_connected_evt_handler(uint8_t conidx)
{
    WL_SV_FUNC_ENTER();

    if (app_wl_smartvoice_is_ble_connected(wl_smartvoice_ble_ctx)) {
        goto exit;
    }

    app_wl_smartvoice_set_ble_connected(wl_smartvoice_ble_ctx, conidx);
    app_datapath_server_register_tx_done(app_wl_smartvoice_tx_done);
    if (wl_smartvoice_ble_ctx->event_handler) {
        wl_smartvoice_ble_ctx->event_handler(APP_WL_SMARTVOICE_BLE_CONNECTED_EVENT, NULL);
    }

exit:
    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_disconnect_evt_handler(uint8_t conidx, uint8_t reason)
{
    WL_SV_FUNC_ENTER();

    if (!app_wl_smartvoice_is_current_ble_link(wl_smartvoice_ble_ctx, conidx)) {
        goto exit;
    }

    app_wl_smartvoice_set_ble_disconnected(wl_smartvoice_ble_ctx);

    app_wl_smartvoice_ble_set_keycode_tx_enable(wl_smartvoice_ble_ctx, false);
    app_wl_smartvoice_ble_set_command_tx_enable(wl_smartvoice_ble_ctx, false);
    app_wl_smartvoice_ble_set_stream_tx_enable(wl_smartvoice_ble_ctx, false);

    if (wl_smartvoice_ble_ctx->event_handler) {
        wl_smartvoice_ble_ctx->event_handler(APP_WL_SMARTVOICE_BLE_DISCONNECTED_EVENT, (void*)&reason);
    }

exit:
    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_ble_keycode_tx_subscribe_ind(uint8_t conidx, bool isEnable)
{
    WL_SV_FUNC_ENTER();

    if (!app_wl_smartvoice_is_current_ble_link(wl_smartvoice_ble_ctx, conidx)) {
        goto exit;
    }

    app_wl_smartvoice_ble_set_keycode_tx_enable(wl_smartvoice_ble_ctx, isEnable);
    if (wl_smartvoice_ble_ctx->event_handler) {
        wl_smartvoice_ble_ctx->event_handler(APP_WL_SMARTVOICE_BLE_KEYCODE_TX_ENABLE_EVENT, NULL);
    }

exit:
    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_ble_command_tx_subscribe_ind(uint8_t conidx, bool isEnable)
{
    WL_SV_FUNC_ENTER();

    if (!app_wl_smartvoice_is_current_ble_link(wl_smartvoice_ble_ctx, conidx)) {
        goto exit;
    }

    app_wl_smartvoice_ble_set_command_tx_enable(wl_smartvoice_ble_ctx, isEnable);
    if (wl_smartvoice_ble_ctx->event_handler) {
        wl_smartvoice_ble_ctx->event_handler(APP_WL_SMARTVOICE_BLE_COMMAND_TX_ENABLE_EVENT, NULL);
    }

exit:
    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_ble_stream_tx_subscribe_ind(uint8_t conidx, bool isEnable)
{
    WL_SV_FUNC_ENTER();

    if (!app_wl_smartvoice_is_current_ble_link(wl_smartvoice_ble_ctx, conidx)) {
        goto exit;
    }

    app_wl_smartvoice_ble_set_stream_tx_enable(wl_smartvoice_ble_ctx, isEnable);
    if (wl_smartvoice_ble_ctx->event_handler) {
        wl_smartvoice_ble_ctx->event_handler(APP_WL_SMARTVOICE_BLE_STREAM_TX_ENABLE_EVENT, NULL);
    }

exit:
    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_ble_key_code_send(uint8_t* data, uint16_t length)
{
    WL_SV_FUNC_ENTER();

    WL_SV_DEBUG("keycode tx enable: %d", app_wl_smartvoice_ble_is_keycode_tx_enable(wl_smartvoice_ble_ctx));

    if (app_wl_smartvoice_ble_is_keycode_tx_enable(wl_smartvoice_ble_ctx)) {
#ifdef __IAG_BLE_INCLUDE__
        app_datapath_server_send_data_via_notification(data, length);
#endif
    }

    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_ble_command_send(uint8_t* data, uint16_t length)
{
    WL_SV_FUNC_ENTER();

    WL_SV_DEBUG("command tx enable: %d", app_wl_smartvoice_ble_is_command_tx_enable(wl_smartvoice_ble_ctx));
    DUMP8("%02x ", data, length);

    if (app_wl_smartvoice_ble_is_command_tx_enable(wl_smartvoice_ble_ctx)) {
#ifdef __IAG_BLE_INCLUDE__
        app_wl_smartvoice_send_command_via_notification(data, length);
#endif
    }

    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_ble_stream_send(uint8_t* data, uint16_t length)
{
    WL_SV_DEBUG("stream tx enable: %d", app_wl_smartvoice_ble_is_stream_tx_enable(wl_smartvoice_ble_ctx));

    if (app_wl_smartvoice_ble_is_stream_tx_enable(wl_smartvoice_ble_ctx)) {
#ifdef __IAG_BLE_INCLUDE__
        app_wl_smartvoice_send_stream_via_notification(data, length);
#endif
    }
}

extern "C" void app_ble_start_disconnect(uint8_t idx);
void app_wl_smartvoice_ble_disconnect(void)
{
    app_ble_start_disconnect(app_wl_smartvoice_ble_conidx_get(wl_smartvoice_ble_ctx));
}

void app_wl_smartvoice_ble_data_received(uint8_t* data, uint16_t length)
{
    WL_SV_FUNC_ENTER();

    if (wl_smartvoice_ble_ctx->data_handler) {
        wl_smartvoice_ble_ctx->data_handler(data, length);
    }

    WL_SV_FUNC_EXIT();
}

void app_wl_smartvoice_ble_event_subscribe(APP_WL_SMARTVOICE_EVENT_HANDLER callback)
{
    wl_smartvoice_ble_ctx->event_handler = callback;
}

void app_wl_smartvoice_ble_cmd_received_subscribe(APP_WL_SMARTVOICE_DATA_RECEIVED_HANDLER callback)
{
    wl_smartvoice_ble_ctx->data_handler = callback;
}

#if 0
static void wl_smartvoice_ble_set_adv_uuid(BLE_ADV_PARAM_T *advInfo)
{
    memcpy(&advInfo->advData[advInfo->advDataLen],
                   WL_SMARTVOICE_ADV_UUID,
                   WL_SMARTVOICE_ADV_UUID_LEN);

    advInfo->advDataLen += WL_SMARTVOICE_ADV_UUID_LEN;
}

static void wl_smartvoice_ble_set_adv_manu_data(BLE_ADV_PARAM_T *advInfo)
{

}
#endif

POSSIBLY_UNUSED static void wl_smartvoice_adv_data_fill(void *advParam)
{
    // BLE_ADV_PARAM_T *advInfo = ( BLE_ADV_PARAM_T * )advParam;
    WL_SV_FUNC_ENTER();

    // wl_smartvoice_ble_set_adv_uuid(advInfo);

    // wl_smartvoice_ble_set_adv_manu_data(advInfo);

    WL_SV_FUNC_EXIT();

#ifdef __IAG_BLE_INCLUDE__
    // app_ble_data_fill_enable(USER_WL_SMARTVOICE, true);
#endif
}

void app_wl_smartvoice_ble_init(void)
{
    WL_SV_FUNC_ENTER();

    memset(wl_smartvoice_ble_ctx, 0x00, sizeof(app_wl_smartvoice_ble_ctx_t));
    app_wl_smartvoice_set_ble_disconnected(wl_smartvoice_ble_ctx);

#ifdef __IAG_BLE_INCLUDE__
    app_datapath_server_register_tx_done(app_wl_smartvoice_tx_done);
    // app_ble_register_data_fill_handle(USER_WL_SMARTVOICE, ( BLE_DATA_FILL_FUNC_T )wl_smartvoice_adv_data_fill, false);
#endif

    WL_SV_FUNC_EXIT();
}