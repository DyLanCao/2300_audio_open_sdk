/**
 ****************************************************************************************
 *
 * @file app_ble_mode_switch.c
 *
 * @date 12 July 2018
 *
 * @brief The application for BLE advertising/scanning/connecting mode switch
 *
 * Copyright (C) 2018
 *
 *
 ****************************************************************************************
 */
#include "string.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "stdbool.h"
#include "app_thread.h"
#include "app_utils.h"
#include "apps.h"

#include "app.h"

#include "app_ble_mode_switch.h"
#include "nvrecord.h"
#include "app_bt_func.h"
#include "hal_timer.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "rwprf_config.h"
#ifdef __AI_VOICE__
#include "ai_manager.h"
#endif

#ifdef  __APP_WL_SMARTVOICE__
#include "app_wl_smartvoice.h"
// #include "factory_section.h"
#endif

static void app_start_ble_advertising_handler(uint8_t advType, uint16_t adv_interval_in_ms);
static void app_start_ble_scanning_handler(uint16_t scanning_window_in_ms, uint16_t scanning_interval_in_ms);
static void app_start_ble_connecting_handler(uint8_t* bleBdAddr);
static void app_restart_ble_advertising(uint8_t advType, uint16_t adv_interval_in_ms);

static void app_stop_fast_connectable_ble_adv_timer(void);
static void ble_stop_all_activities_handler(void);

#ifdef __INTERCONNECTION__
bool isInterconnectionFastAdvTimeout = false;
static void app_interconnection_adv_timeout_callback(void const *param);
osTimerDef(APP_INTERCONNECTION_ADV_TIMER, app_interconnection_adv_timeout_callback);
static osTimerId app_interconnection_adv_timer = NULL;
static bool isInterconnectionAdvTimerOn = false;

static void app_interconnection_disappear_adv_timeout_callback(void const *param);
osTimerDef(APP_INTERCONNECTION_DISAPPEAR_TIMER, app_interconnection_disappear_adv_timeout_callback);
static osTimerId app_interconnection_disappear_timer = NULL;
static bool isInterconnectionAdvDisappearTimerOn = false;
#endif

static BLE_MODE_ENV_T bleModeEnv;

static BLE_OP_E pendingBLEOperation = BLE_OP_IDLE;

extern uint8_t bt_addr[6];

#ifdef GSOUND_ENABLED
extern bool app_gsound_ble_is_connected( void );
#endif

#if 0
#define SET_BLE_STATE(newState)	do { \
                                    TRACE("ble state from %d to %d at line %d", bleModeEnv.state, newState, __LINE__); \
                                    bleModeEnv.state = (newState); \
                                } while (0);
#else
#define SET_BLE_STATE(newState)	do { \
                                    bleModeEnv.state = (newState); \
                                } while (0);
#endif
#ifdef IS_MULTI_AI_ENABLED
volatile uint8_t current_adv_type_temp_selected = AI_SPEC_GSOUND;
osTimerId gva_ama_cross_adv_timer = NULL;
static void ble_gva_ama_cross_adv_timer_handler(void const *param);
osTimerDef (BLE_GVA_AMA_CROSS_ADV_TIMER, (void (*)(void const *))ble_gva_ama_cross_adv_timer_handler);
static void ble_gva_ama_cross_adv_timer_handler(void const *param)
{
    TRACE("cross adv timer callback handler");
    appm_stop_advertising();
    if (current_adv_type_temp_selected == AI_SPEC_GSOUND)
    {
        current_adv_type_temp_selected = AI_SPEC_AMA;
    }
    else if (current_adv_type_temp_selected == AI_SPEC_AMA)
    {
        current_adv_type_temp_selected = AI_SPEC_GSOUND;
    }
    app_start_ble_advertising(GAPM_ADV_UNDIRECT, GVA_AMA_CROSS_ADV_TIME_INTERVAL);
}
#endif

void ble_pending_operation_handler(void)
{
    BLE_OP_E op = pendingBLEOperation;
    pendingBLEOperation = BLE_OP_IDLE;
    switch (op)
    {
        case BLE_OP_START_ADV:
            app_start_ble_advertising_handler(bleModeEnv.advType, bleModeEnv.advIntervalInMs);
            break;
        case BLE_OP_START_SCANNING:
            app_start_ble_scanning_handler(bleModeEnv.scanWindowInMs, bleModeEnv.scanIntervalInMs);
            break;
        case BLE_OP_START_CONNECTING:
            app_start_ble_connecting_handler(bleModeEnv.bleAddrToConnect);
            break;
        case BLE_OP_STOP_ADV:
        case BLE_OP_STOP_SCANNING:
        case BLE_OP_STOP_CONNECTING:
            ble_stop_all_activities_handler();
            break;
        default:
            break;
    }
}

static void app_ble_switch_activities(void)
{

    switch (bleModeEnv.state)
    {
        case BLE_STATE_STARTING_ADV:
            SET_BLE_STATE(BLE_STATE_ADVERTISING);
            break;
        case BLE_STATE_STARTING_SCANNING:
            SET_BLE_STATE(BLE_STATE_SCANNING);
            break;
        case BLE_STATE_STARTING_CONNECTING:
            SET_BLE_STATE(BLE_STATE_CONNECTING);
            break;
        case BLE_STATE_STOPPING_ADV:
        case BLE_STATE_STOPPING_SCANNING:
        case BLE_STATE_STOPPING_CONNECTING:
            SET_BLE_STATE(BLE_STATE_IDLE);
            break;
        default:
            break;
    }

    ble_pending_operation_handler();
}

void app_restart_operation(void)
{
    switch (bleModeEnv.state)
    {
        case BLE_STATE_ADVERTISING:
            TRACE("restart adv");
            app_start_ble_advertising_handler(bleModeEnv.advType, bleModeEnv.advIntervalInMs);
            break;
        case BLE_STATE_SCANNING:
            TRACE("restart scan");
            app_start_ble_scanning_handler(bleModeEnv.scanWindowInMs, bleModeEnv.scanIntervalInMs);
            break;
        default:
            break;
    }
}

static void app_ble_advertising_starting_failed_handler(void)
{
    if (BLE_STATE_STARTING_ADV == bleModeEnv.state)
    {
        bleModeEnv.state = BLE_STATE_IDLE;
    }
}

/**
 * @brief Call back function when the advertising is stopped
 *
 */
void app_advertising_stopped(void)
{
    //TRACE("ble adv is stopped. bleModeEnv.state is %d", bleModeEnv.state);
    app_bt_start_custom_function_in_bt_thread(0,
            0, (uint32_t)app_ble_switch_activities);
}

void app_advertising_starting_failed(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
            0, (uint32_t)app_ble_advertising_starting_failed_handler);
}

/**
 * @brief Call back function when the scanning is stopped
 *
 */
 //stop scan and then start adv
void app_scanning_stopped(void)
{
    //TRACE("Scanning is stopped. bleModeEnv.state is %d", bleModeEnv.state);
    app_bt_start_custom_function_in_bt_thread(0,
        0, (uint32_t)app_ble_switch_activities);
}

void app_connecting_started(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
            0, (uint32_t)app_ble_switch_activities);

}

void app_connecting_stopped(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
            0, (uint32_t)app_ble_switch_activities);

}

void app_advertising_started(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
            0, (uint32_t)app_ble_switch_activities);

}

void app_scanning_started(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
            0, (uint32_t)app_ble_switch_activities);

}

static void app_configure_pending_ble_scanning(uint16_t scanning_window_in_ms, uint16_t scanning_interval_in_ms)
{
    bleModeEnv.scanWindowInMs = scanning_window_in_ms;
    bleModeEnv.scanIntervalInMs = scanning_interval_in_ms;
    pendingBLEOperation = BLE_OP_START_SCANNING;
}

static void app_start_ble_scanning_handler(uint16_t scanning_window_in_ms, uint16_t scanning_interval_in_ms)
{
    TRACE("Start BLE scan with scan_win:%d ms scan_intv:%d state is %d", scanning_window_in_ms, scanning_interval_in_ms, bleModeEnv.state);

    switch (bleModeEnv.state)
    {
        case BLE_STATE_ADVERTISING:
            app_configure_pending_ble_scanning(scanning_window_in_ms, scanning_interval_in_ms);
            SET_BLE_STATE(BLE_STATE_STOPPING_ADV);
            appm_stop_advertising();
            break;
        case BLE_STATE_SCANNING:
            app_configure_pending_ble_scanning(scanning_window_in_ms, scanning_interval_in_ms);
            SET_BLE_STATE(BLE_STATE_STOPPING_SCANNING);
            appm_stop_scanning();
            break;
        case BLE_STATE_CONNECTING:
            app_configure_pending_ble_scanning(scanning_window_in_ms, scanning_interval_in_ms);
            SET_BLE_STATE(BLE_STATE_STOPPING_CONNECTING);
            appm_stop_connecting();
            break;
        case BLE_STATE_STARTING_ADV:
        case BLE_STATE_STARTING_SCANNING:
        case BLE_STATE_STARTING_CONNECTING:
        case BLE_STATE_STOPPING_ADV:
        case BLE_STATE_STOPPING_SCANNING:
        case BLE_STATE_STOPPING_CONNECTING:
            app_configure_pending_ble_scanning(scanning_window_in_ms, scanning_interval_in_ms);
            break;
        case BLE_STATE_IDLE:
            bleModeEnv.scanWindowInMs = scanning_window_in_ms;
            bleModeEnv.scanIntervalInMs = scanning_interval_in_ms;
            SET_BLE_STATE(BLE_STATE_STARTING_SCANNING);
            appm_start_scanning(bleModeEnv.scanIntervalInMs, bleModeEnv.scanWindowInMs, SCAN_ALLOW_ADV_WLST);
            break;
        default:
            break;
    }
}

void app_start_ble_scanning(uint16_t scanning_window_in_ms, uint16_t scanning_interval_in_ms)
{
    app_bt_start_custom_function_in_bt_thread(scanning_window_in_ms, scanning_interval_in_ms,
        (uint32_t)app_start_ble_scanning_handler);
}

static void app_configure_pending_ble_advertising(uint8_t advType, uint16_t adv_interval_in_ms)
{
    bleModeEnv.advIntervalInMs = adv_interval_in_ms;
    bleModeEnv.advType = advType;
    pendingBLEOperation = BLE_OP_START_ADV;
}

static void app_restart_ble_advertising(uint8_t advType, uint16_t adv_interval_in_ms)
{
    app_configure_pending_ble_advertising(advType, adv_interval_in_ms);
    SET_BLE_STATE(BLE_STATE_STOPPING_ADV);
    appm_stop_advertising();
}

static void app_start_ble_advertising_handler(uint8_t advType, uint16_t adv_interval_in_ms)
{
    if (btapp_hfp_is_sco_active())
    {
        TRACE("BLE ADV is started when the hfp call is active! Ignore this until the \
            HFP sco link is down.");
        return;
    }

    TRACE("Start BLE adv with interval %d ms advType:%d state is %d", adv_interval_in_ms, advType, bleModeEnv.state);
    switch (bleModeEnv.state)
    {
        case BLE_STATE_ADVERTISING:
            app_restart_ble_advertising(advType, adv_interval_in_ms);
            break;
        case BLE_STATE_SCANNING:
            app_configure_pending_ble_advertising(advType, adv_interval_in_ms);
            SET_BLE_STATE(BLE_STATE_STOPPING_SCANNING);
            appm_stop_scanning();
            break;
        case BLE_STATE_CONNECTING:
            app_configure_pending_ble_advertising(advType, adv_interval_in_ms);
            SET_BLE_STATE(BLE_STATE_STOPPING_CONNECTING);
            appm_stop_connecting();
            break;
        case BLE_STATE_STARTING_ADV:
        case BLE_STATE_STARTING_SCANNING:
        case BLE_STATE_STARTING_CONNECTING:
        case BLE_STATE_STOPPING_ADV:
        case BLE_STATE_STOPPING_SCANNING:
        case BLE_STATE_STOPPING_CONNECTING:
            app_configure_pending_ble_advertising(advType, adv_interval_in_ms);
            break;
        case BLE_STATE_IDLE:
        case BLE_STATE_CONNECTED:
        {
            TRACE("start adv condition:");
#ifdef GSOUND_ENABLED
            TRACE("%d:%d:%d:%d:%d",
                app_is_arrive_at_max_ble_connections(),
                app_bt_is_in_connecting_profiles_state(),
                app_is_resolving_ble_bd_addr(),
                app_ble_connection_count(),
                app_gsound_ble_is_connected());
#else
            TRACE("%d:%d:%d:%d",
                app_is_arrive_at_max_ble_connections(),
                app_bt_is_in_connecting_profiles_state(),
                app_is_resolving_ble_bd_addr(),
                app_ble_connection_count());
#endif

            if (app_is_arrive_at_max_ble_connections() ||
                app_bt_is_in_connecting_profiles_state() ||
                app_is_resolving_ble_bd_addr())
            {
                break;
            }


            bleModeEnv.advType = advType;
            bleModeEnv.advIntervalInMs = adv_interval_in_ms;

            appm_start_advertising(bleModeEnv.advType,
                bleModeEnv.advIntervalInMs, bleModeEnv.advIntervalInMs);
            SET_BLE_STATE(BLE_STATE_STARTING_ADV);
            break;
        }
        default:
            break;
    }
}

void app_update_ble_adv(uint8_t advType, uint16_t adv_interval_in_ms)
{
    if ((bleModeEnv.advIntervalInMs != adv_interval_in_ms) ||
        (bleModeEnv.advType != advType))
    {
        app_start_ble_advertising(advType, adv_interval_in_ms);
    }
}

void app_start_ble_advertising(uint8_t advType, uint16_t adv_interval_in_ms)
{
    if(app_is_power_off_in_progress())
    {
        return;
    }
    app_bt_start_custom_function_in_bt_thread(advType, adv_interval_in_ms, 
        (uint32_t)app_start_ble_advertising_handler);
}

#ifdef __INTERCONNECTION__
void clear_discoverable_adv_timeout_flag(void)
{
    isInterconnectionFastAdvTimeout = false;
}

/**
 * @brief : is interconnection discoverable advertising allowed
 * 
 * @return true 
 * @return false 
 */
static bool is_interconnection_discoverable_adv_allowed()
{
    // in pairing mode
    if(BTIF_BAM_GENERAL_ACCESSIBLE == app_bt_get_current_access_mode())
    {
        return true;
    }
    else
    {
        TRACE("current access mode is %d", app_bt_get_current_access_mode());
        return false;
    }
}

/**
 * @brief : set is_interconnection_discoverable_adv flag
 * 
 * @param discoverable 
 */
void app_set_interconnection_discoverable(uint8_t discoverable)
{
    bleModeEnv.interconnectionDiscoverable = discoverable;
}

/**
 * @brief : get discoverable state of interconnection
 * 
 * @return uint8_t 
 */
uint8_t app_get_interconnection_discoverable(void)
{
    return bleModeEnv.interconnectionDiscoverable;
}


/**
 * @brief : start interconnection disappear advertise
 * 
 */
void app_interconnection_start_disappear_adv(uint32_t advInterval, uint32_t advDuration)
{
    isInterconnectionAdvTimerOn = false;
    isInterconnectionFastAdvTimeout = false;
    osTimerStop(app_interconnection_adv_timer);

    if (NULL == app_interconnection_disappear_timer)
    {
        app_interconnection_disappear_timer = osTimerCreate(osTimer(APP_INTERCONNECTION_DISAPPEAR_TIMER), osTimerOnce, NULL);	
    }

    // start BLE adv
    app_set_interconnection_discoverable(INTERCONNECTION_DISCOVERABLE_NOT);
    app_start_ble_advertising(GAPM_ADV_UNDIRECT, advInterval);

    if(!isInterconnectionAdvDisappearTimerOn)
    {
        osTimerStart(app_interconnection_disappear_timer, advDuration);
        isInterconnectionAdvDisappearTimerOn = true;
    }
}

/**
 * @brief : callback function of interconnection disappear advertise timeout
 * 
 * @param param 
 */
static void app_interconnection_disappear_adv_timeout_callback(void const *param)
{
    isInterconnectionAdvDisappearTimerOn = false;
    ble_stop_all_activities_handler();
}

/**
 * @brief : start discoverable ble advertising
 * 
 */
void app_interceonnection_start_discoverable_adv(uint32_t advInterval, uint32_t advDuration)
{

    if(NULL == app_interconnection_adv_timer)
    {
        app_interconnection_adv_timer = osTimerCreate(osTimer(APP_INTERCONNECTION_ADV_TIMER), osTimerOnce, NULL);	
    }

    // adv interval check
    if(advInterval == BLE_FAST_ADVERTISING_INTERVAL)
    {
        isInterconnectionFastAdvTimeout = false;
    }
    else if(advInterval == BLE_ADVERTISING_INTERVAL)
    {
        isInterconnectionFastAdvTimeout = true;
    }
    else
    {
        TRACE("invalid adv interval param");
        return;
    }

    // ble advertising permission check
    if(is_interconnection_discoverable_adv_allowed())
    {
        isInterconnectionAdvDisappearTimerOn = false;
        osTimerStop(app_interconnection_disappear_timer);

        // start BLE adv
        app_set_interconnection_discoverable(INTERCONNECTION_DISCOVERABLE_YES);
        app_start_ble_advertising(GAPM_ADV_UNDIRECT, advInterval);

        osTimerStart(app_interconnection_adv_timer, advDuration);
        isInterconnectionAdvTimerOn = true;
    }
}

/**
 * @brief : discoverable advertising timeout callback
 * 
 * @param param 
 */
static void app_interconnection_adv_timeout_callback(void const *param)
{
    if(true == isInterconnectionFastAdvTimeout)
    {
        isInterconnectionAdvTimerOn = false;
        app_interconnection_start_disappear_adv(BLE_ADVERTISING_INTERVAL, APP_INTERCONNECTION_DISAPPEAR_ADV_IN_MS);
    }
    else
    {
        isInterconnectionFastAdvTimeout = true;
        app_interceonnection_start_discoverable_adv(BLE_ADVERTISING_INTERVAL, APP_INTERCONNECTION_SLOW_ADV_TIMEOUT_IN_MS);
    }
}
#endif

void app_start_connectable_ble_adv(uint16_t adv_interval_in_ms)
{
    app_bt_start_custom_function_in_bt_thread(GAPM_ADV_UNDIRECT, adv_interval_in_ms,
        (uint32_t)app_start_ble_advertising_handler);
}

#define APP_FAST_BLE_ADV_TIMEOUT_IN_MS	30000

osTimerId app_fast_ble_adv_timeout_timer = NULL;
static int app_fast_ble_adv_timeout_timehandler(void const *param);
osTimerDef (APP_FAST_BLE_ADV_TIMEOUT_TIMER, (void (*)(void const *))app_fast_ble_adv_timeout_timehandler);                      // define timers
static int app_fast_ble_adv_timeout_timehandler(void const *param)
{
    app_bt_start_custom_function_in_bt_thread(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL,
        (uint32_t)app_start_ble_advertising_handler);

    return 0;
}

void app_start_fast_connectable_ble_adv(uint16_t adv_interval_in_ms)
{
    if (NULL == app_fast_ble_adv_timeout_timer)
    {
        app_fast_ble_adv_timeout_timer =
            osTimerCreate(osTimer(APP_FAST_BLE_ADV_TIMEOUT_TIMER),
            osTimerOnce, NULL);
    }

    osTimerStart(app_fast_ble_adv_timeout_timer, APP_FAST_BLE_ADV_TIMEOUT_IN_MS);

    app_bt_start_custom_function_in_bt_thread(GAPM_ADV_UNDIRECT, adv_interval_in_ms,
        (uint32_t)app_start_ble_advertising_handler);
}

static void app_stop_fast_connectable_ble_adv_timer(void)
{
    if (NULL != app_fast_ble_adv_timeout_timer)
    {
        osTimerStop(app_fast_ble_adv_timeout_timer);
    }
}

bool app_ble_is_in_connected_state(void)
{
    return (BLE_STATE_CONNECTED == bleModeEnv.state);
}

bool app_ble_is_in_advertising_state(void)
{
    return (BLE_STATE_ADVERTISING == bleModeEnv.state) ||
        (BLE_STATE_STARTING_ADV == bleModeEnv.state) ||
        (BLE_STATE_STOPPING_ADV == bleModeEnv.state);
}

void app_disconnected_evt_handler(void)
{
    if (BLE_STATE_CONNECTED == bleModeEnv.state)
    {
        SET_BLE_STATE(BLE_STATE_IDLE);
    }
    app_start_ble_advertising(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL);
}

static void app_connected_evt_handler_in_app_thread(void)
{
    if ((BLE_STATE_ADVERTISING == bleModeEnv.state) ||
        (BLE_STATE_CONNECTING == bleModeEnv.state))
    {
        SET_BLE_STATE(BLE_STATE_CONNECTED);  
    }
    app_start_ble_advertising(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL);
}

void app_connected_evt_handler(const uint8_t* pPeerBdAddress)
{
    app_stop_fast_connectable_ble_adv_timer();

    app_bt_start_custom_function_in_bt_thread(0,
                0, (uint32_t)app_connected_evt_handler_in_app_thread);
}

static void app_start_ble_connecting_handler(uint8_t* bleBdAddr)
{
    if ((BLE_STATE_CONNECTING != bleModeEnv.state) &&
        (BLE_STATE_STARTING_CONNECTING != bleModeEnv.state))
    {
        switch (bleModeEnv.state)
        {
            case BLE_STATE_ADVERTISING:
                SET_BLE_STATE(BLE_STATE_STOPPING_ADV);
                pendingBLEOperation = BLE_OP_START_CONNECTING;
                appm_stop_advertising();
                break;
            case BLE_STATE_SCANNING:
                SET_BLE_STATE(BLE_STATE_STOPPING_SCANNING);
                pendingBLEOperation = BLE_OP_START_CONNECTING;
                appm_stop_scanning();
                break;
            case BLE_STATE_IDLE:
            {
                // start connecting
                SET_BLE_STATE(BLE_STATE_STARTING_CONNECTING);
                struct gap_bdaddr bdAddr;
                memcpy(bdAddr.addr.addr, bleBdAddr, BTIF_BD_ADDR_SIZE);
                bdAddr.addr_type = 0;
                TRACE("Master paired with mobile dev is scanned, connect it via BLE.");
                appm_start_connecting(&bdAddr);
                break;
            }
            default:
                pendingBLEOperation = BLE_OP_START_CONNECTING;
                break;
        }
    }
}

void app_start_ble_connecting(uint8_t* bdAddrToConnect)
{
    memcpy(bleModeEnv.bleAddrToConnect, bdAddrToConnect, BD_ADDR_LEN);
    app_bt_start_custom_function_in_bt_thread((uint32_t)bdAddrToConnect, 0,
        (uint32_t)app_start_ble_connecting_handler);
}

static uint32_t POSSIBLY_UNUSED ble_get_manufacture_data_ptr(uint8_t* advData, uint32_t dataLength, uint8_t* manufactureData)
{
    uint8_t followingDataLengthOfSection;
    uint8_t rawContentDataLengthOfSection;
    uint8_t flag;
    while (dataLength > 0)
    {
        followingDataLengthOfSection = *advData++;
        dataLength--;
        if (dataLength < followingDataLengthOfSection)
        {
            return 0;	// wrong adv data format
        }

        if (followingDataLengthOfSection > 0)
        {
            flag = *advData++;
            dataLength--;

            rawContentDataLengthOfSection = followingDataLengthOfSection - 1;
            if (BLE_ADV_MANU_FLAG == flag)
            {
                uint32_t lengthToCopy;
                if (dataLength < rawContentDataLengthOfSection)
                {
                    lengthToCopy = dataLength;
                }
                else
                {
                    lengthToCopy = rawContentDataLengthOfSection;
                }

                memcpy(manufactureData, advData - 2, lengthToCopy + 2);
                return lengthToCopy + 2;
            }
            else
            {
                advData += rawContentDataLengthOfSection;
                dataLength -= rawContentDataLengthOfSection;
            }
        }
    }

    return 0;
}

static void ble_adv_data_parse(uint8_t* bleBdAddr, int8_t rssi, unsigned char *adv_buf, unsigned char len)
{

}

//received adv data
void app_adv_reported_scanned(struct gapm_adv_report_ind* ptInd)
{
    /*
    TRACE("Scanned RSSI %d BD addr:", (int8_t)ptInd->report.rssi);
    DUMP8("0x%02x ", ptInd->report.adv_addr.addr, BTIF_BD_ADDR_SIZE);
    TRACE("Scanned adv data:");
    DUMP8("0x%02x ", ptInd->report.data, ptInd->report.data_len);
    */

    ble_adv_data_parse(ptInd->report.adv_addr.addr, (int8_t)ptInd->report.rssi,
        ptInd->report.data, (unsigned char)ptInd->report.data_len);
}

/**
 * @brief Initialize the BLE application for TWS
 *
 */
static void ble_mode_init(void)
{
    SET_BLE_STATE(BLE_STATE_IDLE);
    pendingBLEOperation = BLE_OP_IDLE;
#ifdef IS_MULTI_AI_ENABLED
    if (gva_ama_cross_adv_timer == NULL)
        gva_ama_cross_adv_timer = osTimerCreate(osTimer(BLE_GVA_AMA_CROSS_ADV_TIMER), osTimerOnce, NULL);
#endif
}

static void ble_stop_all_activities_handler(void)
{
    switch (bleModeEnv.state)
    {
        case BLE_STATE_ADVERTISING:
        {
            //TRACE("Stop BLE Adv!");
            SET_BLE_STATE(BLE_STATE_STOPPING_ADV);
            appm_stop_advertising();
            break;
        }
        case BLE_STATE_SCANNING:
        {
            //TRACE("Stop BLE scanning!");
            SET_BLE_STATE(BLE_STATE_STOPPING_SCANNING);
            appm_stop_scanning();
            break;
        }
        case BLE_STATE_CONNECTING:
        {
            //TRACE("Stop BLE scanning!");
            SET_BLE_STATE(BLE_STATE_STOPPING_CONNECTING);
            appm_stop_connecting();
            break;
        }
        case BLE_STATE_STARTING_ADV:
        {
            pendingBLEOperation = BLE_OP_STOP_ADV;
            break;
        }
        case BLE_STATE_STARTING_SCANNING:
        {
            pendingBLEOperation = BLE_OP_STOP_SCANNING;
            break;
        }
        case BLE_STATE_STARTING_CONNECTING:
            pendingBLEOperation = BLE_OP_STOP_CONNECTING;
            break;
        case BLE_STATE_STOPPING_ADV:
        case BLE_STATE_STOPPING_SCANNING:
        case BLE_STATE_STOPPING_CONNECTING:
            pendingBLEOperation = BLE_OP_IDLE;
            break;
        default:
            break;
    }
}

void ble_stop_activities(void)
{
    TRACE("%s", __func__);
    app_stop_fast_connectable_ble_adv_timer();
    app_bt_start_custom_function_in_bt_thread(0, 0,
        (uint32_t)ble_stop_all_activities_handler);
}
extern void app_notify_stack_ready();

#ifdef CHIP_BEST2000
extern void bt_drv_reg_set_rssi_seed(uint32_t seed);
#endif
void app_ble_system_ready(void)
{
#ifdef CHIP_BEST2000
    uint32_t generatedSeed = hal_sys_timer_get();
    for (uint8_t index = 0; index < sizeof(bt_addr); index++)
    {
        generatedSeed ^= (((uint32_t)(bt_addr[index])) << (hal_sys_timer_get()&0xF));
    }
    bt_drv_reg_set_rssi_seed(generatedSeed);
#endif
    ble_mode_init();
#ifdef __INTERCONNECTION__
    // app_interceonnection_start_discoverable_adv(BLE_FAST_ADVERTISING_INTERVAL, APP_INTERCONNECTION_FAST_ADV_TIMEOUT_IN_MS);
#else
    app_start_ble_advertising(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL);
#endif
}

