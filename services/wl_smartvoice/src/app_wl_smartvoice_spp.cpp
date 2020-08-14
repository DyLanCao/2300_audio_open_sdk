#include "app_wl_smartvoice_spp.h"
#include "app_wl_smartvoice.h"
#include "app_spp.h"
#include "cmsis_os.h"

static APP_WL_SMARTVOICE_SPP_EVENT_HANDLER event_callback = NULL;
static APP_WL_SMARTVOICE_SPP_DATA_RECEIVED_HANDLER data_callback = NULL;


static void app_wl_smartvoice_spp_thread(const void *arg);
osThreadDef(app_wl_smartvoice_spp_thread, osPriorityAboveNormal, 1, 3072, "app_wl_smartvoice_spp_thread");
osThreadId app_wl_smartvoice_spp_thread_id = NULL;

static void app_wl_smartvoice_spp_thread(const void *arg)
{
    static uint8_t buff[600];
    uint16_t read_bytes;
    uint32_t status;
    WL_SV_FUNC_ENTER();
    WL_SV_DEBUG("### Start...... ###");
    while (1) {
        read_bytes = sizeof(buff);
        status = app_spp_read_data(buff, &read_bytes);
        WL_SV_DEBUG("status: %d, callback: 0x%08x", status, (uint32_t)data_callback);
        if (data_callback) {
            data_callback(buff, read_bytes);
        }
    }
    WL_SV_FUNC_EXIT();
}

static void app_wl_smartvoice_spp_create_thread(void)
{
    WL_SV_FUNC_ENTER();
    app_wl_smartvoice_spp_thread_id = osThreadCreate(osThread(app_wl_smartvoice_spp_thread), NULL);
    WL_SV_FUNC_EXIT();
}

static void app_wl_smartvoice_spp_delete_thread(void)
{
    WL_SV_FUNC_ENTER();
    if (app_wl_smartvoice_spp_thread_id) {
        osThreadTerminate(app_wl_smartvoice_spp_thread_id);
        app_wl_smartvoice_spp_thread_id = NULL;
    }
    WL_SV_FUNC_EXIT();
}


static void app_wl_smartvoice_spp_tx_done(void)
{
    if (event_callback) {
        event_callback(APP_WL_SMARTVOICE_SPP_TX_DONE, NULL);
    }
}

extern "C" void app_wl_smartvoice_spp_connected_evt_handler(void)
{
    app_spp_register_tx_done(app_wl_smartvoice_spp_tx_done);
    app_wl_smartvoice_spp_create_thread();
    if (event_callback) {
        event_callback(APP_WL_SMARTVOICE_SPP_CONNECTED_EVENT, NULL);
    }
}

extern "C" void app_wl_smartvoice_spp_disconnected_evt_handler(void)
{
    app_spp_register_tx_done(NULL);
    app_wl_smartvoice_spp_delete_thread();
    if (event_callback) {
        event_callback(APP_WL_SMARTVOICE_SPP_DISCONNECTED_EVENT, NULL);
    }
}

void app_wl_smartvoice_spp_key_code_send(uint8_t* data, uint16_t length)
{
    app_sv_send_data_via_spp(data, length);
}

void app_wl_smartvoice_spp_command_send(uint8_t* data, uint16_t length)
{
    app_sv_send_data_via_spp(data, length);
}

void app_wl_smartvoice_spp_stream_send(uint8_t* data, uint16_t length)
{
    app_sv_send_data_via_spp(data, length);
}

void app_wl_smartvoice_spp_event_subscribe(APP_WL_SMARTVOICE_SPP_EVENT_HANDLER cb)
{
    event_callback = cb;
}

void app_wl_smartvoice_spp_cmd_received_subscribe(APP_WL_SMARTVOICE_SPP_DATA_RECEIVED_HANDLER cb)
{
    data_callback = cb;
}