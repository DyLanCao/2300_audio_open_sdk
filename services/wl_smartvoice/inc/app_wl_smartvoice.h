#ifndef __APP_WL_SMARTVOICE_H
#define __APP_WL_SMARTVOICE_H

#include "hal_trace.h"
#include "app_utils.h"

#include "cmsis_os.h"
#include "hal_timer.h"

#define WL_SV_DEBUG(s,...)                TRACE("[WL_SV]" s, ##__VA_ARGS__)
#define WL_SV_FUNC_ENTER()                //TRACE("[WL_SV] %s Enter" , __func__)
#define WL_SV_FUNC_EXIT()                 //TRACE("[WL_SV] %s OUT" , __func__)


#define WL_SMARTVOICE_VENDOR_NAME   "vendor_whale"
#define WL_SMARTVOICE_VERSION       "version_1.0.0"

#define WL_SMARTVOICE_SESSION_NAME  "wl_session_1"
#define WL_SMARTVOICE_SPEAKER_NAME  "wl_speaker_1"

#ifdef __cplusplus
extern "C" {
#endif

#define osAccurateTimerDef(timer, function) \
static void timer##_handler(void const *param); \
static void function(void); \
osTimerDef (timer##_TIMER, timer##_handler); \
static osTimerId timer = NULL; \
static uint32_t timer##_now = 0; \
static uint32_t timer##_ms = 0; \
static void timer##_delay_function(uint32_t ms) \
{ \
    timer##_now = hal_sys_timer_get(); \
    timer##_ms = ms; \
    if (NULL == timer) {\
        timer = osTimerCreate(osTimer(timer##_TIMER), osTimerPeriodic, NULL); \
    }\
    osTimerStop(timer);\
    osTimerStart(timer, 100);\
\
} \
static void timer##_handler(void const *param) \
{ \
    uint32_t now; \
    now = hal_sys_timer_get(); \
    if (TICKS_TO_MS(now - timer##_now) >= timer##_ms) {\
        osTimerStop(timer);\
        function(); \
    } \
}

#define osAccurateTimerStart(timer, ms) \
timer##_delay_function(ms)

#define osAccurateTimerStop(timer) \
osTimerStop(timer)

typedef struct{
    bool b_suport_voice;
    uint8_t seqid[16];

    uint8_t* vendor;
    uint16_t vendor_len;
    uint8_t* version;
    uint16_t version_len;

    uint8_t bt_addr[6];

    uint8_t session_name[16];
    uint16_t session_name_len;
    uint8_t speaker_name[16];
    uint16_t speaker_name_len;

    uint8_t net_status;
    uint8_t ppt_session_status;
    /* data */
} app_wl_sv_ctx_t;

void app_wl_sv_init(void);

void app_wl_sv_key_pressed(void);

void app_wl_sv_switch_next_group(void);
void app_wl_sv_switch_pre_group(void);

void app_wl_smartvoice_connected_evt_handler(uint8_t conidx);
void app_wl_smartvoice_disconnect_evt_handler(uint8_t conidx, uint8_t reason);

void app_wl_smartvoice_spp_connected_evt_handler(void);
void app_wl_smartvoice_spp_disconnected_evt_handler(void);

void app_wl_smartvoice_ble_keycode_tx_subscribe_ind(uint8_t conidx, bool isEnable);
void app_wl_smartvoice_ble_command_tx_subscribe_ind(uint8_t conidx, bool isEnable);
void app_wl_smartvoice_ble_stream_tx_subscribe_ind(uint8_t conidx, bool isEnable);

void app_wl_smartvoice_ble_data_received(uint8_t* data, uint16_t length);

int app_wl_smartvoice_player(bool on, enum APP_SYSFREQ_FREQ_T freq);

#ifdef __cplusplus
}
#endif

#endif