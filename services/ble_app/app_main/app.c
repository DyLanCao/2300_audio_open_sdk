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
/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration
#if (BLE_APP_PRESENT)
#include <string.h>

#include "app_task.h"                // Application task Definition
#include "app.h"                     // Application Definition
#include "gap.h"                     // GAP Definition
#include "tgt_hardware.h"
#include "gapm.h"
#include "gapm_task.h"               // GAP Manager Task API
#include "gapc_task.h"               // GAP Controller Task API

#include "gattc_task.h"

#include "co_bt.h"                   // Common BT Definition
#include "co_math.h"                 // Common Maths Definition
#include "l2cc.h"
#include "l2cc_pdu.h"

#include "nvrecord_ble.h"
#include "prf.h"

#include "nvrecord_env.h"
#ifndef _BLE_NVDS_
#include "tgt_hardware.h"
#endif
#ifdef __AI_VOICE__
#include "ai_manager.h"
#include "ai_thread.h"
#include "ai_control.h"
#include "app_ai_ble.h"                  // AI Voice Application Definitions
#endif //(__AI_VOICE__)

#if (BLE_APP_SEC)
#include "app_sec.h"                 // Application security Definition
#endif // (BLE_APP_SEC)


#if (BLE_APP_DATAPATH_SERVER)
#include "app_datapath_server.h"                  // Data Path Server Application Definitions
#endif //(BLE_APP_DATAPATH_SERVER)

#if (BLE_APP_DIS)
#include "app_dis.h"                 // Device Information Service Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"                // Battery Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_HID)
#include "app_hid.h"                 // HID Application Definitions
#endif //(BLE_APP_HID)

#if (BLE_APP_VOICEPATH)
#include "app_voicepath_ble.h"         // Voice Path Application Definitions
#endif //(BLE_APP_VOICEPATH)

#if (BLE_APP_OTA)
#include "app_ota.h"                  // OTA Application Definitions
#endif //(BLE_APP_OTA)

#if (BLE_APP_TOTA)
#include "app_tota_ble.h"                  // OTA Application Definitions
#endif //(BLE_APP_TOTA)

#if (BLE_APP_ANCC)
#include "app_ancc.h"                  // ANCC Application Definitions
#endif //(BLE_APP_ANCC)

#if (BLE_APP_AMS)
#include "app_amsc.h"               // AMS Module Definition
#endif // (BLE_APP_AMS)

#if (BLE_APP_GFPS)
#include "app_gfps.h"               // Google Fast Pair Service Definitions
#endif
#if (DISPLAY_SUPPORT)
#include "app_display.h"             // Application Display Definition
#endif //(DISPLAY_SUPPORT)

#ifdef BLE_APP_AM0
#include "am0_app.h"                 // Audio Mode 0 Application
#endif //defined(BLE_APP_AM0)

#if (NVDS_SUPPORT)
#include "nvds.h"                    // NVDS Definitions
#endif //(NVDS_SUPPORT)

#include "cmsis_os.h"
#include "ke_timer.h"
#include "nvrecord.h"
#define _FILE_TAG_ "APP"
#include "color_log.h"
#ifdef GSOUND_ENABLED
#include "gsound_service.h"
#endif
#include "app_bt.h"
#include "app_ble_mode_switch.h"
#include "apps.h"
#include "crc16.h"

#if defined(__INTERCONNECTION__)
#include "app_bt.h"
#include "app_battery.h"
#endif
/*
 * DEFINES
 ****************************************************************************************
 */
/// Default Device Name if no value can be found in NVDS
#define APP_DFLT_DEVICE_NAME            ("BES_BLE")
#define APP_DFLT_DEVICE_NAME_LEN        (sizeof(APP_DFLT_DEVICE_NAME))

#if (BLE_APP_HID)
// HID Mouse
#define DEVICE_NAME        "Hid Mouse"
#else
#define DEVICE_NAME        "RW DEVICE"
#endif

#define DEVICE_NAME_SIZE    sizeof(DEVICE_NAME)

/**
 * UUID List part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x03 - Complete list of 16-bit UUIDs available
 * x09\x18 - Health Thermometer Service UUID
 *   or
 * x12\x18 - HID Service UUID
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_UUID        "\x03\x03\x09\x18"
#define APP_HT_ADV_DATA_UUID_LEN    (4)
#endif //(BLE_APP_HT)


#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_UUID       "\x03\x03\x12\x18"
#define APP_HID_ADV_DATA_UUID_LEN   (4)
#endif //(BLE_APP_HID)
#ifdef _GFPS_
#define APP_GFPS_ADV_FLAGS_UUID      "\x02\x01\x01"
#define APP_GFPS_ADV_FLAGS_UUID_LEN  (3)
#define APP_GFPS_ADV_GFPS_MODULE_ID_UUID  "\x06\x16\x2C\xFE\x00\x01\x04"
#define APP_GFPS_ADV_GFPS_MODULE_ID_UUID_LEN (7)
#define APP_GFPS_ADV_GFPS_1_0_SERVICE_DATA  "\x06\x16\x2C\xFE\x05\x02\xF0"
#define APP_GFPS_ADV_POWER_UUID  "\x02\x0a\xEC"
#define APP_GFPS_ADV_POWER_UUID_LEN (3)
#define APP_GFPS_ADV_APPEARANCE_UUID   "\x03\x19\xda\x96"
#define APP_GFPS_ADV_APPEARANCE_UUID_LEN (4)
#define APP_GFPS_ADV_MANU_SPE_UUID_TEST  "\x07\xFF\xe0\x00\x01\x5B\x32\x01"
#define APP_GFPS_ADV_MANU_SPE_UUID_LEN (8)
#endif
#if (BLE_APP_DATAPATH_SERVER)
/*
 * x11 - Length
 * x07 - Complete list of 16-bit UUIDs available
 * .... the 128 bit UUIDs
 */
#define APP_DATAPATH_SERVER_ADV_DATA_UUID        "\x11\x07\x9e\x34\x9B\x5F\x80\x00\x00\x80\x00\x10\x00\x00\x00\x01\x00\x01"
#define APP_DATAPATH_SERVER_ADV_DATA_UUID_LEN    (18)
#endif //(BLE_APP_HT)

#ifdef __AMA_VOICE__
#define APP_AMA_UUID            "\x03\x16\x03\xFE"
#define APP_AMA_UUID_LEN        (4)
//#define APP_AMA_UUID            "\x11\x07\xfb\x34\x9b\x5f\x80\x00\x00\x80\x00\x10\x00\x00\x03\xFE\x00\x00"
//#define APP_AMA_UUID_LEN        (18)
#endif
/**
 * Appearance part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x19 - Appearance
 * x03\x00 - Generic Thermometer
 *   or
 * xC2\x04 - HID Mouse
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_APPEARANCE    "\x03\x19\x00\x03"
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_APPEARANCE   "\x03\x19\xC2\x03"
#endif //(BLE_APP_HID)

#define APP_ADV_DATA_APPEARANCE_LEN  (4)



/**
 * Advertising Parameters
 */
#if (BLE_APP_HID)
/// Default Advertising duration - 30s (in multiple of 10ms)
#define APP_DFLT_ADV_DURATION   (3000)
#endif //(BLE_APP_HID)
/// Advertising channel map - 37, 38, 39
#define APP_ADV_CHMAP           (0x07)

#ifdef __AI_VOICE__
bool app_ai_is_ble_adv_uuid_enabled(void);
void bt_drv_restore_ble_control_timing(void);
#endif

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef void (*appm_add_svc_func_t)(void);

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of service to add in the database
enum appm_svc_list
{
    #if (BLE_APP_HT)
    APPM_SVC_HTS,
    #endif //(BLE_APP_HT)
    #if (BLE_APP_DIS)
    APPM_SVC_DIS,
    #endif //(BLE_APP_DIS)
    #if (BLE_APP_BATT)
    APPM_SVC_BATT,
    #endif //(BLE_APP_BATT)
    #if (BLE_APP_HID)
    APPM_SVC_HIDS,
    #endif //(BLE_APP_HID)
    #ifdef BLE_APP_AM0
    APPM_SVC_AM0_HAS,
    #endif //defined(BLE_APP_AM0)
    #if (BLE_APP_HR)
    APPM_SVC_HRP,
    #endif
    #if (BLE_APP_DATAPATH_SERVER)
    APPM_SVC_DATAPATH_SERVER,
    #endif //(BLE_APP_DATAPATH_SERVER)
    #if (BLE_APP_VOICEPATH)
    APPM_SVC_VOICEPATH,
    #ifdef GSOUND_ENABLED
    APPM_SVC_BMS,
    #endif
    #endif //(BLE_APP_VOICEPATH)
    #if (ANCS_PROXY_ENABLE)
    APPM_SVC_ANCSP,
    APPM_SVC_AMSP,
    #endif
    #if (BLE_APP_ANCC)
    APPM_SVC_ANCC,
    #endif //(BLE_APP_ANCC)
    #if (BLE_APP_AMS)
    APPM_SVC_AMSC,
    #endif //(BLE_APP_AMS)
    #if (BLE_APP_OTA)
    APPM_SVC_OTA,
    #endif //(BLE_APP_OTA)
    #if (BLE_APP_GFPS)
    APPM_SVC_GFPS,
    #endif //(BLE_APP_GFPS)
    #if (BLE_AI_VOICE)
    APPM_AI_SMARTVOICE,
    #endif //(BLE_AI_VOICE)
    #if (BLE_APP_TOTA)
    APPM_SVC_TOTA,
    #endif //(BLE_APP_TOTA)

    APPM_SVC_LIST_STOP,
};

/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */
//gattc_msg_handler_tab
//#define KE_MSG_HANDLER_TAB(task)   __STATIC const struct ke_msg_handler task##_msg_handler_tab[] =
/// Application Task Descriptor

//static const struct ke_task_desc TASK_DESC_APP = {&appm_default_handler,
//                                                  appm_state, APPM_STATE_MAX, APP_IDX_MAX};
extern const struct ke_task_desc TASK_DESC_APP;

/// List of functions used to create the database
static const appm_add_svc_func_t appm_add_svc_func_list[APPM_SVC_LIST_STOP] =
{
    #if (BLE_APP_HT)
    (appm_add_svc_func_t)app_ht_add_hts,
    #endif //(BLE_APP_HT)
    #if (BLE_APP_DIS)
    (appm_add_svc_func_t)app_dis_add_dis,
    #endif //(BLE_APP_DIS)
    #if (BLE_APP_BATT)
    (appm_add_svc_func_t)app_batt_add_bas,
    #endif //(BLE_APP_BATT)
    #if (BLE_APP_HID)
    (appm_add_svc_func_t)app_hid_add_hids,
    #endif //(BLE_APP_HID)
    #ifdef BLE_APP_AM0
    (appm_add_svc_func_t)am0_app_add_has,
    #endif //defined(BLE_APP_AM0)
    #if (BLE_APP_HR)
    (appm_add_svc_func_t)app_hrps_add_profile,
    #endif
    #if (BLE_APP_DATAPATH_SERVER)
    (appm_add_svc_func_t)app_datapath_add_datapathps,
    #endif //(BLE_APP_DATAPATH_SERVER)
    #if (BLE_APP_VOICEPATH)
    (appm_add_svc_func_t)app_ble_voicepath_add_svc,
    #ifdef GSOUND_ENABLED
    (appm_add_svc_func_t)app_ble_bms_add_svc,
    #endif
    #endif //(BLE_APP_VOICEPATH)
    #if (ANCS_PROXY_ENABLE)
    (appm_add_svc_func_t)app_ble_ancsp_add_svc,
    (appm_add_svc_func_t)app_ble_amsp_add_svc,
    #endif
    #if (BLE_APP_ANCC)
    (appm_add_svc_func_t)app_ancc_add_ancc,
    #endif
    #if (BLE_APP_AMS)
    (appm_add_svc_func_t)app_amsc_add_amsc,
    #endif
    #if (BLE_APP_OTA)
    (appm_add_svc_func_t)app_ota_add_ota,
    #endif //(BLE_APP_OTA)
    #if (BLE_APP_GFPS)
    (appm_add_svc_func_t)app_gfps_add_gfps,
    #endif
    #if (BLE_APP_AI_VOICE)
    (appm_add_svc_func_t)app_ai_add_ai,
    #endif //(BLE_APP_AI_VOICE)
    #if (BLE_APP_TOTA)
    (appm_add_svc_func_t)app_tota_add_tota,
    #endif //(BLE_APP_TOTA)
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment Structure
struct app_env_tag app_env;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void appm_init()
{
    BLE_APP_FUNC_ENTER();

    // Reset the application manager environment
    memset(&app_env, 0, sizeof(app_env));

    // Create APP task
    ke_task_create(TASK_APP, &TASK_DESC_APP);

    // Initialize Task state
    ke_state_set(TASK_APP, APPM_INIT);

    // Load the device name from NVDS

    // Get the Device Name to add in the Advertising Data (Default one or NVDS one)
#ifdef _BLE_NVDS_
    const char* ble_name_in_nv = nvrec_dev_get_ble_name();
#else
    const char* ble_name_in_nv = BLE_DEFAULT_NAME;
#endif
    uint32_t nameLen = strlen(ble_name_in_nv) + 1;
    if (nameLen > APP_DEVICE_NAME_MAX_LEN)
    {
        nameLen = APP_DEVICE_NAME_MAX_LEN;
    }
    // Get default Device Name (No name if not enough space)
    memcpy(app_env.dev_name, ble_name_in_nv, nameLen);
    app_env.dev_name[nameLen - 1] = '\0';
    app_env.dev_name_len = nameLen;
    TRACE("device ble name:%s", app_env.dev_name);
#ifdef _BLE_NVDS_
    nv_record_blerec_init();

    nv_record_blerec_get_local_irk(app_env.loc_irk);
#else
    uint8_t counter;

    //avoid ble irk collision low probability
    uint32_t generatedSeed = hal_sys_timer_get();
    for (uint8_t index = 0; index < sizeof(bt_addr); index++)
    {
        generatedSeed ^= (((uint32_t)(bt_addr[index])) << (hal_sys_timer_get()&0xF));
    }
    srand(generatedSeed);

    // generate a new IRK
    for (counter = 0; counter < KEY_LEN; counter++)
    {
        app_env.loc_irk[counter]    = (uint8_t)co_rand_word();
    }
#endif

    /*------------------------------------------------------
     * INITIALIZE ALL MODULES
     *------------------------------------------------------*/

    // load device information:

    #if (DISPLAY_SUPPORT)
    app_display_init();
    #endif //(DISPLAY_SUPPORT)

    #if (BLE_APP_SEC)
    // Security Module
    app_sec_init();
    #endif // (BLE_APP_SEC)

    #if (BLE_APP_HT)
    // Health Thermometer Module
    app_ht_init();
    #endif //(BLE_APP_HT)

    #if (BLE_APP_DIS)
    // Device Information Module
    app_dis_init();
    #endif //(BLE_APP_DIS)

    #if (BLE_APP_HID)
    // HID Module
    app_hid_init();
    #endif //(BLE_APP_HID)

    #if (BLE_APP_BATT)
    // Battery Module
    app_batt_init();
    #endif //(BLE_APP_BATT)

    #ifdef BLE_APP_AM0
    // Audio Mode 0 Module
    am0_app_init();
    #endif // defined(BLE_APP_AM0)

    #if (BLE_APP_VOICEPATH)
    // Voice Path Module
    app_ble_voicepath_init();
    #endif //(BLE_APP_VOICEPATH)

    #if (BLE_APP_DATAPATH_SERVER)
    // Data Path Server Module
    app_datapath_server_init();
    #endif //(BLE_APP_DATAPATH_SERVER)
    #if (BLE_APP_AI_VOICE)
    // AI Voice Module
    app_ai_init();
    #endif //(BLE_APP_AI_VOICE)

    #if (BLE_APP_OTA)
    // OTA Module
    app_ota_init();
    #endif //(BLE_APP_OTA)

    #if (BLE_APP_TOTA)
    // TOTA Module
    app_tota_ble_init();
    #endif //(BLE_APP_TOTA)

    #if (BLE_APP_GFPS)
    //
    app_gfps_init();
    #endif

    BLE_APP_FUNC_LEAVE();
}

bool appm_add_svc(void)
{
    // Indicate if more services need to be added in the database
    bool more_svc = false;

    // Check if another should be added in the database
    if (app_env.next_svc != APPM_SVC_LIST_STOP)
    {
        ASSERT_INFO(appm_add_svc_func_list[app_env.next_svc] != NULL, app_env.next_svc, 1);

        BLE_APP_DBG("appm_add_svc adds service");

        // Call the function used to add the required service
        appm_add_svc_func_list[app_env.next_svc]();

        // Select following service to add
        app_env.next_svc++;
        more_svc = true;
    }
    else
    {
        BLE_APP_DBG("appm_add_svc doesn't execute, next svc is %d", app_env.next_svc);
    }


    return more_svc;
}

uint16_t appm_get_conhdl_from_conidx(uint8_t conidx)
{
    return app_env.context[conidx].conhdl;
}

void appm_disconnect_all(void)
{
    for (uint8_t index = 0;index < BLE_CONNECTION_MAX;index++)
    {
        if (app_env.context[index].isConnected)
        {
            appm_disconnect(index);
        }
    }
}

uint8_t appm_is_connected(void)
{
    for (uint8_t index = 0;index < BLE_CONNECTION_MAX;index++)
    {
        if (app_env.context[index].isConnected)
        {
            return 1;
        }
    }
    return 0;
}
void appm_disconnect(uint8_t conidx)
{
    struct gapc_disconnect_cmd *cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                                   KE_BUILD_ID(TASK_GAPC, conidx),
                                                   TASK_APP,
                                                   gapc_disconnect_cmd);

    cmd->operation = GAPC_DISCONNECT;
    cmd->reason    = CO_ERROR_REMOTE_USER_TERM_CON;

    // Send the message
    ke_msg_send(cmd);
}

#ifdef _GFPS_
static uint8_t app_is_in_fastpairing_mode = false;

bool app_is_in_fastparing_mode(void)
{
    return app_is_in_fastpairing_mode;
}

void app_set_in_fastpairing_mode_flag(bool isEnabled)
{
    app_is_in_fastpairing_mode = isEnabled;
}

void app_exit_fastpairing_mode(void)
{
    if (app_is_in_fastparing_mode())
    {
        app_stop_10_second_timer(APP_FASTPAIR_LASTING_TIMER_ID);

        app_set_in_fastpairing_mode_flag(false);

        // reset ble adv
        app_start_ble_advertising(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL);
    }
}

void app_fast_pairing_timeout_timehandler(void)
{
    app_exit_fastpairing_mode();
}

void app_enter_fastpairing_mode(void)
{
    app_set_in_fastpairing_mode_flag(true);

    app_start_ble_advertising(GAPM_ADV_UNDIRECT, BLE_FAST_ADVERTISING_INTERVAL);
    app_start_10_second_timer(APP_FASTPAIR_LASTING_TIMER_ID);
}
#endif

#if defined(__INTERCONNECTION__)
// TODO: implement the confirmation function
static bool is_interconnection_adv_allowed(void)
{
    return true;
}
#endif
#ifdef IS_MULTI_AI_ENABLED
extern volatile uint8_t current_adv_type_temp_selected;
extern osTimerId gva_ama_cross_adv_timer;
#endif

#define BISTO_SERVICE_DATA_LEN      6
#define BISTO_SERVICE_DATA_UUID     0xFE26
#define DEVICE_MODEL_ID             0x00003E
#define FP_DEVICE_MODEL_ID          0x000104

static void appm_fill_ble_adv_scan_rsp_data(
    uint8_t* adv_data, uint8_t* pAdvDataLen,
    uint8_t* scan_rsp_data, uint8_t* pScanRspDataLen)
{
    uint8_t adv_data_len, scan_rsp_data_len;

    uint8_t avail_space;
#ifdef IS_MULTI_AI_ENABLED
    uint8_t ai_current_spec = ai_manager_get_current_spec();
#endif

    /*-----------------------------------------------------------------------------------
        * Set the Advertising Data and the Scan Response Data
        *---------------------------------------------------------------------------------*/
    // Advertising Data
    adv_data_len = 0;

#if defined(__INTERCONNECTION__)
    if(is_interconnection_adv_allowed())
    {
        TRACE("Start interconnection adv.");

        CUSTOMIZED_ADV_DATA_HEAD_T* pCustomizedAdvDataHead = (CUSTOMIZED_ADV_DATA_HEAD_T*)(adv_data);
        pCustomizedAdvDataHead->advType = CUSTOMIZED_ADV_TYPE;
        memcpy(pCustomizedAdvDataHead->uuidValue, UUID_VALUE, UUID_LEN);

        // nearby service
        pCustomizedAdvDataHead->nearbyServiceID = SERVICE_ID_NEARBY;

        // iconnect
        pCustomizedAdvDataHead->iconnectServiceID = SERVICE_ID_ICONNECT;
        if(INTERCONNECTION_DISCOVERABLE_YES == app_get_interconnection_discoverable())
        {
            pCustomizedAdvDataHead->iconnectStatus = RECONNECTABLE_NOT_DISCOVERABLE_YES;
        }
        else
        {
            pCustomizedAdvDataHead->iconnectStatus = RECONNECTABLE_NOT_DISCOVERABLE_NOT;
        }

        // rssi reference
        pCustomizedAdvDataHead->rssiReferenceValueServiceID = SERVICE_ID_RSSI_REFERENCE_VALUE;
        pCustomizedAdvDataHead->rssiReferenceValue = -60;

        // model ID
        pCustomizedAdvDataHead->modelIDServiceID = SERVICE_ID_MODEL_ID;
        memcpy(pCustomizedAdvDataHead->modelID, MODEL_ID, MODEL_ID_LEN);

        // submodel ID
        pCustomizedAdvDataHead->subModelServiceID = SERVICE_ID_SUB_MODEL_ID;
        pCustomizedAdvDataHead->subModelID = SUB_MODEL_ID;

        // update the adv data length
        adv_data_len += FIXED_CUSTOMIZED_ADV_DATA_LEN;

        btif_device_record_t record1;
        btif_device_record_t record2;

        int ret = nv_record_enum_latest_two_paired_dev(&record1, &record2);
        uint8_t extensibleService[EXTENSIBLE_SERVICE_MAX_LEN];

        if(ret == 0)
        {
            //do nothing
        }
        else if(ret==1 || ret==2)
        {
            extensibleService[0] = SERVICE_ID_PAIRED_DEVICE_ID;
            extensibleService[1] = record1.bdAddr.address[1];
            extensibleService[2] = record1.bdAddr.address[0];

            memcpy(&adv_data[adv_data_len], extensibleService, 3);
            adv_data_len += 3;
        }

        // get battery info
        uint8_t batteryInfo;
        app_battery_get_info(NULL, &batteryInfo, NULL);
        adv_data[adv_data_len++] = SERVICE_ID_TOTAL_BATTERY;
        adv_data[adv_data_len++] = batteryInfo;
        pCustomizedAdvDataHead->advLength = adv_data_len - 1;
    }
#endif // #ifdef __INTERCONNECTION__

        //Add list of UUID and appearance
#ifdef _GFPS_
        if (app_is_in_fastparing_mode())
        {
            adv_data[adv_data_len++] = 0x06;
            adv_data[adv_data_len++] = 0x16;
            adv_data[adv_data_len++] = 0x2c;
            adv_data[adv_data_len++] = 0xFE;
            adv_data[adv_data_len++] = (FP_DEVICE_MODEL_ID >> 16) & 0xFF;
            adv_data[adv_data_len++] = (FP_DEVICE_MODEL_ID >> 8) & 0xFF;
            adv_data[adv_data_len++] = (FP_DEVICE_MODEL_ID >> 0) & 0xFF;

            memcpy(&adv_data[adv_data_len],
                    APP_GFPS_ADV_POWER_UUID, APP_GFPS_ADV_POWER_UUID_LEN);
            adv_data_len += APP_GFPS_ADV_POWER_UUID_LEN;
        }
        else
        {
#if BLE_APP_GFPS_VER==FAST_PAIR_REV_2_0
            uint8_t serviceData[32];
            serviceData[0] = 0x03;
            serviceData[1] = 0x16;
            serviceData[2] = 0x2C;
            serviceData[3] = 0xFE;
            uint8_t dataLen = app_gfps_generate_accountkey_data(&serviceData[4]);
            serviceData[0] += dataLen;
            memcpy(&adv_data[adv_data_len],
                serviceData, serviceData[0]+1);
            adv_data_len += (serviceData[0]+1);

            memcpy(&adv_data[adv_data_len],
                    APP_GFPS_ADV_POWER_UUID, APP_GFPS_ADV_POWER_UUID_LEN);
            adv_data_len += APP_GFPS_ADV_POWER_UUID_LEN;
#endif
        }
#endif // _GFPS_

        // Scan Response Data
        scan_rsp_data_len = 0;
#ifdef _GFPS_
        if (!app_is_in_fastparing_mode())
#endif
        {
#ifdef GSOUND_ENABLED
#ifdef IS_MULTI_AI_ENABLED
            if ((AI_SPEC_GSOUND == ai_current_spec && ai_manager_get_spec_connected_status(AI_SPEC_GSOUND) == 0) || \
                        (AI_SPEC_AMA == ai_current_spec && ai_manager_get_spec_connected_status(AI_SPEC_AMA) == 1))
#endif
            {
                TRACE("bisto bisto bisto bisto bisto bisto....\n");
                scan_rsp_data[scan_rsp_data_len++] = BISTO_SERVICE_DATA_LEN;
                scan_rsp_data[scan_rsp_data_len++] = 0x16; /* Service Data */
                scan_rsp_data[scan_rsp_data_len++] = (BISTO_SERVICE_DATA_UUID >> 0) & 0xFF;
                scan_rsp_data[scan_rsp_data_len++] = (BISTO_SERVICE_DATA_UUID >> 8) & 0xFF;
                scan_rsp_data[scan_rsp_data_len++] = (DEVICE_MODEL_ID >> 0) & 0xFF;
                scan_rsp_data[scan_rsp_data_len++] = (DEVICE_MODEL_ID >> 8) & 0xFF;
                scan_rsp_data[scan_rsp_data_len++] = (DEVICE_MODEL_ID >> 16) & 0xFF;
            }
#endif

#ifdef __AMA_VOICE__
#ifdef IS_MULTI_AI_ENABLED
            if ((AI_SPEC_AMA == ai_current_spec && ai_manager_get_spec_connected_status(AI_SPEC_AMA) == 0) || \
                        (AI_SPEC_GSOUND == ai_current_spec && ai_manager_get_spec_connected_status(AI_SPEC_GSOUND) == 1))
#endif
            {
                adv_data_len = 0;
                TRACE("ama ama ama ama ama ama....\n");
                if (app_ai_is_ble_adv_uuid_enabled()) {
                    memcpy(&adv_data[adv_data_len],APP_AMA_UUID, APP_AMA_UUID_LEN);
                    adv_data_len += APP_AMA_UUID_LEN;
                }
                // Get remaining space in the Advertising Data - 2 bytes are used for name length/flag
                avail_space = BLE_DATA_LEN - adv_data_len - 2;
                // Check if data can be added to the adv Data
                if (avail_space > 2) {
                    avail_space = co_min(avail_space, app_env.dev_name_len);
                    adv_data[adv_data_len]     = avail_space + 1;
                    // Fill Device Name Flag
                    adv_data[adv_data_len + 1] = (avail_space == app_env.dev_name_len) ? '\x08' : '\x09';
                    // Copy device name
                    memcpy(&adv_data[adv_data_len + 2], app_env.dev_name, avail_space);
                    // Update adv Data Length
                    adv_data_len += (avail_space + 2);
                }
            }
#endif

#ifdef IS_MULTI_AI_ENABLED
            if (AI_SPEC_INIT == ai_current_spec) {
                TRACE("server init status, cross adv bisto and ama....\n");
                if (current_adv_type_temp_selected == AI_SPEC_GSOUND) {
                    TRACE("bisto bisto bisto bisto bisto bisto....\n");
                    scan_rsp_data[scan_rsp_data_len++] = BISTO_SERVICE_DATA_LEN;
                    scan_rsp_data[scan_rsp_data_len++] = 0x16; /* Service Data */
                    scan_rsp_data[scan_rsp_data_len++] = (BISTO_SERVICE_DATA_UUID >> 0) & 0xFF;
                    scan_rsp_data[scan_rsp_data_len++] = (BISTO_SERVICE_DATA_UUID >> 8) & 0xFF;
                    scan_rsp_data[scan_rsp_data_len++] = (DEVICE_MODEL_ID >> 0) & 0xFF;
                    scan_rsp_data[scan_rsp_data_len++] = (DEVICE_MODEL_ID >> 8) & 0xFF;
                    scan_rsp_data[scan_rsp_data_len++] = (DEVICE_MODEL_ID >> 16) & 0xFF;
                } else if (current_adv_type_temp_selected == AI_SPEC_AMA) {
                    adv_data_len = 0;
                    TRACE("ama ama ama ama ama ama....\n");
                    if (app_ai_is_ble_adv_uuid_enabled()) {
                        memcpy(&adv_data[adv_data_len],APP_AMA_UUID, APP_AMA_UUID_LEN);
                        adv_data_len += APP_AMA_UUID_LEN;
                    }

                    // Get remaining space in the Advertising Data - 2 bytes are used for name length/flag
                    avail_space = BLE_DATA_LEN - adv_data_len - 2;
                    // Check if data can be added to the adv Data
                    if (avail_space > 2) {
                        avail_space = co_min(avail_space, app_env.dev_name_len);
                        adv_data[adv_data_len]     = avail_space + 1;
                        // Fill Device Name Flag
                        adv_data[adv_data_len + 1] = (avail_space == app_env.dev_name_len) ? '\x08' : '\x09';
                        // Copy device name
                        memcpy(&adv_data[adv_data_len + 2], app_env.dev_name, avail_space);
                        // Update adv Data Length
                        adv_data_len += (avail_space + 2);
                    }
                }
                osTimerStop(gva_ama_cross_adv_timer);
                osTimerStart(gva_ama_cross_adv_timer, GVA_AMA_CROSS_ADV_TIME_INTERVAL);
            }
#endif
        }

        if (scan_rsp_data_len > 0)
        {
            avail_space = (SCAN_RSP_DATA_LEN) - scan_rsp_data_len - 2;
            // Check if data can be added to the Scan response Data
            if (avail_space > 2)
            {
                avail_space = co_min(avail_space, app_env.dev_name_len);

                scan_rsp_data[scan_rsp_data_len]     = avail_space + 1;
                // Fill Device Name Flag
                scan_rsp_data[scan_rsp_data_len + 1] = (avail_space == app_env.dev_name_len) ? '\x08' : '\x09';
                // Copy device name
                memcpy(&scan_rsp_data[scan_rsp_data_len + 2], app_env.dev_name, avail_space);
                // Update Scan response Data Length
                scan_rsp_data_len += (avail_space + 2);
            }
        }

        if (0 == adv_data_len)
        {
            // Get remaining space in the Advertising Data - 2 bytes are used for name length/flag
            avail_space = BLE_DATA_LEN - adv_data_len - 2;
            // Check if data can be added to the adv Data
            if (avail_space > 2)
            {
                avail_space = co_min(avail_space, app_env.dev_name_len);
                adv_data[adv_data_len]     = avail_space + 1;
                // Fill Device Name Flag
                adv_data[adv_data_len + 1] = (avail_space == app_env.dev_name_len) ? '\x08' : '\x09';
                // Copy device name
                memcpy(&adv_data[adv_data_len + 2], app_env.dev_name, avail_space);
                // Update adv Data Length
                adv_data_len += (avail_space + 2);
            }
        }

        ASSERT(adv_data_len <= ADV_DATA_MAX_LEN, "adv data overflow, please check.");
        TRACE("Adv data: len = %d", adv_data_len);
        DUMP8("%02x ", adv_data, adv_data_len);

        TRACE("Scan rsp data: len =%d", scan_rsp_data_len);
        DUMP8("%02x ", scan_rsp_data, scan_rsp_data_len);

        *pAdvDataLen = adv_data_len;
        *pScanRspDataLen = scan_rsp_data_len;

}

#if (BLE_APP_GFPS)
#define UPDATE_BLE_ADV_DATA_RIGHT_BEFORE_STARTING_ADV_ENABLED   1
#else
#define UPDATE_BLE_ADV_DATA_RIGHT_BEFORE_STARTING_ADV_ENABLED   0
#endif

void gapm_update_ble_adv_data_right_before_started(uint8_t* pAdvData, uint8_t* advDataLen,
    uint8_t* pScanRspData, uint8_t* scanRspDataLen)
{
    // for the user case that the BLE adv data or scan rsp data need to be generated from the
    // run-time BLE address.
    // Just enable macro UPDATE_BLE_ADV_DATA_RIGHT_BEFORE_STARTING_ADV_ENABLED,
    // and call appm_get_current_ble_addr to get the run-time used BLE address
    // For fastpair GFPS_ACCOUNTKEY_SALT_TYPE==USE_BLE_ADDR_AS_SALT case,
    // this macro must be enabled

#if UPDATE_BLE_ADV_DATA_RIGHT_BEFORE_STARTING_ADV_ENABLED
    appm_fill_ble_adv_scan_rsp_data(pAdvData, advDataLen, pScanRspData, scanRspDataLen);
#endif
}

/**
 ****************************************************************************************
 * Advertising Functions
 ****************************************************************************************
 */
void appm_start_advertising(uint8_t advType,
    uint32_t advMaxIntervalInMs, uint32_t advMinIntervalInMs)
{
    BLE_APP_FUNC_ENTER();

    TRACE("%s state: %d", __func__, ke_state_get(TASK_APP));

    // Prepare the GAPM_START_ADVERTISE_CMD message
    struct gapm_start_advertise_cmd *cmd = KE_MSG_ALLOC(GAPM_START_ADVERTISE_CMD,
                                                        TASK_GAPM, TASK_APP,
                                                        gapm_start_advertise_cmd);
#if BLE_IS_USE_RPA
    cmd->op.addr_src    = GAPM_GEN_RSLV_ADDR;
    cmd->op.randomAddrRenewIntervalInSecond = (uint16_t)(60*15);
#else
    cmd->op.addr_src    = GAPM_STATIC_ADDR;

    // To use non-resolvable address
    //cmd->op.addr_src    = GAPM_GEN_NON_RSLV_ADDR;
    //cmd->op.randomAddrRenewIntervalInSecond = (uint16_t)(60*15);

#endif

    cmd->channel_map    = APP_ADV_CHMAP;

    cmd->intv_min = advMinIntervalInMs*8/5;
    cmd->intv_max = advMaxIntervalInMs*8/5;

    cmd->isIncludedFlags = true;

    cmd->op.code        = advType;
    cmd->info.host.mode = GAP_GEN_DISCOVERABLE;

    cmd->info.host.flags = GAP_LE_GEN_DISCOVERABLE_FLG | GAP_BR_EDR_NOT_SUPPORTED;

#ifdef _GFPS_
    if (app_is_in_fastparing_mode())
    {

        cmd->intv_min = BLE_FASTPAIR_FAST_ADVERTISING_INTERVAL;
        cmd->intv_max = BLE_FASTPAIR_FAST_ADVERTISING_INTERVAL;
    }
    else
    {
        cmd->intv_min = BLE_FASTPAIR_NORMAL_ADVERTISING_INTERVAL;
        cmd->intv_max = BLE_FASTPAIR_NORMAL_ADVERTISING_INTERVAL;
    }
#endif // _GFPS_

    appm_fill_ble_adv_scan_rsp_data(
        cmd->info.host.adv_data,
        &(cmd->info.host.adv_data_len),
        cmd->info.host.scan_rsp_data,
        &(cmd->info.host.scan_rsp_data_len));

#if defined (__SMART_VOICE__) || defined (__DMA_VOICE__) || defined (__TENCENT_VOICE__)
    ai_function_handle(API_START_ADV, cmd, 0);
#endif

    // Send the message
    ke_msg_send(cmd);

    // Set the state of the task to APPM_ADVERTISING
    ke_state_set(TASK_APP, APPM_ADVERTISING);

    BLE_APP_FUNC_LEAVE();
}


void appm_stop_advertising(void)
{

        #if (BLE_APP_HID)
        // Stop the advertising timer if needed
        if (ke_timer_active(APP_ADV_TIMEOUT_TIMER, TASK_APP))
        {
            ke_timer_clear(APP_ADV_TIMEOUT_TIMER, TASK_APP);
        }
        #endif //(BLE_APP_HID)

        // Go in ready state
        ke_state_set(TASK_APP, APPM_READY);

        // Prepare the GAPM_CANCEL_CMD message
        struct gapm_cancel_cmd *cmd = KE_MSG_ALLOC(GAPM_CANCEL_CMD,
                                                   TASK_GAPM, TASK_APP,
                                                   gapm_cancel_cmd);

        cmd->operation = GAPM_CANCEL;

        // Send the message
        ke_msg_send(cmd);

        #if (DISPLAY_SUPPORT)
        // Update advertising state screen
        app_display_set_adv(false);
        #endif //(DISPLAY_SUPPORT)

}
void appm_start_scanning(uint16_t intervalInMs, uint16_t windowInMs, uint32_t filtPolicy)
{
    struct gapm_start_scan_cmd* cmd = KE_MSG_ALLOC(GAPM_START_SCAN_CMD,
                                                                 TASK_GAPM, TASK_APP,
                                                                 gapm_start_scan_cmd);


    cmd->op.code = GAPM_SCAN_PASSIVE;
    cmd->op.addr_src = GAPM_STATIC_ADDR;

    cmd->interval = intervalInMs*1000/625;
    cmd->window = windowInMs*1000/625;
    cmd->mode = GAP_OBSERVER_MODE; //GAP_GEN_DISCOVERY;
    cmd->filt_policy = filtPolicy;
    cmd->filter_duplic = SCAN_FILT_DUPLIC_DIS;

    ke_state_set(TASK_APP, APPM_SCANNING);

    ke_msg_send(cmd);
}

void appm_stop_scanning(void)
{
    //if (ke_state_get(TASK_APP) == APPM_SCANNING)
    {
        // Go in ready state
        ke_state_set(TASK_APP, APPM_READY);

        // Prepare the GAPM_CANCEL_CMD message
        struct gapm_cancel_cmd *cmd = KE_MSG_ALLOC(GAPM_CANCEL_CMD,
                                                   TASK_GAPM, TASK_APP,
                                                   gapm_cancel_cmd);

        cmd->operation = GAPM_CANCEL;

        // Send the message
        ke_msg_send(cmd);
    }
}

void appm_add_dev_into_whitelist(struct gap_bdaddr* ptBdAddr)
{
    struct gapm_white_list_mgt_cmd* cmd = KE_MSG_ALLOC_DYN(GAPM_WHITE_LIST_MGT_CMD,
                                                                 TASK_GAPM, TASK_APP,
                                                                 gapm_white_list_mgt_cmd, sizeof(struct gap_bdaddr));
    cmd->operation = GAPM_ADD_DEV_IN_WLIST;
    cmd->nb = 1;

    memcpy(cmd->devices, ptBdAddr, sizeof(struct gap_bdaddr));

    ke_msg_send(cmd);
}

void appm_start_connecting(struct gap_bdaddr* ptBdAddr)
{
    struct gapm_start_connection_cmd* cmd = KE_MSG_ALLOC_DYN(GAPM_START_CONNECTION_CMD,
                                                                 TASK_GAPM, TASK_APP,
                                                                 gapm_start_connection_cmd, sizeof(struct gap_bdaddr));
    cmd->ce_len_max = 200;
    cmd->ce_len_min = 100;
    cmd->con_intv_max = 60; // in the unit of 1.25ms
    cmd->con_intv_min = 45; // in the unit of 1.25ms
    cmd->con_latency = 0;
    cmd->superv_to = 1000;  // in the unit of 10ms
    cmd->op.code = GAPM_CONNECTION_DIRECT;
    cmd->op.addr_src = GAPM_STATIC_ADDR;
    cmd->nb_peers = 1;
    cmd->scan_interval = ((600) * 1000 / 625);
    cmd->scan_window = ((300) * 1000 / 625);

    memcpy(cmd->peers, ptBdAddr, sizeof(struct gap_bdaddr));

    ke_state_set(TASK_APP, APPM_CONNECTING);

    ke_msg_send(cmd);
}

void appm_stop_connecting(void)
{
    if (ke_state_get(TASK_APP) == APPM_CONNECTING)
    {
        // Go in ready state
        ke_state_set(TASK_APP, APPM_READY);

        // Prepare the GAPM_CANCEL_CMD message
        struct gapm_cancel_cmd *cmd = KE_MSG_ALLOC(GAPM_CANCEL_CMD,
                                                   TASK_GAPM, TASK_APP,
                                                   gapm_cancel_cmd);

        cmd->operation = GAPM_CANCEL;

        // Send the message
        ke_msg_send(cmd);
    }
}


void appm_update_param(uint8_t conidx, struct gapc_conn_param *conn_param)
{
    // Prepare the GAPC_PARAM_UPDATE_CMD message
    struct gapc_param_update_cmd *cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD,
                                                     KE_BUILD_ID(TASK_GAPC, conidx), TASK_APP,
                                                     gapc_param_update_cmd);

    cmd->operation  = GAPC_UPDATE_PARAMS;
    cmd->intv_min   = conn_param->intv_min;
    cmd->intv_max   = conn_param->intv_max;
    cmd->latency    = conn_param->latency;
    cmd->time_out   = conn_param->time_out;

    // not used by a slave device
    cmd->ce_len_min = 0xFFFF;
    cmd->ce_len_max = 0xFFFF;

    // Send the message
    ke_msg_send(cmd);
}

void l2cap_update_param(uint8_t conidx, struct gapc_conn_param *conn_param)
{
    struct l2cc_update_param_req *req = L2CC_SIG_PDU_ALLOC(conidx, L2C_CODE_CONN_PARAM_UPD_REQ,
                                                   KE_BUILD_ID(TASK_GAPC, conidx), l2cc_update_param_req);

    // generate packet identifier
    uint8_t pkt_id = co_rand_word() & 0xFF;
    if  (pkt_id == 0)
    {
        pkt_id = 1;
    }

    /* fill up the parameters */
    req->intv_max = conn_param->intv_max;
    req->intv_min = conn_param->intv_min;
    req->latency  = conn_param->latency;
    req->timeout  = conn_param->time_out;
    req->pkt_id   = pkt_id;
    TRACE("%s val: %x %x %x %x %x",__func__,req->intv_max,req->intv_min,req->latency,req->timeout,req->pkt_id);
    l2cc_pdu_send(req);
}

uint8_t appm_get_dev_name(uint8_t* name)
{
    // copy name to provided pointer
    memcpy(name, app_env.dev_name, app_env.dev_name_len);
    // return name length
    return app_env.dev_name_len;
}

void appm_exchange_mtu(uint8_t conidx)
{
    struct gattc_exc_mtu_cmd *cmd = KE_MSG_ALLOC(GATTC_EXC_MTU_CMD,
                                                     KE_BUILD_ID(TASK_GATTC, conidx), TASK_APP,
                                                     gattc_exc_mtu_cmd);

    cmd->operation = GATTC_MTU_EXCH;
    cmd->seq_num= 0;

    ke_msg_send(cmd);
}

void appm_check_and_resolve_ble_address(uint8_t conidx)
{
    APP_BLE_CONN_CONTEXT_T* pContext = &(app_env.context[conidx]);

    // solved already, return
    if (pContext->isGotSolvedBdAddr)
    {
        TRACE("Already get solved bd addr.");
        return;
    }
    // not solved yet and the solving is in progress, return and wait
    else if (app_is_resolving_ble_bd_addr())
    {
        TRACE("Random bd addr solving on going.");
        return;
    }

    if (BLE_RANDOM_ADDR == pContext->peerAddrType)
    {
        memset(pContext->solvedBdAddr, 0, BD_ADDR_LEN);
        bool isSuccessful = appm_resolve_random_ble_addr_from_nv(conidx, pContext->bdAddr);
        TRACE("%s isSuccessful %d", __func__, isSuccessful);
        if (isSuccessful)
        {
            pContext->isBdAddrResolvingInProgress = true;
        }
        else
        {
            pContext->isGotSolvedBdAddr = true;
            pContext->isBdAddrResolvingInProgress = false;
        }
    }
    else
    {
        pContext->isGotSolvedBdAddr = true;
        pContext->isBdAddrResolvingInProgress = false;
        memcpy(pContext->solvedBdAddr, pContext->bdAddr, BD_ADDR_LEN);
    }

}

bool appm_resolve_random_ble_addr_from_nv(uint8_t conidx, uint8_t* randomAddr)
{
#ifdef _BLE_NVDS_
    struct gapm_resolv_addr_cmd *cmd = KE_MSG_ALLOC_DYN(GAPM_RESOLV_ADDR_CMD,
                                       KE_BUILD_ID(TASK_GAPM, conidx), TASK_APP,
                                       gapm_resolv_addr_cmd,
                                       BLE_RECORD_NUM*GAP_KEY_LEN);
    uint8_t irkeyNum = nv_record_ble_fill_irk((uint8_t *)(cmd->irk));
    if (0 == irkeyNum)
    {
        TRACE("No history irk, cannot solve bd addr.");
        KE_MSG_FREE(cmd);
        return false;
    }

    TRACE("Start random bd addr solving.");

    cmd->operation = GAPM_RESOLV_ADDR;
    cmd->nb_key = irkeyNum;
    memcpy(cmd->addr.addr, randomAddr, GAP_BD_ADDR_LEN);
    ke_msg_send(cmd);
    return true;
#else
    return false;
#endif

}

void appm_resolve_random_ble_addr_with_sepcific_irk(uint8_t conidx, uint8_t* randomAddr, uint8_t* pIrk)
{
    struct gapm_resolv_addr_cmd *cmd = KE_MSG_ALLOC_DYN(GAPM_RESOLV_ADDR_CMD,
                                       KE_BUILD_ID(TASK_GAPM, conidx), TASK_APP,
                                       gapm_resolv_addr_cmd,
                                       GAP_KEY_LEN);
    cmd->operation = GAPM_RESOLV_ADDR;
    cmd->nb_key = 1;
    memcpy(cmd->addr.addr, randomAddr, GAP_BD_ADDR_LEN);
    memcpy(cmd->irk, pIrk, GAP_KEY_LEN);
    ke_msg_send(cmd);
}

void appm_random_ble_addr_solved(bool isSolvedSuccessfully, uint8_t* irkUsedForSolving)
{
    APP_BLE_CONN_CONTEXT_T* pContext;
    uint32_t conidx;
    for (conidx = 0;conidx < BLE_CONNECTION_MAX;conidx++)
    {
        pContext = &(app_env.context[conidx]);
        if (pContext->isBdAddrResolvingInProgress)
        {
            break;
        }
    }

    if (conidx < BLE_CONNECTION_MAX)
    {
        pContext->isBdAddrResolvingInProgress = false;
        pContext->isGotSolvedBdAddr = true;

        TRACE("%s conidx %d isSolvedSuccessfully %d", __func__, conidx, isSolvedSuccessfully);
        if (isSolvedSuccessfully)
        {
#ifdef _BLE_NVDS_
            bool isSuccessful = nv_record_blerec_get_bd_addr_from_irk(app_env.context[conidx].solvedBdAddr, irkUsedForSolving);
            if (isSuccessful)
            {
                TRACE("Connected random address's original addr is:");
                DUMP8("%02x ", app_env.context[conidx].solvedBdAddr, GAP_BD_ADDR_LEN);
            }
            else
#endif
            {
                TRACE("Resolving of the connected BLE random addr failed.");
            }
        }
        else
        {
            TRACE("BLE random resolving failed.");
        }
    }

#if defined(CFG_VOICEPATH)
    ke_task_msg_retrieve(prf_get_task_from_id(TASK_ID_VOICEPATH));
#endif
    ke_task_msg_retrieve(TASK_GAPC);
    ke_task_msg_retrieve(TASK_APP);

    app_start_ble_advertising(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL);
}

uint8_t app_ble_connection_count(void)
{
    return app_env.conn_cnt;
}

bool app_is_arrive_at_max_ble_connections(void)
{
    TRACE("ble connection count %d", app_env.conn_cnt);
#if defined(_GFPS_) && (BLE_APP_GFPS_VER==FAST_PAIR_REV_2_0)
    return (app_env.conn_cnt >= BLE_CONNECTION_MAX);
#else
    return (app_env.conn_cnt >= 1);
#endif
}

bool app_is_resolving_ble_bd_addr(void)
{
    for (uint32_t index = 0;index < BLE_CONNECTION_MAX;index++)
    {
        if (app_env.context[index].isBdAddrResolvingInProgress)
        {
            return true;
        }
    }

    return false;
}

void app_trigger_ble_service_discovery(uint8_t conidx, uint16_t shl, uint16_t ehl)
{
    struct gattc_send_svc_changed_cmd *cmd= KE_MSG_ALLOC(GATTC_SEND_SVC_CHANGED_CMD,\
                                                            KE_BUILD_ID(TASK_GATTC, conidx),\
                                                            TASK_APP,\
                                                            gattc_send_svc_changed_cmd);
    cmd->operation = GATTC_SVC_CHANGED;
    cmd->svc_shdl = shl;
    cmd->svc_ehdl = ehl;
    ke_msg_send(cmd);
}

extern void OS_NotifyEvm(void);

void app_stop_ble_advertising(void)
{
    appm_stop_advertising();

    OS_NotifyEvm();
}

__attribute__((weak)) void app_connected_evt_handler(const uint8_t* pPeerBdAddress)
{

}

__attribute__((weak)) void app_disconnected_evt_handler(void)
{

}

__attribute__((weak)) void app_advertising_stopped(void)
{

}

__attribute__((weak)) void app_advertising_started(void)
{

}

__attribute__((weak)) void app_connecting_started(void)
{

}

__attribute__((weak)) void app_scanning_stopped(void)
{

}

__attribute__((weak)) void app_connecting_stopped(void)
{

}


__attribute__((weak)) void app_scanning_started(void)
{

}

__attribute__((weak)) void app_ble_system_ready(void)
{

}

__attribute__((weak)) void app_adv_reported_scanned(struct gapm_adv_report_ind* ptInd)
{

}

uint8_t* appm_get_current_ble_addr(void)
{
#if BLE_IS_USE_RPA
    return (uint8_t *)gapm_get_bdaddr();
#else
    return ble_addr;
#endif
}

uint8_t* appm_get_local_identity_ble_addr(void)
{
    return ble_addr;
}

// bit mask of the existing conn param modes
static uint32_t existingBleConnParamModes[BLE_CONNECTION_MAX] = {0};

static BLE_CONN_PARAM_CONFIG_T ble_conn_param_config[] =
{
    // default value: for the case of BLE just connected and the BT idle state
    {BLE_CONN_PARAM_MODE_DEFAULT, BLE_CONN_PARAM_PRIORITY_NORMAL, 24},

    {BLE_CONN_PARAM_MODE_A2DP_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL, 36},

    {BLE_CONN_PARAM_MODE_HFP_ON, BLE_CONN_PARAM_PRIORITY_ABOVE_NORMAL, 36},

    {BLE_CONN_PARAM_MODE_OTA, BLE_CONN_PARAM_PRIORITY_HIGH, 12},

    // TODO: add mode cases if needed
};

void app_ble_reset_conn_param_mode(uint8_t conidx)
{
    uint32_t lock = int_lock_global();
    existingBleConnParamModes[conidx] = 0;
    int_unlock_global(lock);
}

void app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_E mode, bool isEnable)
{
    for (uint8_t index = 0;index < BLE_CONNECTION_MAX;index++)
    {        
        if (app_env.context[index].isConnected)
        {
            app_ble_update_conn_param_mode_of_specific_connection(
                index, mode, isEnable);
        }
    }
}

void app_ble_update_conn_param_mode_of_specific_connection(uint8_t conidx, BLE_CONN_PARAM_MODE_E mode, bool isEnable)
{
    ASSERT(mode < BLE_CONN_PARAM_MODE_NUM, "Wrong ble conn param mode %d!", mode);

    uint32_t lock = int_lock_global();

    // locate the conn param mode
    BLE_CONN_PARAM_CONFIG_T* pConfig = NULL;
    uint8_t index;
    for (index = 0;index < 
        sizeof(ble_conn_param_config)/sizeof(BLE_CONN_PARAM_CONFIG_T);
        index++)
    {
        if (mode == ble_conn_param_config[index].ble_conn_param_mode)
        {
            pConfig = &ble_conn_param_config[index];
            break;
        }
    }
    
    if (NULL == pConfig)
    {
        int_unlock_global(lock);
        TRACE("BLE conn param mode %d not defined!", mode);        
        return;
    }
    
    if (isEnable)
    {
        if (0 == existingBleConnParamModes[conidx])
        {
            // no other params existing, just configure this one
            existingBleConnParamModes[conidx] = 1 << mode;
        }
        else
        {
            // already existing, directly return
            if (existingBleConnParamModes[conidx]&(1 << mode))
            {
                int_unlock_global(lock);
                return;
            }
            else
            {
                // update the bit-mask 
                existingBleConnParamModes[conidx] |= (1 << mode);
                
                // not existing yet, need to go throuth the existing params to see whether
                // we need to update the param
                for (index = 0;index < 
                    sizeof(ble_conn_param_config)/sizeof(BLE_CONN_PARAM_CONFIG_T);
                    index++)
                {
                    if (((uint32_t)1 << (uint8_t)ble_conn_param_config[index].ble_conn_param_mode)&existingBleConnParamModes[conidx])
                    {
                        if (ble_conn_param_config[index].priority > pConfig->priority)
                        {
                            // one of the exiting param has higher priority than this one,
                            // so do nothing but update the bit-mask                            
                            int_unlock_global(lock);
                            return;
                        }
                    }
                }

                // no higher priority conn param existing, so we need to apply this one                
            }
        }
    }
    else
    {
        if (0 == existingBleConnParamModes[conidx])
        {
            // no other params existing, just return
            int_unlock_global(lock);
            return;
        }
        else 
        {
            // doesn't exist, directly return
            if (!(existingBleConnParamModes[conidx]&(1 << mode)))
            {
                int_unlock_global(lock);
                return;
            }
            else
            {
                // update the bit-mask 
                existingBleConnParamModes[conidx] &= (~(1 << mode));

                if (0 == existingBleConnParamModes[conidx])
                {
                    int_unlock_global(lock);
                    return;
                }

                pConfig = NULL;
                
                // existing, need to apply for the highest priority conn param
                for (index = 0;index < 
                    sizeof(ble_conn_param_config)/sizeof(BLE_CONN_PARAM_CONFIG_T);
                    index++)
                {                    
                    if (((uint32_t)1 << (uint8_t)ble_conn_param_config[index].ble_conn_param_mode)&existingBleConnParamModes[conidx])
                    {
                        if (NULL != pConfig)
                        {
                            if (ble_conn_param_config[index].priority > pConfig->priority)
                            {
                                pConfig = &ble_conn_param_config[index];
                            }
                        }
                        else
                        {
                            pConfig = &ble_conn_param_config[index];
                        }
                    }
                }
            }
        }
    }

    int_unlock_global(lock);

    // if we can arrive here, it means we have got one config to apply
    ASSERT(NULL != pConfig, "It's strange that config pointer is still NULL.");
    
    struct gapc_conn_param conn_param;
    uint16_t conn_interval = 
        pConfig->conn_param_interval;
    
    conn_param.intv_min = conn_interval;
    conn_param.intv_max = conn_interval;
    conn_param.latency  = BLE_CONN_PARAM_SLAVE_LATENCY_CNT;
    conn_param.time_out = BLE_CONN_PARAM_SUPERVISE_TIMEOUT_MS / 10;
    l2cap_update_param(conidx, &conn_param);

    TRACE("BLE conn param mode switched to %d:", mode);
    TRACE("Conn interval %d", pConfig->conn_param_interval);

}


#endif //(BLE_APP_PRESENT)
/// @} APP
