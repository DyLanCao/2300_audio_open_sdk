/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "stdio.h"
#include "cmsis_os.h"
#include "list.h"
#include "string.h"

#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_bootmode.h"

#include "audioflinger.h"
#include "apps.h"
#include "app_thread.h"
#include "app_key.h"
#ifdef __PC_CMD_UART__
#include "app_cmd.h"
#endif
#include "app_pwl.h"
#include "app_audio.h"
#include "app_overlay.h"
#include "app_battery.h"
#include "app_utils.h"
#include "app_status_ind.h"
#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory.h"
#include "app_factory_bt.h"
#endif
#include "bt_drv_interface.h"
#include "besbt.h"
#include "norflash_api.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"
#include "crash_dump_section.h"
#include "log_section.h"
#include "factory_section.h"

#include "a2dp_api.h"
#include "me_api.h"
#include "os_api.h"
#include "btapp.h"
#include "app_bt.h"
#ifdef __INTERCONNECTION__
#include "app_interconnection.h"
#include "app_interconnection_logic_protocol.h"
#include "app_ble_mode_switch.h"
#endif

#ifdef GSOUND_OTA_ENABLED
#include "gsound_ota.h"
#endif

#ifdef BES_OTA_BASIC
#include "ota_bes.h"
#endif
#include "pmu.h"
#ifdef MEDIA_PLAYER_SUPPORT
#include "resources.h"
#include "app_media_player.h"
#endif
#include "app_bt_media_manager.h"
#include "hal_sleep.h"
#ifdef VOICE_DATAPATH
#include "app_voicepath.h"
#endif
#ifdef BT_USB_AUDIO_DUAL_MODE
#include "btusb_audio.h"
#include "usbaudio_thread.h"
#endif

#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_ibrt_customif_ui.h"
#include "app_ibrt_ui_test.h"
#endif

#ifdef  __APP_WL_SMARTVOICE__
#include "app_wl_smartvoice.h"
#endif

#ifdef AUDIO_DEBUG_V0_1_0
extern "C" int speech_tuning_init(void);
#endif

#if defined(BTUSB_AUDIO_MODE)
extern "C"  bool app_usbaudio_mode_on(void) ;
#endif

#ifdef BES_OTA_BASIC
extern "C" void ota_flash_init(void);
#endif

#ifdef _GFPS_
extern "C" void app_enter_fastpairing_mode(void);
#endif
#ifdef BTIF_BLE_APP_DATAPATH_SERVER
#include "app_ble_cmd_handler.h"
#endif
#ifdef ANC_APP
#include "app_anc.h"
#endif

#ifdef __AI_VOICE__
#include "ai_manager.h"
#include "ai_thread.h"
#include "ai_control.h"
#include "ai_spp.h"
#include "app_ai_voice.h"
#endif

#include "gapm_task.h"
#include "app_ble_mode_switch.h"

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#ifdef WL_GPIO_SWITCH
#include "tgt_hardware.h"
#endif

#ifdef WL_LED_ZG_SWITCH
#include "tgt_hardware.h" 
#endif


#ifdef AUDIO_LOOPBACK
#include "app_factory_audio.h"


#ifdef AUDIO_LINEIN_SOURCE
extern int app_source_linein_loopback_test(bool on);
#endif

#endif



#ifdef MIC_32K_LOOPBACK
#include "app_mic_32k.h"
#endif

#ifdef MIC_UART
#include "app_mic_uart.h"
union SECURITY_VALUE_T sec_val;
#endif

#ifdef DUAL_MIC_NSX
#include "dual_mic_nsx.h"
#endif


#define APP_BATTERY_LEVEL_LOWPOWERTHRESHOLD (1)

#ifdef RB_CODEC
extern int rb_ctl_init();
extern bool rb_ctl_is_init_done(void);
extern void app_rbplay_audio_reset_pause_status(void);
#endif

#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
extern bool app_pwr_key_monitor_get_val(void);
static bool anc_single_mode_on = false;
extern "C" bool anc_single_mode_is_on(void)
{
    return anc_single_mode_on;
}
#endif

#ifdef __ANC_STICK_SWITCH_USE_GPIO__
extern bool app_anc_switch_get_val(void);
#endif

#ifdef _GFPS_
extern "C" void app_fast_pairing_timeout_timehandler(void);
#endif

extern "C" void gsound_dump_set_flag(uint8_t is_happend);

uint8_t  app_poweroff_flag = 0;

#ifndef APP_TEST_MODE
static uint8_t app_status_indication_init(void)
{
#ifndef WL_LED_ZG_SWITCH
    struct APP_PWL_CFG_T cfg;
    memset(&cfg, 0, sizeof(struct APP_PWL_CFG_T));
    app_pwl_open();
    app_pwl_setup(APP_PWL_ID_0, &cfg);
    app_pwl_setup(APP_PWL_ID_1, &cfg);   
#endif 
    return 0;
}
#endif

#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)

void PairingTransferToConnectable(void);

typedef void (*APP_10_SECOND_TIMER_CB_T)(void);

void app_pair_timerout(void);
void app_poweroff_timerout(void);
void CloseEarphone(void);

typedef struct
{
    uint8_t timer_id;
    uint8_t timer_en;
    uint8_t timer_count;
    uint8_t timer_period;
    APP_10_SECOND_TIMER_CB_T cb;
}APP_10_SECOND_TIMER_STRUCT;

#define INIT_APP_TIMER(_id, _en, _count, _period, _cb) \
    { \
        .timer_id = _id, \
        .timer_en = _en, \
        .timer_count = _count, \
        .timer_period = _period, \
        .cb = _cb, \
    }

APP_10_SECOND_TIMER_STRUCT app_10_second_array[] =
{
    INIT_APP_TIMER(APP_PAIR_TIMER_ID, 0, 0, 6, PairingTransferToConnectable),
    INIT_APP_TIMER(APP_POWEROFF_TIMER_ID, 0, 0, 90, CloseEarphone),
#ifdef _GFPS_
    INIT_APP_TIMER(APP_FASTPAIR_LASTING_TIMER_ID, 0, 0, APP_FAST_PAIRING_TIMEOUT_IN_SECOND/10,
        app_fast_pairing_timeout_timehandler),
#endif
};

void app_stop_10_second_timer(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = &app_10_second_array[timer_id];

    timer->timer_en = 0;
    timer->timer_count = 0;
}

void app_start_10_second_timer(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = &app_10_second_array[timer_id];

    timer->timer_en = 1;
    timer->timer_count = 0;
}

void app_set_10_second_timer(uint8_t timer_id, uint8_t enable, uint8_t period)
{
    APP_10_SECOND_TIMER_STRUCT *timer = &app_10_second_array[timer_id];

    timer->timer_en = enable;
    timer->timer_count = period;
}

void app_10_second_timer_check(void)
{
    APP_10_SECOND_TIMER_STRUCT *timer = app_10_second_array;
    unsigned int i;

    for(i = 0; i < ARRAY_SIZE(app_10_second_array); i++) {
        if (timer->timer_en) {
            timer->timer_count++;
            if (timer->timer_count >= timer->timer_period) {
                timer->timer_en = 0;
                if (timer->cb)
                    timer->cb();
            }
        }
        timer++;
    }
}

void CloseEarphone(void)
{
    int activeCons;
    osapi_lock_stack();
    activeCons = btif_me_get_activeCons();
    osapi_unlock_stack();

#ifdef ANC_APP
    if(app_anc_work_status()) {
        app_set_10_second_timer(APP_POWEROFF_TIMER_ID, 1, 30);
        return;
    }
#endif /* ANC_APP */
    if(activeCons == 0) {
        TRACE("!!!CloseEarphone\n");
        app_shutdown();
    }
}
#endif /* #if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__) */

//#if (HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_BATTERY_REPORT) || (HF_SDK_FEATURES & HF_FEATURE_HF_INDICATORS)
#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
extern void app_hfp_set_battery_level(uint8_t level);
#endif

int app_status_battery_report(uint8_t level)
{
#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
    if( anc_single_mode_on )//anc power on,anc only mode
        return 0;
#endif
#if defined(__BTIF_EARPHONE__)
    app_bt_state_checker();
    app_10_second_timer_check();
#endif
    if (level<=APP_BATTERY_LEVEL_LOWPOWERTHRESHOLD){
        //add something
    }
//#if (HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_BATTERY_REPORT) || (HF_SDK_FEATURES & HF_FEATURE_HF_INDICATORS)
#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
#if defined(IBRT)
    if (app_tws_ibrt_mobile_link_connected())
    {
        app_hfp_set_battery_level(level);
    }
#else
    app_hfp_set_battery_level(level);
#endif
#else
    TRACE("[%s] Can not enable SUPPORT_BATTERY_REPORT",__func__);
#endif
    osapi_notify_evm();
    return 0;
}

#ifdef MEDIA_PLAYER_SUPPORT

void app_status_set_num(const char* p)
{
    media_Set_IncomingNumber(p);
}

extern "C" int app_voice_report(APP_STATUS_INDICATION_T status,uint8_t device_id)
{
#if (defined(BTUSB_AUDIO_MODE) || defined(BTUSB_AUDIO_MODE))
    if(app_usbaudio_mode_on()) return 0;
#endif
    TRACE("%s %d",__func__, status);
    AUD_ID_ENUM id = MAX_RECORD_NUM;
#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
    if(anc_single_mode_on)
        return 0;
#endif
    if (app_poweroff_flag == 1){
        switch (status) {
            case APP_STATUS_INDICATION_POWEROFF:
                id = AUD_ID_POWER_OFF;
                break;
            default:
                return 0;
                break;
        }
    }else{
        switch (status) {
            case APP_STATUS_INDICATION_POWERON:
                id = AUD_ID_POWER_ON;
                break;
            case APP_STATUS_INDICATION_POWEROFF:
                id = AUD_ID_POWER_OFF;
                break;
            case APP_STATUS_INDICATION_CONNECTED:
                id = AUD_ID_BT_CONNECTED;
                break;
            case APP_STATUS_INDICATION_DISCONNECTED:
                id = AUD_ID_BT_DIS_CONNECT;
                break;
            case APP_STATUS_INDICATION_CALLNUMBER:
                id = AUD_ID_BT_CALL_INCOMING_NUMBER;
                break;
            case APP_STATUS_INDICATION_CHARGENEED:
                id = AUD_ID_BT_CHARGE_PLEASE;
                break;
            case APP_STATUS_INDICATION_FULLCHARGE:
                id = AUD_ID_BT_CHARGE_FINISH;
                break;
            case APP_STATUS_INDICATION_PAIRSUCCEED:
                id = AUD_ID_BT_PAIRING_SUC;
                break;
            case APP_STATUS_INDICATION_PAIRFAIL:
                id = AUD_ID_BT_PAIRING_FAIL;
                break;

            case APP_STATUS_INDICATION_HANGUPCALL:
                id = AUD_ID_BT_CALL_HUNG_UP;
                break;

            case APP_STATUS_INDICATION_REFUSECALL:
                id = AUD_ID_BT_CALL_REFUSE;
                break;

            case APP_STATUS_INDICATION_ANSWERCALL:
                id = AUD_ID_BT_CALL_ANSWER;
                break;

            case APP_STATUS_INDICATION_CLEARSUCCEED:
                id = AUD_ID_BT_CLEAR_SUCCESS;
                break;

            case APP_STATUS_INDICATION_CLEARFAIL:
                id = AUD_ID_BT_CLEAR_FAIL;
                break;
            case APP_STATUS_INDICATION_INCOMINGCALL:
                id = AUD_ID_BT_CALL_INCOMING_CALL;
                break;
            case APP_STATUS_INDICATION_BOTHSCAN:
                id = AUD_ID_BT_PAIR_ENABLE;
                break;
            case APP_STATUS_INDICATION_WARNING:
                id = AUD_ID_BT_WARNING;
                break;
            case APP_STATUS_INDICATION_ALEXA_START:
                id = AUDIO_ID_BT_ALEXA_START;
                break;
            case APP_STATUS_INDICATION_ALEXA_STOP:
                id = AUDIO_ID_BT_ALEXA_STOP;
                break;
            case APP_STATUS_INDICATION_GSOUND_MIC_OPEN:
                id = AUDIO_ID_BT_GSOUND_MIC_OPEN;
                break;
            case APP_STATUS_INDICATION_GSOUND_MIC_CLOSE:
                id = AUDIO_ID_BT_GSOUND_MIC_CLOSE;
                break;
            case APP_STATUS_INDICATION_GSOUND_NC:
                id = AUDIO_ID_BT_GSOUND_NC;
                break;
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__
			case APP_STATUS_RING_WARNING:
				id = AUD_ID_RING_WARNING;
				break;
#endif
            case APP_STATUS_INDICATION_MUTE:
                id = AUDIO_ID_BT_MUTE;
                break;
            default:
                break;
        }
    }
#ifdef BT_USB_AUDIO_DUAL_MODE
    if(!btusb_is_usb_mode())
    {
#if defined(IBRT)
        app_ibrt_if_voice_report(id);
#else
        media_PlayAudio(id, device_id);
#endif
    }

#else
#if defined(IBRT)
    app_ibrt_if_voice_report(id);
#else
    media_PlayAudio(id, device_id);
#endif
#endif

    return 0;
}
#endif


#define POWERON_PRESSMAXTIME_THRESHOLD_MS  (5000)

enum APP_POWERON_CASE_T {
    APP_POWERON_CASE_NORMAL = 0,
    APP_POWERON_CASE_DITHERING,
    APP_POWERON_CASE_REBOOT,
    APP_POWERON_CASE_ALARM,
    APP_POWERON_CASE_CALIB,
    APP_POWERON_CASE_BOTHSCAN,
    APP_POWERON_CASE_CHARGING,
    APP_POWERON_CASE_FACTORY,
    APP_POWERON_CASE_TEST,
    APP_POWERON_CASE_INVALID,

    APP_POWERON_CASE_NUM
};


static osThreadId apps_init_tid = NULL;

static enum APP_POWERON_CASE_T pwron_case = APP_POWERON_CASE_INVALID;

static void app_poweron_normal(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    pwron_case = APP_POWERON_CASE_NORMAL;

    osSignalSet(apps_init_tid, 0x2);
}

static void app_poweron_scan(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    pwron_case = APP_POWERON_CASE_BOTHSCAN;

    osSignalSet(apps_init_tid, 0x2);
}

#ifdef __ENGINEER_MODE_SUPPORT__
static void app_poweron_factorymode(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    app_factorymode_enter();
}
#endif

static void app_poweron_finished(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    osSignalSet(apps_init_tid, 0x2);
}

void app_poweron_wait_finished(void)
{
    osSignalWait(0x2, osWaitForever);
}

#if  defined(__POWERKEY_CTRL_ONOFF_ONLY__)
void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param);
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},           "power on: shutdown"     , app_bt_key_shutdown, NULL},
};
#elif defined(__ENGINEER_MODE_SUPPORT__)
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITUP},           "power on: normal"     , app_poweron_normal, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGPRESS},    "power on: both scan"  , app_poweron_scan  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGLONGPRESS},"power on: factory mode", app_poweron_factorymode  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITFINISHED},     "power on: finished"   , app_poweron_finished  , NULL},
};
#else
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITUP},           "power on: normal"     , app_poweron_normal, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGPRESS},    "power on: both scan"  , app_poweron_scan  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITFINISHED},     "power on: finished"   , app_poweron_finished  , NULL},
};
#endif

#ifndef APP_TEST_MODE
static void app_poweron_key_init(void)
{
    uint8_t i = 0;
    TRACE("%s",__func__);

    for (i=0; i<(sizeof(pwron_key_handle_cfg)/sizeof(APP_KEY_HANDLE)); i++){
        app_key_handle_registration(&pwron_key_handle_cfg[i]);
    }
}

static uint8_t app_poweron_wait_case(void)
{
    uint32_t stime, etime;

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
    pwron_case = APP_POWERON_CASE_NORMAL;
#else
    pwron_case = APP_POWERON_CASE_INVALID;

    stime = hal_sys_timer_get();
    osSignalWait(0x2, POWERON_PRESSMAXTIME_THRESHOLD_MS);
    etime = hal_sys_timer_get();
    TRACE("powon raw case:%d time:%d", pwron_case, TICKS_TO_MS(etime - stime));
#endif
    return pwron_case;

}
#endif

extern "C" void app_notify_stack_ready()
{
    TRACE("app_notify_stack_ready");
    osSignalSet(apps_init_tid, 0x3);
}
static void app_wait_stack_ready(void)
{
    uint32_t stime, etime;
    stime = hal_sys_timer_get();
    osSignalWait(0x3, 1000);
    etime = hal_sys_timer_get();
    TRACE("app_wait_stack_ready: wait:%d ms", TICKS_TO_MS(etime - stime));
}

int system_shutdown(void);
int app_shutdown(void)
{
    system_shutdown();
    return 0;
}

int system_reset(void);
int app_reset(void)
{
    system_reset();
    return 0;
}

static void app_postponed_reset_timer_handler(void const *param);
osTimerDef(APP_POSTPONED_RESET_TIMER, app_postponed_reset_timer_handler);
static osTimerId app_postponed_reset_timer = NULL;
#define APP_RESET_PONTPONED_TIME_IN_MS  2000
static void app_postponed_reset_timer_handler(void const *param)
{
    system_reset();
}

void app_start_postponed_reset(void)
{
    if (NULL == app_postponed_reset_timer)
    {
        app_postponed_reset_timer = osTimerCreate(osTimer(APP_POSTPONED_RESET_TIMER), osTimerOnce, NULL);
    }

    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);

    osTimerStart(app_postponed_reset_timer, APP_RESET_PONTPONED_TIME_IN_MS);
}

void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    app_reset();
#else
    app_shutdown();
#endif
}

void app_bt_key_enter_testmode(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s\n",__FUNCTION__);

    if(app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN){
#ifdef __FACTORY_MODE_SUPPORT__
        app_factorymode_bt_signalingtest(status, param);
#endif
    }
}

void app_bt_key_enter_nosignal_mode(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s\n",__FUNCTION__);
    if(app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN){
#ifdef __FACTORY_MODE_SUPPORT__
        app_factorymode_bt_nosignalingtest(status, param);
#endif
    }
}

void app_bt_singleclick(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
}

void app_bt_doubleclick(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
}

void app_power_off(APP_KEY_STATUS *status, void *param)
{
    TRACE("app_power_off\n");
}

extern "C" void OS_NotifyEvm(void);

#define PRESS_KEY_TO_ENTER_OTA_INTERVEL    (15000)          // press key 15s enter to ota
#define PRESS_KEY_TO_ENTER_OTA_REPEAT_CNT    ((PRESS_KEY_TO_ENTER_OTA_INTERVEL - 2000) / 500)
void app_otaMode_enter(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s",__func__);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
#ifdef __KMATE106__
    app_status_indication_set(APP_STATUS_INDICATION_OTA);
    app_voice_report(APP_STATUS_INDICATION_WARNING, 0);
    osDelay(1200);
#endif
    hal_cmu_sys_reboot();
}

#ifdef __USB_COMM__
void app_usb_cdc_comm_key_handler(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d", __func__, status->code, status->event);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_CDC_COMM);
    pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
    hal_cmu_reset_set(HAL_CMU_MOD_GLOBAL);
}
#endif

#if 0
void app_dfu_key_handler(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s ",__func__);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_FORCE_USB_DLD);
    pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
    hal_cmu_reset_set(HAL_CMU_MOD_GLOBAL);
}
#else
void app_dfu_key_handler(APP_KEY_STATUS *status, void *param)
{
	TRACE("%s ",__func__);
	hal_sw_bootmode_clear(0xffffffff);
	hal_sw_bootmode_set(HAL_SW_BOOTMODE_FORCE_USB_DLD | HAL_SW_BOOTMODE_SKIP_FLASH_BOOT);
	pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
	hal_cmu_reset_set(HAL_CMU_MOD_GLOBAL);
}
#endif

void app_ota_key_handler(APP_KEY_STATUS *status, void *param)
{
    static uint32_t time = hal_sys_timer_get();
    static uint16_t cnt = 0;

    TRACE("%s %d,%d",__func__, status->code, status->event);

    if (TICKS_TO_MS(hal_sys_timer_get() - time) > 600) // 600 = (repeat key intervel)500 + (margin)100
        cnt = 0;
    else
        cnt++;

    if (cnt == PRESS_KEY_TO_ENTER_OTA_REPEAT_CNT) {
        app_otaMode_enter(NULL, NULL);
    }

    time = hal_sys_timer_get();
}
extern "C" void app_bt_key(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
#define DEBUG_CODE_USE 0
    switch(status->event)
    {
        case APP_KEY_EVENT_CLICK:
            TRACE("first blood!");
#if DEBUG_CODE_USE
            if (status->code == APP_KEY_CODE_PWR)
            {
#ifdef __INTERCONNECTION__
                // add favorite music
                // app_interconnection_handle_favorite_music_through_ccmp(1);

                // ask for ota update
                ota_update_request();
                return;
#else
                static int m = 0;
                if (m == 0) {
                    m = 1;
                    hal_iomux_set_analog_i2c();
                }
                else {
                    m = 0;
                    hal_iomux_set_uart0();
                }
#endif
            }
#endif
            break;
        case APP_KEY_EVENT_DOUBLECLICK:
            TRACE("double kill");
#if DEBUG_CODE_USE
            if (status->code == APP_KEY_CODE_PWR)
            {
#ifdef __INTERCONNECTION__
                // play favorite music
                app_interconnection_handle_favorite_music_through_ccmp(2);
#else
                app_otaMode_enter(NULL, NULL);
#endif
                return;
            }
            break;
        case APP_KEY_EVENT_TRIPLECLICK:
            TRACE("triple kill");
            if (status->code == APP_KEY_CODE_PWR)
            {
                app_bt_accessmode_set(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#ifdef __INTERCONNECTION__
                app_interceonnection_start_discoverable_adv(BLE_FAST_ADVERTISING_INTERVAL, APP_INTERCONNECTION_FAST_ADV_TIMEOUT_IN_MS);
                return;
#endif
#ifdef _GFPS_
                app_enter_fastpairing_mode();
#endif
                return;
            }
#endif
            break;
        case APP_KEY_EVENT_ULTRACLICK:
            TRACE("ultra kill");
            break;
        case APP_KEY_EVENT_RAMPAGECLICK:
            TRACE("rampage kill!you are crazy!");
            break;

        case APP_KEY_EVENT_UP:
            break;
    }
#ifdef __FACTORY_MODE_SUPPORT__
    if (app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN && (status->event == APP_KEY_EVENT_DOUBLECLICK)){
        app_factorymode_languageswitch_proc();
    }else
#endif
    {
#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
        if(!anc_single_mode_on)
#endif
        bt_key_send(status);
    }
}

#ifdef RB_CODEC
extern bool app_rbcodec_check_hfp_active(void );
void app_switch_player_key(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);

    if(!rb_ctl_is_init_done()) {
        TRACE("rb ctl not init done");
        return ;
    }

    if( app_rbcodec_check_hfp_active() ) {
        app_bt_key(status,param);
        return;
    }

    app_rbplay_audio_reset_pause_status();

    if(app_rbplay_mode_switch()) {
        app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
        app_rbcodec_ctr_play_onoff(true);
    } else {
        app_rbcodec_ctr_play_onoff(false);
        app_voice_report(APP_STATUS_INDICATION_POWEROFF, 0);
    }
    return ;

}
#endif

#ifdef IS_MULTI_AI_ENABLED
#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
extern "C" bool gva_status_update_start;
extern "C" bool ama_status_update_start;
#endif
#endif
void app_voice_assistant_key(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s event %d", __func__, status->event);
#ifdef IS_MULTI_AI_ENABLED
    if (ai_manager_spec_get_status_is_in_invalid()) {
        TRACE("AI feature has been diabled");
        return;
    }

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    if (gva_status_update_start||ama_status_update_start) {
        TRACE("device reboot is ongoing...");
        return;
    }
#endif

    if(ai_voicekey_is_enable()) {
        if (AI_SPEC_GSOUND == ai_manager_get_current_spec()) {
#ifdef VOICE_DATAPATH
            app_voicepath_key(status, param);
#endif
        } else if(AI_SPEC_INIT != ai_manager_get_current_spec()) {
            if (APP_KEY_EVENT_CLICK == status->event)
                ai_function_handle(CALLBACK_WAKE_UP, status, 0);
        }
    }
#else
#if defined(__AI_VOICE__)
    if(ai_struct.wake_up_type == TYPE__PRESS_AND_HOLD) {
        if (APP_KEY_EVENT_FIRST_DOWN == status->event) {
            TRACE("%s wake up AI", __func__);
            ai_function_handle(CALLBACK_WAKE_UP, status, 0);
        } else if (APP_KEY_EVENT_UP == status->event) {
            TRACE("%s stop speech", __func__);
#ifdef __AMA_VOICE__
            ai_function_handle(CALLBACK_ENDPOINT_SPEECH, status, 0);
#else
            ai_function_handle(CALLBACK_STOP_SPEECH, status, 0);
#endif
        }
    } else {
        if (APP_KEY_EVENT_CLICK == status->event) {
            ai_function_handle(CALLBACK_WAKE_UP, status, 0);
        }
    }
#elif defined(VOICE_DATAPATH)
    app_voicepath_key(status, param);
#endif
#endif
}

#ifdef IS_MULTI_AI_ENABLED
void app_voice_gva_onoff_key(APP_KEY_STATUS *status, void *param)
{
    uint8_t current_ai_spec = ai_manager_get_current_spec();

    TRACE("%s current_ai_spec %d", __func__, current_ai_spec);
    if (current_ai_spec == AI_SPEC_INIT)
    {
        ai_manager_enable(true, AI_SPEC_GSOUND);
    }
    else if(current_ai_spec == AI_SPEC_GSOUND)
    {
        ai_manager_enable(false, AI_SPEC_GSOUND);
    }
    else if(current_ai_spec == AI_SPEC_AMA)
    {
        ai_manager_switch_spec(AI_SPEC_GSOUND);
    }
    app_start_ble_advertising(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL);
}

void app_voice_ama_onoff_key(APP_KEY_STATUS *status, void *param)
{
    uint8_t current_ai_spec = ai_manager_get_current_spec();

    TRACE("%s current_ai_spec %d", __func__, current_ai_spec);
    if (current_ai_spec == AI_SPEC_INIT)
    {
        ai_manager_enable(true, AI_SPEC_AMA);
    }
    else if(current_ai_spec == AI_SPEC_AMA)
    {
        ai_manager_enable(false, AI_SPEC_AMA);
    }
    else if(current_ai_spec == AI_SPEC_GSOUND)
    {
        ai_manager_switch_spec(AI_SPEC_AMA);
    }
    app_start_ble_advertising(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL);
}
#endif

#if defined(BT_USB_AUDIO_DUAL_MODE_TEST) && defined(BT_USB_AUDIO_DUAL_MODE)
extern "C" void test_btusb_switch(void);
void app_btusb_audio_dual_mode_test(APP_KEY_STATUS *status, void *param)
{
    TRACE("test_btusb_switch");
    test_btusb_switch();
}
#endif

extern void switch_dualmic_status(void);

void app_switch_dualmic_key(APP_KEY_STATUS *status, void *param)
{
    switch_dualmic_status();
}

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
#if defined(__APP_KEY_FN_STYLE_A__)
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_LONGPRESS},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_LONGPRESS},"bt play forward key",app_bt_key, NULL},
#ifdef SUPPORT_SIRI
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif

};
#else //#elif defined(__APP_KEY_FN_STYLE_B__)
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_REPEAT},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_REPEAT},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_UP},"bt play forward key",app_bt_key, NULL},
#ifdef SUPPORT_SIRI
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif

};
#endif
#else
#if defined(__APP_KEY_FN_STYLE_A__)
//--
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGPRESS},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
#if defined(BT_USB_AUDIO_DUAL_MODE_TEST) && defined(BT_USB_AUDIO_DUAL_MODE)
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_bt_key, NULL},
#ifdef RB_CODEC
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_switch_player_key, NULL},
#else
    //{{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"btusb mode switch key.",app_btusb_audio_dual_mode_test, NULL},
#endif
#endif
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_TRIPLECLICK},"bt function key",app_bt_key, NULL},
#if RAMPAGECLICK_TEST_MODE
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_ULTRACLICK},"bt function key",app_bt_key_enter_nosignal_mode, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_RAMPAGECLICK},"bt function key",app_bt_key_enter_testmode, NULL},
#endif
    //{{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt volume up key",app_bt_key, NULL},
    //{{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt play backward key",app_bt_key, NULL},
#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"bt mode src snk key",app_bt_key, NULL},
#endif
    //{{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key, NULL},
    //{{APP_KEY_CODE_FN2,APP_KEY_EVENT_LONGPRESS},"bt play forward key",app_bt_key, NULL},
    //{{APP_KEY_CODE_FN15,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_bt_key, NULL},

#ifdef SUPPORT_SIRI
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif
#ifdef __BT_ANC_KEY__
     {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt anc key",app_anc_key, NULL},
#endif
#if defined(VOICE_DATAPATH) || defined(__AI_VOICE__)
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_FIRST_DOWN}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP_AFTER_LONGPRESS}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_LONGPRESS}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_CLICK}, "google assistant key", app_voice_assistant_key, NULL},
    {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_DOUBLECLICK}, "google assistant key", app_voice_assistant_key, NULL},
#endif
#ifdef IS_MULTI_AI_ENABLED
    {{APP_KEY_CODE_FN13, APP_KEY_EVENT_CLICK}, "gva on-off key", app_voice_gva_onoff_key, NULL},
    {{APP_KEY_CODE_FN14, APP_KEY_EVENT_CLICK}, "ama on-off key", app_voice_ama_onoff_key, NULL},
#endif
#if defined(BT_USB_AUDIO_DUAL_MODE_TEST) && defined(BT_USB_AUDIO_DUAL_MODE)
    {{APP_KEY_CODE_FN15, APP_KEY_EVENT_CLICK}, "btusb mode switch key.", app_btusb_audio_dual_mode_test, NULL},
#endif
};
#else //#elif defined(__APP_KEY_FN_STYLE_B__)
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGPRESS},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_TRIPLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_ULTRACLICK},"bt function key",app_bt_key_enter_nosignal_mode, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_RAMPAGECLICK},"bt function key",app_bt_key_enter_testmode, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_REPEAT},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_REPEAT},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt play forward key",app_bt_key, NULL},
#ifdef SUPPORT_SIRI
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif
};
#endif
#endif

void app_key_init(void)
{
#if defined(IBRT)
    app_ibrt_ui_test_key_init();
#else
    uint8_t i = 0;
    TRACE("%s",__func__);

    app_key_handle_clear();
    for (i=0; i<(sizeof(app_key_handle_cfg)/sizeof(APP_KEY_HANDLE)); i++){
        app_key_handle_registration(&app_key_handle_cfg[i]);
    }
#endif
}
void app_key_init_on_charging(void)
{
    uint8_t i = 0;
    const APP_KEY_HANDLE  key_cfg[] = {
        {{APP_KEY_CODE_PWR,APP_KEY_EVENT_REPEAT},"ota function key",app_ota_key_handler, NULL},
        {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_dfu_key_handler, NULL},
#ifdef __USB_COMM__
        {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"usb cdc key",app_usb_cdc_comm_key_handler, NULL},
#endif
    };

    TRACE("%s",__func__);
    for (i=0; i<(sizeof(key_cfg)/sizeof(APP_KEY_HANDLE)); i++){
        app_key_handle_registration(&key_cfg[i]);
    }
}

extern bt_status_t LinkDisconnectDirectly(void);
void a2dp_suspend_music_force(void);

bool app_is_power_off_in_progress(void)
{
    return app_poweroff_flag?TRUE:FALSE;
}

int app_deinit(int deinit_case)
{
    int nRet = 0;
    TRACE("%s case:%d",__func__, deinit_case);

#ifdef __PC_CMD_UART__
    app_cmd_close();
#endif
#if defined(BTUSB_AUDIO_MODE)
    if(app_usbaudio_mode_on()) return 0;
#endif
    if (!deinit_case){
        app_poweroff_flag = 1;
#if defined(APP_LINEIN_A2DP_SOURCE)
        app_audio_sendrequest(APP_A2DP_SOURCE_LINEIN_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,0);
#endif
#if defined(APP_I2S_A2DP_SOURCE)
        app_audio_sendrequest(APP_A2DP_SOURCE_I2S_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,0);
#endif
        app_status_indication_filter_set(APP_STATUS_INDICATION_BOTHSCAN);
        app_audio_sendrequest(APP_BT_STREAM_INVALID, (uint8_t)APP_BT_SETTING_CLOSEALL, 0);
        osDelay(500);
        LinkDisconnectDirectly();
        osDelay(500);
        app_status_indication_set(APP_STATUS_INDICATION_POWEROFF);
#ifdef MEDIA_PLAYER_SUPPORT
        app_voice_report(APP_STATUS_INDICATION_POWEROFF, 0);
#endif
        osDelay(2000);
        af_close();
#if FPGA==0
        nv_record_flash_flush();
#endif
    }

    return nRet;
}

#ifdef APP_TEST_MODE
extern void app_test_init(void);
int app_init(void)
{
    int nRet = 0;
    //uint8_t pwron_case = APP_POWERON_CASE_INVALID;
    TRACE("%s",__func__);
    app_poweroff_flag = 0;

    apps_init_tid = osThreadGetId();
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);
    list_init();
    af_open();
    app_os_init();
    app_pwl_open();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();
    if (app_key_open(true))
    {
        nRet = -1;
        goto exit;
    }

    app_test_init();
exit:
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    return nRet;
}
#else /* !defined(APP_TEST_MODE) */

#define NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS (0xffffaa55)

int app_bt_connect2tester_init(void)
{
    btif_device_record_t rec;
    bt_bdaddr_t tester_addr;
    uint8_t i;
    bool find_tester = false;
    struct nvrecord_env_t *nvrecord_env;
    btdevice_profile *btdevice_plf_p;
    nv_record_env_get(&nvrecord_env);

    if (nvrecord_env->factory_tester_status.status != NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT)
        return 0;

    if (!nvrec_dev_get_dongleaddr(&tester_addr)){
        nv_record_open(section_usrdata_ddbrecord);
        for (i = 0; nv_record_enum_dev_records(i, &rec) == BT_STS_SUCCESS; i++) {
            if (!memcmp(rec.bdAddr.address, tester_addr.address, BTIF_BD_ADDR_SIZE)){
                find_tester = true;
            }
        }
        if(i==0 && !find_tester){
            memset(&rec, 0, sizeof(btif_device_record_t));
            memcpy(rec.bdAddr.address, tester_addr.address, BTIF_BD_ADDR_SIZE);
            nv_record_add(section_usrdata_ddbrecord, &rec);
            btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(rec.bdAddr.address);
            btdevice_plf_p->hfp_act = true;
            btdevice_plf_p->a2dp_act = true;
        }
        if (find_tester && i>2){
            nv_record_ddbrec_delete(&tester_addr);
            nvrecord_env->factory_tester_status.status = NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS;
            nv_record_env_set(nvrecord_env);
        }
    }

    return 0;
}

int app_nvrecord_rebuild(void)
{
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);

    nv_record_sector_clear();
    nv_record_env_init();
    nvrecord_env->factory_tester_status.status = NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS;
    nv_record_env_set(nvrecord_env);
    nv_record_flash_flush();

    return 0;
}

#if (defined(BTUSB_AUDIO_MODE) || defined(BTUSB_AUDIO_MODE))
#include "app_audio.h"
#include "usb_audio_frm_defs.h"
#include "usb_audio_app.h"

static bool app_usbaudio_mode = false;

extern "C" void btusbaudio_entry(void);
void app_usbaudio_entry(void)
{
    btusbaudio_entry();
    app_usbaudio_mode = true ;
}

bool app_usbaudio_mode_on(void)
{
    return app_usbaudio_mode;
}

void app_usb_key(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);

}

const APP_KEY_HANDLE  app_usb_handle_cfg[] = {
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"USB HID FN1 UP key",app_usb_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"USB HID FN2 UP key",app_usb_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},"USB HID PWR UP key",app_usb_key, NULL},
};

void app_usb_key_init(void)
{
    uint8_t i = 0;
    TRACE("%s",__func__);
    for (i=0; i<(sizeof(app_usb_handle_cfg)/sizeof(APP_KEY_HANDLE)); i++){
        app_key_handle_registration(&app_usb_handle_cfg[i]);
    }
}
#endif /* BTUSB_AUDIO_MODE */

//#define OS_HAS_CPU_STAT 1
#if OS_HAS_CPU_STAT
extern "C" void rtx_show_all_threads_usage(void);
#define CPU_USAGE_TIMER_TMO_VALUE (_CPU_STATISTICS_PEROID_/3)
static void cpu_usage_timer_handler(void const *param);
osTimerDef(cpu_usage_timer, cpu_usage_timer_handler);
static osTimerId cpu_usage_timer_id = NULL;
static void cpu_usage_timer_handler(void const *param)
{
    rtx_show_all_threads_usage();
}
#endif
extern void  btif_me_write_bt_sleep_enable(uint8_t sleep_en);
extern "C" int app_bt_start_custom_function_in_bt_thread(
    uint32_t param0, uint32_t param1, uint32_t funcPtr);
int btdrv_tportopen(void);


#ifdef WL_GPIO_SWITCH
void wl_gpio_init(void)
{
    if (app_wl_nsx_switch_detecter_cfg.pin != HAL_IOMUX_PIN_NUM)
    {
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_wl_nsx_switch_detecter_cfg, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_wl_nsx_switch_detecter_cfg.pin, HAL_GPIO_DIR_IN, 1);
    }

}
#endif

#ifdef WL_LED_ZG_SWITCH
void wl_gpio_zg_init(void)
{
    if (app_wl_zg_mute_switch_cfg.pin != HAL_IOMUX_PIN_NUM)
    {
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_wl_zg_mute_switch_cfg.pin, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_wl_zg_mute_switch_cfg.pin, HAL_GPIO_DIR_IN, 1);
        TRACE("wl_gpio_zg_init:%d ", hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)app_wl_zg_mute_switch_cfg.pin));
    }

}
#endif

int app_init(void)
{
    int nRet = 0;
#ifdef __AI_VOICE__
    uint8_t ai_spec = AI_SPEC_INIT;
#endif
    struct nvrecord_env_t *nvrecord_env;
    bool need_check_key = true;
    uint8_t pwron_case = APP_POWERON_CASE_INVALID;
#ifdef BT_USB_AUDIO_DUAL_MODE
    uint8_t usb_plugin = 0;
#endif

    TRACE("app_init\n");
    nv_record_init();
    factory_section_init();
#ifdef __WATCHER_DOG_RESET__
    app_wdt_open(15);
#endif
#ifdef ANC_APP
    app_anc_ios_init();
#endif
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
#if defined(MCU_HIGH_PERFORMANCE_MODE)
    TRACE("sys freq calc : %d\n", hal_sys_timer_calc_cpu_freq(5, 0));
#endif
    apps_init_tid = osThreadGetId();
    list_init();
    nRet = app_os_init();
    if (nRet) {
        goto exit;
    }
#if OS_HAS_CPU_STAT
        cpu_usage_timer_id = osTimerCreate(osTimer(cpu_usage_timer), osTimerPeriodic, NULL);
        if (cpu_usage_timer_id != NULL) {
            osTimerStart(cpu_usage_timer_id, CPU_USAGE_TIMER_TMO_VALUE);
        }
#endif

    app_status_indication_init();

#ifdef BTADDR_FOR_DEBUG
    gen_bt_addr_for_debug();
#endif

#ifdef FORCE_SIGNALINGMODE
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE | HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
#elif defined FORCE_NOSIGNALINGMODE
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE | HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE);
#endif

    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_REBOOT_FROM_CRASH){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT_FROM_CRASH);
        TRACE("Crash happened!!!");
    #ifdef VOICE_DATAPATH    
        gsound_dump_set_flag(true); 
    #endif
    }

    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_REBOOT){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
        pwron_case = APP_POWERON_CASE_REBOOT;
        need_check_key = false;
        TRACE("Initiative REBOOT happens!!!");
    }

    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_MODE){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MODE);
        pwron_case = APP_POWERON_CASE_TEST;
        need_check_key = false;
        TRACE("To enter test mode!!!");
    }


#ifdef BT_USB_AUDIO_DUAL_MODE
    usb_os_init();
#endif

#ifdef WL_GPIO_SWITCH
    wl_gpio_init();
#endif

#ifdef WL_LED_ZG_SWITCH
    wl_gpio_zg_init();
#endif

    nRet = app_battery_open();
    TRACE("BATTERY %d",nRet);
    if (nRet < 0){
        nRet = -1;
        goto exit;
    }else if (nRet > 0 &&
        pwron_case != APP_POWERON_CASE_TEST){
#ifdef BT_USB_AUDIO_DUAL_MODE
        TRACE("USB plusin.");
        usb_plugin = 1;
        nRet = 0;
#else  // BT_USB_AUDIO_DUAL_MODE
        pwron_case = APP_POWERON_CASE_CHARGING;
        app_status_indication_set(APP_STATUS_INDICATION_CHARGING);
        TRACE("CHARGING!");
        app_battery_start();
#if !defined(BTUSB_AUDIO_MODE)
        btdrv_start_bt();
        btdrv_hciopen();
        btdrv_sleep_config(1);
        btdrv_hcioff();
#endif
        app_key_open(false);
        app_key_init_on_charging();
        nRet = 0;
        goto exit;
#endif // BT_USB_AUDIO_DUAL_MODE
    }else{
        nRet = 0;
    }

    if (app_key_open(need_check_key)){
        nRet = -1;
        goto exit;
    }

    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT);
#if defined(_AUTO_TEST_)
    AUTO_TEST_SEND("Power on.");
#endif
    app_bt_init();
    af_open();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();


    nv_record_env_init();
    nvrec_dev_data_open();
    factory_section_open();
//    app_bt_connect2tester_init();
    nv_record_env_get(&nvrecord_env);

#ifdef IS_MULTI_AI_ENABLED
    init_ai_mode();
#endif

#ifdef __AI_VOICE__
#ifdef __AMA_VOICE__
        ai_spec = AI_SPEC_AMA;
#elif defined (__DMA_VOICE__)
        ai_spec = AI_SPEC_BAIDU;
#elif defined (__SMART_VOICE__)
        ai_spec = AI_SPEC_BES;
#elif defined (__TENCENT_VOICE__)
        ai_spec = AI_SPEC_TENCENT;
#endif

        if(ai_open(ai_spec)) {
            nRet = -1;
            goto exit;
        }

#ifdef KNOWLES_UART_DATA
        app_ai_voice_uart_audio_init();
#endif	
#endif

#ifdef __PC_CMD_UART__
    app_cmd_open();
#endif
#ifdef AUDIO_DEBUG_V0_1_0
    speech_tuning_init();
#endif
#ifdef ANC_APP
    app_anc_open_module();
#endif

#ifdef MEDIA_PLAYER_SUPPORT
    app_play_audio_set_lang(nvrecord_env->media_language.language);
#endif
    app_bt_stream_volume_ptr_update(NULL);

#ifdef __THIRDPARTY
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO2,THIRDPARTY_INIT);
#endif

#if defined(BES_OTA_BASIC)
    ota_flash_init();
#endif

#if defined(GSOUND_OTA_ENABLED)
    GSoundOtaHandlerInit();
#endif

#ifdef AUDIO_LOOPBACK

#ifdef WL_DEBUG_MODE
    TRACE("nv debug mode :0x%x",nv_debug_mode_get());
    if(nv_debug_mode_get() == NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT_DEBUG_MODE)
    {
        TRACE("****** end of test mode ****** ");
        osDelay(10);
        return -1;
    }
#endif 

#ifdef APP_LINEIN_SOURCE
    extern int app_source_linein_loopback_test(bool on);
    app_source_linein_loopback_test(true);
#else
   app_factorymode_audioloop(true, APP_SYSFREQ_208M);
#endif

    while(1);
#endif


#ifdef MIC_32K_LOOPBACK


#ifdef WL_DEBUG_MODE
    TRACE("nv debug mode :0x%x",nv_debug_mode_get());
    if(nv_debug_mode_get() == NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT_DEBUG_MODE)
    {
        TRACE("****** end of test mode ****** ");
        osDelay(10);
        return -1;
    }
#endif 

   app_mic_32k_audioloop(true, APP_SYSFREQ_208M);

    while(1);
#endif


#ifdef MIC_UART

#ifdef WL_DEBUG_MODE
    TRACE("nv debug mode :0x%x",nv_debug_mode_get());
    //nv_record_init_board_mode();

    TRACE("nv sec get:sec_val:0x%x vendor_id:0x%x ",sec_val.reg,sec_val.vendor_id);

    if(nv_init_flag_mode_get() != NVRAM_ENV_WL_BOARD_MODE)
    {
        TRACE("******you is not whale board, exit ****** ");
        osDelay(10);
        return -1;
    }

    if(nv_debug_mode_get() == NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT_DEBUG_MODE)
    {
        TRACE("****** end of test mode please contract whale tech group ****** ");
        osDelay(10);
        return -1;
    }
#endif 

    app_mic_uart_audioloop(true, APP_SYSFREQ_208M);

    while(1);
#endif


    btdrv_start_bt();
    if (pwron_case != APP_POWERON_CASE_TEST) {
        BesbtInit();
        app_wait_stack_ready();
        bt_drv_extra_config_after_init();
#if defined(IBRT)
        ibrt_config_t config;
        app_tws_ibrt_init();
        app_ibrt_ui_init();
        app_ibrt_if_config_load(&config);
        app_ibrt_customif_ui_start();
#ifdef IBRT_SEARCH_UI
        app_tws_ibrt_start(&config, true);
        app_ibrt_search_ui_init(false,IBRT_NONE_EVENT);
#else
        app_tws_ibrt_start(&config, false);   
#endif
        
#endif
    }
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);
    TRACE("\n\n\nbt_stack_init_done:%d\n\n\n", pwron_case);

#ifdef __APP_WL_SMARTVOICE__
    app_wl_sv_init();
#endif

    if (pwron_case == APP_POWERON_CASE_REBOOT){

        app_bt_start_custom_function_in_bt_thread((uint32_t)1,
                    0, (uint32_t)btif_me_write_bt_sleep_enable);
        btdrv_set_lpo_times();

#ifdef __AI_VOICE__
        app_ai_spp_server_init();
#endif

#if defined(BES_OTA_BASIC)
        bes_ota_init();
#endif
        app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);

        app_key_init();
        app_battery_start();
#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)
        app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif

#if defined(__IAG_BLE_INCLUDE__) && defined(BTIF_BLE_APP_DATAPATH_SERVER)
        BLE_custom_command_init();
#endif
#ifdef __THIRDPARTY
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_START);
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO2,THIRDPARTY_BT_CONNECTABLE);
#endif
#if defined( __BTIF_EARPHONE__) && defined(__BTIF_BT_RECONNECT__)
#if !defined(IBRT)
        app_bt_profile_connect_manager_opening_reconnect();
#endif
#endif
    }
#ifdef __ENGINEER_MODE_SUPPORT__
    else if(pwron_case == APP_POWERON_CASE_TEST){
        app_factorymode_set(true);
        app_status_indication_set(APP_STATUS_INDICATION_POWERON);
#ifdef MEDIA_PLAYER_SUPPORT
        app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
#endif
#ifdef __WATCHER_DOG_RESET__
        app_wdt_close();
#endif
        TRACE("!!!!!ENGINEER_MODE!!!!!\n");
        nRet = 0;
        app_nvrecord_rebuild();
        app_factorymode_key_init();
        if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_SIGNALINGMODE){
            hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MASK);
            app_factorymode_bt_signalingtest(NULL, NULL);
        }
        if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE){
            hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MASK);
            app_factorymode_bt_nosignalingtest(NULL, NULL);
        }
    }
#endif
    else{
        app_status_indication_set(APP_STATUS_INDICATION_POWERON);
#ifdef MEDIA_PLAYER_SUPPORT
        app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
#endif
        app_poweron_key_init();
        pwron_case = app_poweron_wait_case();
        {
            if (pwron_case != APP_POWERON_CASE_INVALID && pwron_case != APP_POWERON_CASE_DITHERING){
                TRACE("hello world i'm bes1000 hahaha case:%d\n", pwron_case);
                nRet = 0;
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
                app_status_indication_set(APP_STATUS_INDICATION_INITIAL);
#endif
                app_bt_start_custom_function_in_bt_thread((uint32_t)1,
                            0, (uint32_t)btif_me_write_bt_sleep_enable);
                btdrv_set_lpo_times();
#ifdef __AI_VOICE__
                app_ai_spp_server_init();
#endif

#ifdef BES_OTA_BASIC
                bes_ota_init();
#endif

#ifdef __INTERCONNECTION__
                app_interconnection_init();
#endif

#if defined(__IAG_BLE_INCLUDE__) && defined(BTIF_BLE_APP_DATAPATH_SERVER)
                BLE_custom_command_init();
#endif
                switch (pwron_case) {
                    case APP_POWERON_CASE_CALIB:
                        break;
                    case APP_POWERON_CASE_BOTHSCAN:
                        app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
#ifdef MEDIA_PLAYER_SUPPORT
                        app_voice_report(APP_STATUS_INDICATION_BOTHSCAN,0);
#endif
#if defined( __BTIF_EARPHONE__)
#if defined(IBRT)
#ifdef IBRT_SEARCH_UI
                        app_ibrt_enter_limited_mode();                        
#endif
#else
                        app_bt_accessmode_set(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#endif
#ifdef _GFPS_
                        app_enter_fastpairing_mode();
#endif
#if defined(__BTIF_AUTOPOWEROFF__)
                        app_start_10_second_timer(APP_PAIR_TIMER_ID);
#endif
#endif
#ifdef __THIRDPARTY
                        app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO2,THIRDPARTY_BT_DISCOVERABLE);
#endif
                        break;
                    case APP_POWERON_CASE_NORMAL:
#if defined( __BTIF_EARPHONE__ ) && !defined(__EARPHONE_STAY_BOTH_SCAN__)
#if defined(IBRT)
#ifdef IBRT_SEARCH_UI
                        if(IBRT_UNKNOW == nvrecord_env->ibrt_mode.mode)
                        {
                            TRACE("ibrt_ui_log:power on unknow mode");
                            app_ibrt_enter_limited_mode();
                        }
                        else
                        {
                            TRACE("ibrt_ui_log:power on %d fetch out", nvrecord_env->ibrt_mode.mode);
                            app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
                        }
#endif
#else
                        app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);
#endif
#endif
                    case APP_POWERON_CASE_REBOOT:
                    case APP_POWERON_CASE_ALARM:
                    default:
                        app_status_indication_set(APP_STATUS_INDICATION_PAGESCAN);
#if defined( __BTIF_EARPHONE__) && defined(__BTIF_BT_RECONNECT__) && !defined(IBRT)
                        app_bt_profile_connect_manager_opening_reconnect();
#endif
#ifdef __THIRDPARTY
                        app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO2,THIRDPARTY_BT_CONNECTABLE);
#endif

                        break;
                }
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
                app_poweron_wait_finished();
#endif
                app_key_init();
                app_battery_start();
#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)
                app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif
#ifdef __THIRDPARTY
                app_thirdparty_specific_lib_event_handle(THIRDPARTY_ID_NO1,THIRDPARTY_START);
#endif

#ifdef RB_CODEC
                rb_ctl_init();
#endif
            }else{
                af_close();
                app_key_close();
                nRet = -1;
            }
        }
    }
exit:
#ifdef ANC_APP
    app_anc_set_init_done();
#endif
#ifdef BT_USB_AUDIO_DUAL_MODE
    if(usb_plugin)
    {
        btusb_switch(BTUSB_MODE_USB);
    }
    else
    {
        btusb_switch(BTUSB_MODE_BT);
    }
#else //BT_USB_AUDIO_DUAL_MODE
#if defined(BTUSB_AUDIO_MODE)
    if(pwron_case == APP_POWERON_CASE_CHARGING) {
#ifdef __WATCHER_DOG_RESET__
        app_wdt_close();
#endif
        af_open();
        app_key_handle_clear();
        app_usb_key_init();
        app_usbaudio_entry();
    }

#endif // BTUSB_AUDIO_MODE
#endif // BT_USB_AUDIO_DUAL_MODE
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    btdrv_tportopen();

    return nRet;
}

#endif /* APP_TEST_MODE */
