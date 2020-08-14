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

#include "hal_aud.h"
#include "hal_chipid.h"
#include "hal_trace.h"
#include "apps.h"
#include "app_thread.h"
#include "app_status_ind.h"

#include "bluetooth.h"
#include "nvrecord.h"
#include "besbt.h"
#include "besbt_cfg.h"
#include "me_api.h"
#include "mei_api.h"
#include "a2dp_api.h"
#include "hci_api.h"
#include "l2cap_api.h"
#include "hfp_api.h"
#include "dip_api.h"

#include "btapp.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "app_bt_func.h"
#include "bt_drv_interface.h"
#include "os_api.h"
#include "bt_drv_reg_op.h"
#if defined(IBRT)
#include "app_tws_ibrt.h"
#include "app_ibrt_ui.h"
#include "app_ibrt_if.h"
#endif
#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#ifdef VOICE_DATAPATH
#include "app_voicepath.h"
#endif
#include "app_ble_mode_switch.h"
#include "app_gfps.h"

#ifdef __AI_VOICE__
#include "ai_manager.h"
#include "ai_spp.h"
#include "ai_thread.h"

extern void app_ai_disconnect_ble();
#endif

#ifdef __INTERCONNECTION__
#include "app_interconnection.h"
#endif

#if defined(__EARPHONE_STAY_BCR_SLAVE__) && defined(__BT_ONE_BRING_TWO__)
#error can not defined at the same time.
#endif

bool a2dp_is_music_ongoing(void);

#ifdef _GFPS_    
extern "C" void app_enter_fastpairing_mode();
#endif

extern struct BT_DEVICE_T  app_bt_device;
extern "C" bool app_anc_work_status(void);
extern "C" void app_notify_stack_ready();

static btif_remote_device_t *connectedMobile = NULL;

U16 bt_accessory_feature_feature = BTIF_HF_CUSTOM_FEATURE_SUPPORT;

#ifdef __BT_ONE_BRING_TWO__
btif_device_record_t record2_copy;
uint8_t record2_avalible;

#endif

enum bt_profile_reconnect_mode{
    bt_profile_reconnect_null,
    bt_profile_reconnect_openreconnecting,
    bt_profile_reconnect_reconnecting,
    bt_profile_reconnect_reconnect_pending,
};

enum bt_profile_connect_status{
    bt_profile_connect_status_unknow,
    bt_profile_connect_status_success,
    bt_profile_connect_status_failure,
};

struct app_bt_profile_manager{
    bool has_connected;
    enum bt_profile_connect_status hfp_connect;
    enum bt_profile_connect_status hsp_connect;
    enum bt_profile_connect_status a2dp_connect;
    bt_bdaddr_t rmt_addr;
    bt_profile_reconnect_mode reconnect_mode;
    bt_profile_reconnect_mode saved_reconnect_mode;
    a2dp_stream_t *stream;
    //HfChannel *chan;
    hf_chan_handle_t chan;
#if defined (__HSP_ENABLE__)
    HsChannel * hs_chan;
#endif
    uint16_t reconnect_cnt;
    osTimerId connect_timer;
    void (* connect_timer_cb)(void const *);

    APP_BT_CONNECTING_STATE_E connectingState;
};

//reconnect = (INTERVAL+PAGETO)*CNT = (3000ms+5000ms)*15 = 120s
#define APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS (3000)
#define APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT   (2)
#define APP_BT_PROFILE_RECONNECT_RETRY_LIMIT_CNT (15)
#define APP_BT_PROFILE_CONNECT_RETRY_MS (10000)

static struct app_bt_profile_manager bt_profile_manager[BT_DEVICE_NUM];

static int8_t app_bt_profile_reconnect_pending_process(void);
void app_bt_connectable_mode_stop_reconnecting(void);

btif_accessible_mode_t g_bt_access_mode = BTIF_BAM_NOT_ACCESSIBLE;

#define APP_BT_PROFILE_BOTH_SCAN_MS (11000)
#define APP_BT_PROFILE_PAGE_SCAN_MS (4000)

osTimerId app_bt_accessmode_timer = NULL;
btif_accessible_mode_t app_bt_accessmode_timer_argument = BTIF_BAM_NOT_ACCESSIBLE;
static int app_bt_accessmode_timehandler(void const *param);
osTimerDef (APP_BT_ACCESSMODE_TIMER, (void (*)(void const *))app_bt_accessmode_timehandler);                      // define timers

static void app_delay_connect_a2dp_callback(void const *param);
static void app_delay_connect_hfp_callback(void const *param);

#define CONNECT_A2DP_DELAY_MS   500
osTimerDef(APP_DELAY_CONNECT_A2DP_TIMER, app_delay_connect_a2dp_callback);
static osTimerId app_delay_connect_a2dp_timer = NULL;

static uint32_t delay_connect_a2dp_param1, delay_connect_a2dp_param2;

#define CONNECT_HFP_DELAY_MS   500
osTimerDef(APP_DELAY_CONNECT_HFP_TIMER, app_delay_connect_hfp_callback);
static osTimerId app_delay_connect_hfp_timer = NULL;

static uint32_t delay_connect_hfp_param1, delay_connect_hfp_param2;

void app_bt_delay_connect_a2dp(uint32_t param1, uint32_t param2)
{
    delay_connect_a2dp_param1 = param1;
    delay_connect_a2dp_param2 = param2;
    osTimerStart(app_delay_connect_a2dp_timer, CONNECT_A2DP_DELAY_MS);
}

void app_bt_delay_connect_hfp(uint32_t param1, uint32_t param2)
{
    delay_connect_hfp_param1 = param1;
    delay_connect_hfp_param2 = param2;
    osTimerStart(app_delay_connect_hfp_timer, CONNECT_HFP_DELAY_MS);
}

static void app_delay_connect_a2dp_callback(void const *param)
{
    app_bt_Force_A2DP_OpenStream((a2dp_stream_t *)delay_connect_a2dp_param1, 
        (bt_bdaddr_t *)delay_connect_a2dp_param2);
}

static void app_delay_connect_hfp_callback(void const *param)
{
    app_bt_Force_HF_CreateServiceLink((hf_chan_handle_t)delay_connect_hfp_param1, 
        (bt_bdaddr_t *)delay_connect_hfp_param2);
}


#if defined(__INTERCONNECTION__)
btif_accessible_mode_t app_bt_get_current_access_mode(void)
{
    return g_bt_access_mode;
}

bool app_bt_is_connected()
{
    uint8_t i=0;
    bool connceted_value=false;
    for(i=0;i<BT_DEVICE_NUM;i++)
    {
        if(bt_profile_manager[i].has_connected)
        {
            connceted_value = true;
            break;
        }
    }

    TRACE("bt_is_connected is %d", connceted_value);
    return connceted_value;
}
#endif

static void app_bt_precheck_before_starting_connecting(uint8_t isBtConnected);

static int app_bt_accessmode_timehandler(void const *param)
{
    const btif_access_mode_info_t info = { BTIF_BT_DEFAULT_INQ_SCAN_INTERVAL,
                                    BTIF_BT_DEFAULT_INQ_SCAN_WINDOW,
                                    BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL,
                                    BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW };
    btif_accessible_mode_t accMode = *(btif_accessible_mode_t*)(param);

    osapi_lock_stack();
    if (accMode == BTIF_BAM_CONNECTABLE_ONLY){
        app_bt_accessmode_timer_argument = BTIF_BAM_GENERAL_ACCESSIBLE;
        osTimerStart(app_bt_accessmode_timer, APP_BT_PROFILE_PAGE_SCAN_MS);
    }else if(accMode == BTIF_BAM_GENERAL_ACCESSIBLE){
        app_bt_accessmode_timer_argument = BTIF_BAM_CONNECTABLE_ONLY;
        osTimerStart(app_bt_accessmode_timer, APP_BT_PROFILE_BOTH_SCAN_MS);
    }
    app_bt_ME_SetAccessibleMode(accMode, &info);
    TRACE("app_bt_accessmode_timehandler accMode=%x",accMode);
    osapi_unlock_stack();
    return 0;
}


void app_bt_accessmode_set(btif_accessible_mode_t mode)
{
    const btif_access_mode_info_t info = { BTIF_BT_DEFAULT_INQ_SCAN_INTERVAL,
                                    BTIF_BT_DEFAULT_INQ_SCAN_WINDOW,
                                    BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL,
                                    BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW };
#if defined(IBRT)
    return;
#endif
    TRACE("%s %d",__func__, mode);
    osapi_lock_stack();
    g_bt_access_mode = mode;
    if (g_bt_access_mode == BTIF_BAM_GENERAL_ACCESSIBLE){
        app_bt_accessmode_timehandler(&g_bt_access_mode);
    }else{
        osTimerStop(app_bt_accessmode_timer);
        app_bt_ME_SetAccessibleMode(g_bt_access_mode, &info);
        TRACE("app_bt_accessmode_set access_mode=%x",g_bt_access_mode);
    }
    osapi_unlock_stack();
}

extern "C" uint8_t app_bt_get_act_cons(void)
{
    int activeCons;

    osapi_lock_stack();
    activeCons = btif_me_get_activeCons();
    osapi_unlock_stack();
    TRACE("%s %d",__func__,activeCons);
    return activeCons;
}
enum{
    INITIATE_PAIRING_NONE = 0,
    INITIATE_PAIRING_RUN = 1,
};
static uint8_t initiate_pairing = INITIATE_PAIRING_NONE;
void app_bt_connectable_state_set(uint8_t set)
{
    initiate_pairing = set;
}
bool is_app_bt_pairing_running(void)
{
    return (initiate_pairing == INITIATE_PAIRING_RUN)?(true):(false);
}
#define APP_DISABLE_PAGE_SCAN_AFTER_CONN
#ifdef APP_DISABLE_PAGE_SCAN_AFTER_CONN
osTimerId disable_page_scan_check_timer = NULL;
static void disable_page_scan_check_timer_handler(void const *param);
osTimerDef (DISABLE_PAGE_SCAN_CHECK_TIMER, (void (*)(void const *))disable_page_scan_check_timer_handler);                      // define timers
static void disable_page_scan_check_timer_handler(void const *param)
{
#ifdef __BT_ONE_BRING_TWO__
    if((btif_me_get_activeCons() > 1) && (initiate_pairing == INITIATE_PAIRING_NONE)){
#else
    if((btif_me_get_activeCons() > 0) && (initiate_pairing == INITIATE_PAIRING_NONE)){
#endif
        app_bt_accessmode_set_req(BTIF_BAM_NOT_ACCESSIBLE);
    }
}

static void disable_page_scan_check_timer_start(void)
{
    if(disable_page_scan_check_timer == NULL){
        disable_page_scan_check_timer = osTimerCreate(osTimer(DISABLE_PAGE_SCAN_CHECK_TIMER), osTimerOnce, NULL);
    }
    osTimerStart(disable_page_scan_check_timer, 4000);
}

#endif
void PairingTransferToConnectable(void)
{
    int activeCons;
    osapi_lock_stack();
    activeCons = btif_me_get_activeCons();
    osapi_unlock_stack();
    TRACE("%s",__func__);

    app_bt_connectable_state_set(INITIATE_PAIRING_NONE);
    if(activeCons == 0){
        TRACE("!!!PairingTransferToConnectable  BAM_CONNECTABLE_ONLY\n");
        app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
    }
}
int app_bt_get_audio_up_id(void)
{
    uint8_t i;
    btif_remote_device_t *remDev = NULL;
    btif_cmgr_handler_t    *cmgrHandler;
    for (i=0; i<BT_DEVICE_NUM; i++)
    {
        remDev = btif_me_enumerate_remote_devices(i);
        cmgrHandler = btif_cmgr_get_acl_handler(remDev);
        if(btif_cmgr_is_audio_up(cmgrHandler) == true)
            break;
    }
    return i;
}

int app_bt_state_checker(void)
{
    btif_remote_device_t *remDev = NULL;
    btif_cmgr_handler_t    *cmgrHandler;

#if defined(IBRT)
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (app_tws_ibrt_mobile_link_connected()){
        remDev = p_ibrt_ctrl->p_mobile_remote_dev;
        cmgrHandler = btif_cmgr_get_acl_handler(remDev);
        TRACE_IMM("checker: IBRT_MASTER activeCons:%d", btif_me_get_activeCons());        
        TRACE("checker: mobile state:%d mode:%d role:%d sniffInterval:%d/%d IsAudioUp:%d", 
                                                                           btif_me_get_remote_device_state(remDev),
                                                                           btif_me_get_remote_device_mode(remDev),  
                                                                           btif_me_get_remote_device_role(remDev),
                                                                           btif_cmgr_get_cmgrhandler_sniff_Interval(cmgrHandler), 
                                                                           btif_cmgr_get_cmgrhandler_sniff_info(cmgrHandler)->maxInterval, 
                                                                           app_bt_device.hf_audio_state[0]);
        DUMP8("0x%02x ", btif_me_get_remote_device_bdaddr(remDev)->address, BTIF_BD_ADDR_SIZE);
        TRACE("remDev:%x a2dp State:%d remDev:%x hf_channel Connected:%d remDev:%x ",
                                remDev,
                                app_bt_device.a2dp_connected_stream[0] ?  btif_a2dp_get_stream_state(app_bt_device.a2dp_connected_stream[0]):BTIF_A2DP_STREAM_STATE_CLOSED,
                                app_bt_device.a2dp_connected_stream[0] ?  btif_a2dp_get_stream_devic_cmgrHandler_remdev(app_bt_device.a2dp_connected_stream[0]):NULL,
                                app_bt_device.hf_conn_flag[0],
                                (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(app_bt_device.hf_channel[0]));
        
        remDev = p_ibrt_ctrl->p_tws_remote_dev;
        cmgrHandler = btif_cmgr_get_acl_handler(remDev);
        TRACE("checker: tws state:%d mode:%d role:%d sniffInterval:%d/%d", 
                                                                           btif_me_get_remote_device_state(remDev),
                                                                           btif_me_get_remote_device_mode(remDev),  
                                                                           btif_me_get_remote_device_role(remDev),
                                                                           btif_cmgr_get_cmgrhandler_sniff_Interval(cmgrHandler), 
                                                                           btif_cmgr_get_cmgrhandler_sniff_info(cmgrHandler)->maxInterval);        
        DUMP8("0x%02x ", btif_me_get_remote_device_bdaddr(remDev)->address, BTIF_BD_ADDR_SIZE);
        
    }else if (app_tws_ibrt_ibrt_link_connected()){
        TRACE_IMM("checker: IBRT_SLAVE activeCons:%d", btif_me_get_activeCons());
        TRACE("a2dp State:%d remDev:%x hf_channel Connected:%d remDev:%x ",
                                app_bt_device.a2dp_connected_stream[0] ?  btif_a2dp_get_stream_state(app_bt_device.a2dp_connected_stream[0]):BTIF_A2DP_STREAM_STATE_CLOSED,
                                app_bt_device.a2dp_connected_stream[0] ?  btif_a2dp_get_stream_devic_cmgrHandler_remdev(app_bt_device.a2dp_connected_stream[0]):NULL,
                                btif_hf_is_acl_connected(app_bt_device.hf_channel[0]),
                                (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(app_bt_device.hf_channel[0]));
        
        remDev = p_ibrt_ctrl->p_tws_remote_dev;
        cmgrHandler = btif_cmgr_get_acl_handler(remDev);
        TRACE("checker: tws state:%d mode:%d role:%d sniffInterval:%d/%d", 
                                                                           btif_me_get_remote_device_state(remDev),
                                                                           btif_me_get_remote_device_mode(remDev),  
                                                                           btif_me_get_remote_device_role(remDev),
                                                                           btif_cmgr_get_cmgrhandler_sniff_Interval(cmgrHandler), 
                                                                           btif_cmgr_get_cmgrhandler_sniff_info(cmgrHandler)->maxInterval);        
        DUMP8("0x%02x ", btif_me_get_remote_device_bdaddr(remDev)->address, BTIF_BD_ADDR_SIZE);
    }else{
        TRACE_IMM("checker: IBRT_UNKNOW activeCons:%d", btif_me_get_activeCons());
    }
    app_ibrt_if_ctx_checker();
#else
    for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
        remDev = btif_me_enumerate_remote_devices(i);
        cmgrHandler = btif_cmgr_get_acl_handler(remDev);
        TRACE("checker: id:%d state:%d mode:%d role:%d cmghdl:%x sniffInterva:%d/%d IsAudioUp:%d", i,  btif_me_get_remote_device_state(remDev),
                 btif_me_get_remote_device_mode(remDev),  btif_me_get_remote_device_role(remDev), cmgrHandler,
                     btif_cmgr_get_cmgrhandler_sniff_Interval(cmgrHandler), btif_cmgr_get_cmgrhandler_sniff_info(cmgrHandler)->maxInterval, app_bt_device.hf_audio_state[i]);
        DUMP8("0x%02x ", btif_me_get_remote_device_bdaddr(remDev)->address, BTIF_BD_ADDR_SIZE);
        TRACE("remDev:%x a2dp State:%d remDev:%x hf_channel Connected:%d remDev:%x ",
                                remDev,
                                app_bt_device.a2dp_connected_stream[i] ?  btif_a2dp_get_stream_state(app_bt_device.a2dp_connected_stream[i]):BTIF_A2DP_STREAM_STATE_CLOSED,
                                app_bt_device.a2dp_connected_stream[i] ?  btif_a2dp_get_stream_devic_cmgrHandler_remdev(app_bt_device.a2dp_connected_stream[i]):NULL,
                                app_bt_device.hf_conn_flag[i],
                                (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(app_bt_device.hf_channel[i]));

#ifdef __AI_VOICE__
    TRACE("ai_setup_complete %d", ai_struct.ai_stream.ai_setup_complete);
#endif
#ifdef IS_MULTI_AI_ENABLED
    TRACE("current_spec %d", ai_manager_get_current_spec());
#endif
#if defined (__HSP_ENABLE__)
        TRACE("hs_channel Connected:%d remDev:%x ",
                        app_bt_device.hs_conn_flag[i],
                        app_bt_device.hs_channel[i].cmgrHandler.remDev);
#endif
    }
#endif
    return 0;
}

static uint8_t app_bt_get_devId_from_RemDev(  btif_remote_device_t* remDev)
{
    uint8_t connectedDevId = 0;
    for (uint8_t devId = 0;devId < BT_DEVICE_NUM;devId++)
    {
        if ( btif_me_enumerate_remote_devices(devId) == remDev)
        {
            connectedDevId = devId;
            break;
        }
    }

    return connectedDevId;
}
void app_bt_accessible_manager_process(const btif_event_t *Event)
{
#if defined(IBRT)
    //IBRT device's access mode will be controlled by UI
    return;
#else
    btif_event_type_t etype = btif_me_get_callback_event_type(Event);
#ifdef __BT_ONE_BRING_TWO__
    static uint8_t opening_reconnect_cnf_cnt = 0;
    //uint8_t disconnectedDevId = app_bt_get_devId_from_RemDev(btif_me_get_callback_event_rem_dev( Event));

    if (app_bt_profile_connect_openreconnecting(NULL)){
        if (etype == BTIF_BTEVENT_LINK_CONNECT_CNF){
            opening_reconnect_cnf_cnt++;
        }
        if (record2_avalible){
            if (opening_reconnect_cnf_cnt<2){
                return;
            }
        }
    }
#endif
    switch (etype) {
        case BTIF_BTEVENT_LINK_CONNECT_CNF:
        case BTIF_BTEVENT_LINK_CONNECT_IND:
            TRACE("BTEVENT_LINK_CONNECT_IND/CNF activeCons:%d",btif_me_get_activeCons());
#if defined(__BTIF_EARPHONE__)   && !defined(FPGA)
            app_stop_10_second_timer(APP_PAIR_TIMER_ID);
#endif
#ifdef __BT_ONE_BRING_TWO__
             if(btif_me_get_activeCons() == 0){
#ifdef __EARPHONE_STAY_BOTH_SCAN__
                app_bt_accessmode_set_req(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#else
                app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
#endif
            }else if(btif_me_get_activeCons() == 1){
                app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
            }else if(btif_me_get_activeCons() >= 2){
                app_bt_accessmode_set_req(BTIF_BAM_NOT_ACCESSIBLE);
            }
#else
            if(btif_me_get_activeCons() == 0){
#ifdef __EARPHONE_STAY_BOTH_SCAN__
                app_bt_accessmode_set_req(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#else
                app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
#endif
            }else if(btif_me_get_activeCons() >= 1){
                app_bt_accessmode_set_req(BTIF_BAM_NOT_ACCESSIBLE);
            }
#endif
            break;
        case BTIF_BTEVENT_LINK_DISCONNECT:
            TRACE("DISCONNECT activeCons:%d",btif_me_get_activeCons());
#ifdef __EARPHONE_STAY_BOTH_SCAN__
#ifdef __BT_ONE_BRING_TWO__
            if(btif_me_get_activeCons() == 0){
                app_bt_accessmode_set_req(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
            }else if(btif_me_get_activeCons() == 1){
                app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
            }else if(btif_me_get_activeCons() >= 2){
                app_bt_accessmode_set_req(BTIF_BAM_NOT_ACCESSIBLE);
            }
#else
            app_bt_accessmode_set_req(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#endif
#else
            app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
#endif
            break;
#ifdef __BT_ONE_BRING_TWO__
        case BTIF_BTEVENT_SCO_CONNECT_IND:
        case BTIF_BTEVENT_SCO_CONNECT_CNF:
            if(btif_me_get_activeCons() == 1){
                app_bt_accessmode_set_req(BTIF_BAM_NOT_ACCESSIBLE);
            }
            break;
        case BTIF_BTEVENT_SCO_DISCONNECT:
            if(btif_me_get_activeCons() == 1){
                app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
            }
            break;
#endif
        default:
            break;
    }
#endif
}

#define APP_BT_SWITCHROLE_LIMIT (2)
//#define __SET_OUR_AS_MASTER__

void app_bt_role_manager_process(const btif_event_t *Event)
{
#if defined(IBRT)
    return;
#else
    static btif_remote_device_t *opRemDev = NULL;
    static uint8_t switchrole_cnt = 0;
    btif_remote_device_t *remDev = NULL;
    btif_event_type_t etype = btif_me_get_callback_event_type(Event);
    //on phone connecting
    switch (etype) {
        case BTIF_BTEVENT_LINK_CONNECT_IND:
            if(  btif_me_get_callback_event_err_code(Event) == BTIF_BEC_NO_ERROR){
                if (btif_me_get_activeCons() == 1){
                    switch ( btif_me_get_callback_event_rem_dev_role (Event)) {
#if defined(__SET_OUR_AS_MASTER__)
                        case BTIF_BCR_SLAVE:
                        case BTIF_BCR_PSLAVE:
#else
                        case BTIF_BCR_MASTER:
                        case BTIF_BCR_PMASTER:
#endif
                            TRACE("CONNECT_IND try to role %x\n",   btif_me_get_callback_event_rem_dev( Event));
                            //curr connectrot try to role
                            opRemDev =btif_me_get_callback_event_rem_dev( Event);
                            switchrole_cnt = 0;
                            app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev( Event), BTIF_BLP_MASTER_SLAVE_SWITCH|BTIF_BLP_SNIFF_MODE);
                            break;
#if defined(__SET_OUR_AS_MASTER__)
                        case BTIF_BCR_MASTER:
                        case BTIF_BCR_PMASTER:
#else
                        case BTIF_BCR_SLAVE:
                        case BTIF_BCR_PSLAVE:
#endif
                        case BTIF_BCR_ANY:
                        case BTIF_BCR_UNKNOWN:
                        default:
                            TRACE("CONNECT_IND disable role %x\n",btif_me_get_callback_event_rem_dev( Event));
                            //disable roleswitch when 1 connect
                            app_bt_Me_SetLinkPolicy  (   btif_me_get_callback_event_rem_dev( Event), BTIF_BLP_SNIFF_MODE);
                            break;
                    }
                    //set next connector to master
                    app_bt_ME_SetConnectionRole(BTIF_BCR_MASTER);
                }else if (btif_me_get_activeCons() > 1){
                    switch (btif_me_get_callback_event_rem_dev_role (Event)) {
                        case BTIF_BCR_MASTER:
                        case BTIF_BCR_PMASTER:
                            TRACE("CONNECT_IND disable role %x\n",btif_me_get_callback_event_rem_dev( Event));
                            //disable roleswitch
                            app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev( Event), BTIF_BLP_SNIFF_MODE);
                            break;
                        case BTIF_BCR_SLAVE:
                        case BTIF_BCR_PSLAVE:
                        case BTIF_BCR_ANY:
                        case BTIF_BCR_UNKNOWN:
                        default:
                            //disconnect slave
                            TRACE("CONNECT_IND disconnect slave %x\n",btif_me_get_callback_event_rem_dev( Event));
                            app_bt_MeDisconnectLink(btif_me_get_callback_event_rem_dev( Event));
                            break;
                    }
                    //set next connector to master
                    app_bt_ME_SetConnectionRole(BTIF_BCR_MASTER);
                }
            }
            break;
        case BTIF_BTEVENT_LINK_CONNECT_CNF:
            if (btif_me_get_activeCons() == 1){
                switch (btif_me_get_callback_event_rem_dev_role (Event)) {
#if defined(__SET_OUR_AS_MASTER__)
                    case BTIF_BCR_SLAVE:
                    case BTIF_BCR_PSLAVE:
#else
                    case BTIF_BCR_MASTER:
                    case BTIF_BCR_PMASTER:
#endif
                        TRACE("CONNECT_CNF try to role %x\n",btif_me_get_callback_event_rem_dev( Event));
                        //curr connectrot try to role
                        opRemDev =btif_me_get_callback_event_rem_dev( Event);
                        switchrole_cnt = 0;
                        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev( Event), BTIF_BLP_MASTER_SLAVE_SWITCH|BTIF_BLP_SNIFF_MODE);
                        app_bt_ME_SwitchRole(btif_me_get_callback_event_rem_dev( Event));
                        break;
#if defined(__SET_OUR_AS_MASTER__)
                    case BTIF_BCR_MASTER:
                    case BTIF_BCR_PMASTER:
#else
                    case BTIF_BCR_SLAVE:
                    case BTIF_BCR_PSLAVE:
#endif
                    case BTIF_BCR_ANY:
                    case BTIF_BCR_UNKNOWN:
                    default:
                        TRACE("CONNECT_CNF disable role %x\n",btif_me_get_callback_event_rem_dev( Event));
                        //disable roleswitch
                        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev( Event), BTIF_BLP_SNIFF_MODE);
                        break;
                }
                //set next connector to master
                app_bt_ME_SetConnectionRole(BTIF_BCR_MASTER);
            }else if (btif_me_get_activeCons() > 1){
                switch (btif_me_get_callback_event_rem_dev_role (Event)) {
                    case BTIF_BCR_MASTER:
                    case BTIF_BCR_PMASTER :
                        TRACE("CONNECT_CNF disable role %x\n",btif_me_get_callback_event_rem_dev( Event));
                        //disable roleswitch
                        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev( Event), BTIF_BLP_SNIFF_MODE);
                        break;
                    case BTIF_BCR_SLAVE:
                    case BTIF_BCR_ANY:
                    case BTIF_BCR_UNKNOWN:
                    default:
                        //disconnect slave
                        TRACE("CONNECT_CNF disconnect slave %x\n",btif_me_get_callback_event_rem_dev( Event));
                        app_bt_MeDisconnectLink(btif_me_get_callback_event_rem_dev( Event));
                        break;
                }
                //set next connector to master
                app_bt_ME_SetConnectionRole(BTIF_BCR_MASTER);
            }
            break;
        case BTIF_BTEVENT_LINK_DISCONNECT:
            if (opRemDev ==btif_me_get_callback_event_rem_dev( Event)){
                opRemDev = NULL;
                switchrole_cnt = 0;
            }
            if (btif_me_get_activeCons() == 0){
                for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
                    if(app_bt_device.a2dp_connected_stream[i])
                        app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_connected_stream[i], FALSE);
                    app_bt_HF_SetMasterRole(app_bt_device.hf_channel[i], FALSE);
                }
                app_bt_ME_SetConnectionRole(BTIF_BCR_ANY);
            }else if (btif_me_get_activeCons() == 1){
                //set next connector to master
                app_bt_ME_SetConnectionRole(BTIF_BCR_MASTER);
            }
            break;
        case BTIF_BTEVENT_ROLE_CHANGE:
        if (opRemDev ==btif_me_get_callback_event_rem_dev( Event)){
                switch ( btif_me_get_callback_event_role_change_new_role(Event)) {
#if defined(__SET_OUR_AS_MASTER__)
                    case BTIF_BCR_SLAVE:
#else
                    case BTIF_BCR_MASTER:
#endif
                        if (++switchrole_cnt<=APP_BT_SWITCHROLE_LIMIT){
                            app_bt_ME_SwitchRole(btif_me_get_callback_event_rem_dev( Event));
                        }else{
#if defined(__SET_OUR_AS_MASTER__)
                            TRACE("ROLE TO MASTER FAILED remDev%x cnt:%d\n",btif_me_get_callback_event_rem_dev( Event), switchrole_cnt);
#else
                            TRACE("ROLE TO SLAVE FAILED remDev%x cnt:%d\n",btif_me_get_callback_event_rem_dev( Event), switchrole_cnt);
#endif
                            opRemDev = NULL;
                            switchrole_cnt = 0;
                        }
                        break;
#if defined(__SET_OUR_AS_MASTER__)
                    case BTIF_BCR_MASTER:
                        TRACE("ROLE TO MASTER SUCCESS remDev%x cnt:%d\n",btif_me_get_callback_event_rem_dev( Event), switchrole_cnt);
#else
                    case BTIF_BCR_SLAVE:
                        TRACE("ROLE TO SLAVE SUCCESS remDev%x cnt:%d\n",btif_me_get_callback_event_rem_dev( Event), switchrole_cnt);
#endif
                        opRemDev = NULL;
                        switchrole_cnt = 0;
                        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev( Event),BTIF_BLP_SNIFF_MODE);
                        break;
                    case BTIF_BCR_ANY:
                        break;
                    case BTIF_BCR_UNKNOWN:
                        break;
                    default:
                        break;
                }
            }

            if (btif_me_get_activeCons() > 1){
                uint8_t slave_cnt = 0;
                for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
                    remDev = btif_me_enumerate_remote_devices(i);
                    if ( btif_me_get_current_role(remDev) == BTIF_BCR_SLAVE){
                        slave_cnt++;
                    }
                }
                if (slave_cnt>1){
                    TRACE("ROLE_CHANGE disconnect slave %x\n",btif_me_get_callback_event_rem_dev( Event));
                    app_bt_MeDisconnectLink(btif_me_get_callback_event_rem_dev( Event));
                }
            }
            break;
        default:
           break;
        }
#endif
}

void app_bt_role_manager_process_dual_slave(const btif_event_t *Event)
{
#if defined(IBRT)
    return;
#else
    static btif_remote_device_t *opRemDev = NULL;
    static uint8_t switchrole_cnt = 0;
    //btif_remote_device_t *remDev = NULL;
    //on phone connecting
    switch ( btif_me_get_callback_event_type(Event)) {
        case BTIF_BTEVENT_LINK_CONNECT_IND:
        case BTIF_BTEVENT_LINK_CONNECT_CNF:
            if(btif_me_get_callback_event_err_code(Event) == BTIF_BEC_NO_ERROR){
                switch (btif_me_get_callback_event_rem_dev_role (Event)) {
#if defined(__SET_OUR_AS_MASTER__)
                    case BTIF_BCR_SLAVE:
                    case BTIF_BCR_PSLAVE:
#else
                    case BTIF_BCR_MASTER:
                    case BTIF_BCR_PMASTER:
#endif
                        TRACE("CONNECT_IND/CNF try to role %x\n",btif_me_get_callback_event_rem_dev( Event));
                        opRemDev =btif_me_get_callback_event_rem_dev( Event);
                        switchrole_cnt = 0;
                        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev( Event), BTIF_BLP_MASTER_SLAVE_SWITCH|BTIF_BLP_SNIFF_MODE);
                        app_bt_ME_SwitchRole(btif_me_get_callback_event_rem_dev( Event));
                        break;
#if defined(__SET_OUR_AS_MASTER__)
                    case BTIF_BCR_MASTER:
                    case BTIF_BCR_PMASTER:
#else
                    case BTIF_BCR_SLAVE:
                    case BTIF_BCR_PSLAVE:
#endif
                    case BTIF_BCR_ANY:
                    case BTIF_BCR_UNKNOWN:
                    default:
                        TRACE("CONNECT_IND disable role %x\n",btif_me_get_callback_event_rem_dev( Event));
                        app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev( Event), BTIF_BLP_SNIFF_MODE);
                        break;
                }
                app_bt_ME_SetConnectionRole(BTIF_BCR_SLAVE);
            }
            break;
        case BTIF_BTEVENT_LINK_DISCONNECT:
            if (opRemDev ==btif_me_get_callback_event_rem_dev( Event)){
                opRemDev = NULL;
                switchrole_cnt = 0;
            }
            if (  btif_me_get_activeCons() == 0){
                for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
                    if(app_bt_device.a2dp_connected_stream[i])
                        app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_connected_stream[i], FALSE);
                    app_bt_HF_SetMasterRole(app_bt_device.hf_channel[i], FALSE);
                }
                app_bt_ME_SetConnectionRole(BTIF_BCR_ANY);
            }else if (btif_me_get_activeCons() == 1){
                app_bt_ME_SetConnectionRole(BTIF_BCR_SLAVE);
            }
            break;
        case BTIF_BTEVENT_ROLE_CHANGE:
            if (opRemDev ==btif_me_get_callback_event_rem_dev( Event)){
                switch (btif_me_get_callback_event_role_change_new_role(Event)) {
#if defined(__SET_OUR_AS_MASTER__)
                    case BTIF_BCR_SLAVE:
#else
                    case BTIF_BCR_MASTER:
#endif
                        if (++switchrole_cnt<=APP_BT_SWITCHROLE_LIMIT){
                            TRACE("ROLE_CHANGE try to role again: %d", switchrole_cnt);
                            app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev( Event), BTIF_BLP_MASTER_SLAVE_SWITCH|BTIF_BLP_SNIFF_MODE);
                            app_bt_ME_SwitchRole(btif_me_get_callback_event_rem_dev( Event));
                        }else{
#if defined(__SET_OUR_AS_MASTER__)
                            TRACE("ROLE TO MASTER FAILED remDev%x cnt:%d\n",btif_me_get_callback_event_rem_dev( Event), switchrole_cnt);
#else
                            TRACE("ROLE TO SLAVE FAILED remDev%x cnt:%d\n",btif_me_get_callback_event_rem_dev( Event), switchrole_cnt);
#endif
                            opRemDev = NULL;
                            switchrole_cnt = 0;
                        }
                        break;
#if defined(__SET_OUR_AS_MASTER__)
                    case BTIF_BCR_MASTER:
                        TRACE("ROLE TO MASTER SUCCESS remDev%x cnt:%d\n",btif_me_get_callback_event_rem_dev( Event), switchrole_cnt);
#else
                    case BTIF_BCR_SLAVE:
                        TRACE("ROLE TO SLAVE SUCCESS remDev%x cnt:%d\n",btif_me_get_callback_event_rem_dev( Event), switchrole_cnt);
#endif
                        opRemDev = NULL;
                        switchrole_cnt = 0;
                        //workaround for power reset opening reconnect sometime unsuccessfully in sniff mode,
                        //only after authentication completes, enable sniff mode.
                        opRemDev =btif_me_get_callback_event_rem_dev( Event);
                        if (btif_me_get_remote_device_auth_state(opRemDev) == BTIF_BAS_AUTHENTICATED)
                        {
                            app_bt_Me_SetLinkPolicy(opRemDev,BTIF_BLP_SNIFF_MODE);
                        }
                        else
                        {
                            app_bt_Me_SetLinkPolicy(opRemDev,BTIF_BLP_DISABLE_ALL);
                        }
                        break;
                    case BTIF_BCR_ANY:
                        break;
                    case BTIF_BCR_UNKNOWN:
                        break;
                    default:
                        break;
                }
            }
            break;
    }
#endif
}

static int app_bt_sniff_manager_init(void)
{
    btif_sniff_info_t sniffInfo;
    btif_remote_device_t *remDev = NULL;

    for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
        remDev = btif_me_enumerate_remote_devices(i);
        sniffInfo.maxInterval = BTIF_CMGR_SNIFF_MAX_INTERVAL;
        sniffInfo.minInterval = BTIF_CMGR_SNIFF_MIN_INTERVAL;
        sniffInfo.attempt = BTIF_CMGR_SNIFF_ATTEMPT;
        sniffInfo.timeout =BTIF_CMGR_SNIFF_TIMEOUT;
        app_bt_CMGR_SetSniffInofToAllHandlerByRemDev(&sniffInfo, remDev);
        app_bt_HF_EnableSniffMode(app_bt_device.hf_channel[i], FALSE);
#if defined (__HSP_ENABLE__)
        app_bt_HS_EnableSniffMode(&app_bt_device.hs_channel[i], FALSE);
#endif
    }

    return 0;
}

void app_bt_sniff_config(btif_remote_device_t *remDev)
{
    btif_remote_device_t* tmpRemDev;
    btif_cmgr_handler_t    *currbtif_cmgr_handler_t = NULL;
    btif_cmgr_handler_t    *otherbtif_cmgr_handler_t = NULL;
    btif_sniff_info_t sniffInfo;
    sniffInfo.maxInterval = BTIF_CMGR_SNIFF_MAX_INTERVAL;
    sniffInfo.minInterval = BTIF_CMGR_SNIFF_MIN_INTERVAL;
    sniffInfo.attempt = BTIF_CMGR_SNIFF_ATTEMPT;
    sniffInfo.timeout = BTIF_CMGR_SNIFF_TIMEOUT;
    app_bt_CMGR_SetSniffInofToAllHandlerByRemDev(&sniffInfo, remDev);
    if (btif_me_get_activeCons() > 1){
        currbtif_cmgr_handler_t = btif_cmgr_get_conn_ind_handler(remDev);
        for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
            tmpRemDev = btif_me_enumerate_remote_devices(i);
            if (remDev != tmpRemDev){
                otherbtif_cmgr_handler_t = btif_cmgr_get_acl_handler(tmpRemDev);
                if (otherbtif_cmgr_handler_t && currbtif_cmgr_handler_t){
                    if ( btif_cmgr_get_cmgrhandler_sniff_info(otherbtif_cmgr_handler_t)->maxInterval == btif_cmgr_get_cmgrhandler_sniff_info(currbtif_cmgr_handler_t)->maxInterval){
                        sniffInfo.maxInterval = btif_cmgr_get_cmgrhandler_sniff_info(otherbtif_cmgr_handler_t)->maxInterval -20;
                        sniffInfo.minInterval = btif_cmgr_get_cmgrhandler_sniff_info(otherbtif_cmgr_handler_t)->minInterval - 20;
                        sniffInfo.attempt = BTIF_CMGR_SNIFF_ATTEMPT;
                        sniffInfo.timeout = BTIF_CMGR_SNIFF_TIMEOUT;
                        app_bt_CMGR_SetSniffInofToAllHandlerByRemDev(&sniffInfo, remDev);
                    }
                }
                break;
            }
        }
    }
}

void app_bt_sniff_manager_process(const btif_event_t *Event)
{
    static btif_remote_device_t *opRemDev = NULL;
    btif_remote_device_t *remDev = NULL;
    btif_cmgr_handler_t    *currbtif_cmgr_handler_t = NULL;
    btif_cmgr_handler_t    *otherbtif_cmgr_handler_t = NULL;

    btif_sniff_info_t sniffInfo;

    if (!besbt_cfg.sniff)
        return;

    switch (btif_me_get_callback_event_type(Event)) {
        case BTIF_BTEVENT_LINK_CONNECT_IND:
            break;
        case BTIF_BTEVENT_LINK_CONNECT_CNF:
            break;
        case BTIF_BTEVENT_LINK_DISCONNECT:
             if (opRemDev ==btif_me_get_callback_event_rem_dev( Event)){
                opRemDev = NULL;
            }
            sniffInfo.maxInterval = BTIF_CMGR_SNIFF_MAX_INTERVAL;
            sniffInfo.minInterval = BTIF_CMGR_SNIFF_MIN_INTERVAL;
            sniffInfo.attempt = BTIF_CMGR_SNIFF_ATTEMPT;
            sniffInfo.timeout = BTIF_CMGR_SNIFF_TIMEOUT;
            app_bt_CMGR_SetSniffInofToAllHandlerByRemDev(&sniffInfo,btif_me_get_callback_event_rem_dev( Event));
            break;
        case BTIF_BTEVENT_MODE_CHANGE:

            /*
            if(Event->p.modeChange.curMode == BLM_SNIFF_MODE){
                currbtif_cmgr_handler_t = btif_cmgr_get_acl_handler(btif_me_get_callback_event_rem_dev( Event));
                if (Event->p.modeChange.interval > CMGR_SNIFF_MAX_INTERVAL){
                        if (!opRemDev){
                            opRemDev = currbtif_cmgr_handler_t->remDev;
                        }
                        currbtif_cmgr_handler_t->sniffInfo.maxInterval = CMGR_SNIFF_MAX_INTERVAL;
                        currbtif_cmgr_handler_t->sniffInfo.minInterval = CMGR_SNIFF_MIN_INTERVAL;
                        currbtif_cmgr_handler_t->sniffInfo.attempt = CMGR_SNIFF_ATTEMPT;
                        currbtif_cmgr_handler_t->sniffInfo.timeout = CMGR_SNIFF_TIMEOUT;
                        app_bt_CMGR_SetSniffInofToAllHandlerByRemDev(&currbtif_cmgr_handler_t->sniffInfo,btif_me_get_callback_event_rem_dev( Event));
                        app_bt_ME_StopSniff(currbtif_cmgr_handler_t->remDev);
                }else{
                    if (currbtif_cmgr_handler_t){
                        currbtif_cmgr_handler_t->sniffInfo.maxInterval = Event->p.modeChange.interval;
                        currbtif_cmgr_handler_t->sniffInfo.minInterval = Event->p.modeChange.interval;
                        currbtif_cmgr_handler_t->sniffInfo.attempt = CMGR_SNIFF_ATTEMPT;
                        currbtif_cmgr_handler_t->sniffInfo.timeout = CMGR_SNIFF_TIMEOUT;
                        app_bt_CMGR_SetSniffInofToAllHandlerByRemDev(&currbtif_cmgr_handler_t->sniffInfo,btif_me_get_callback_event_rem_dev( Event));
                    }
                    if (btif_me_get_activeCons() > 1){
                        for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
                            remDev = btif_me_enumerate_remote_devices(i);
                            if (btif_me_get_callback_event_rem_dev( Event) != remDev){
                                otherbtif_cmgr_handler_t = btif_cmgr_get_acl_handler(remDev);
                                if (otherbtif_cmgr_handler_t){
                                    if (otherbtif_cmgr_handler_t->sniffInfo.maxInterval == currbtif_cmgr_handler_t->sniffInfo.maxInterval){
                                        if (btif_me_get_current_mode(remDev) == BLM_ACTIVE_MODE){
                                            otherbtif_cmgr_handler_t->sniffInfo.maxInterval -= 20;
                                            otherbtif_cmgr_handler_t->sniffInfo.minInterval -= 20;
                                            otherbtif_cmgr_handler_t->sniffInfo.attempt = CMGR_SNIFF_ATTEMPT;
                                            otherbtif_cmgr_handler_t->sniffInfo.timeout = CMGR_SNIFF_TIMEOUT;
                                            app_bt_CMGR_SetSniffInofToAllHandlerByRemDev(&otherbtif_cmgr_handler_t->sniffInfo, remDev);
                                            TRACE("reconfig sniff other RemDev:%x\n", remDev);
                                        }else if (btif_me_get_current_mode(remDev) == BLM_SNIFF_MODE){
                                            need_reconfig = true;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                    if (need_reconfig){
                        opRemDev = remDev;
                        if (currbtif_cmgr_handler_t){
                            currbtif_cmgr_handler_t->sniffInfo.maxInterval -= 20;
                            currbtif_cmgr_handler_t->sniffInfo.minInterval -= 20;
                            currbtif_cmgr_handler_t->sniffInfo.attempt = CMGR_SNIFF_ATTEMPT;
                            currbtif_cmgr_handler_t->sniffInfo.timeout = CMGR_SNIFF_TIMEOUT;
                            app_bt_CMGR_SetSniffInofToAllHandlerByRemDev(&currbtif_cmgr_handler_t->sniffInfo, currbtif_cmgr_handler_t->remDev);
                        }
                        app_bt_ME_StopSniff(currbtif_cmgr_handler_t->remDev);
                        TRACE("reconfig sniff setup op opRemDev:%x\n", opRemDev);
                    }
                }
            }
            if (Event->p.modeChange.curMode == BLM_ACTIVE_MODE){
                if (opRemDev ==btif_me_get_callback_event_rem_dev( Event)){
                    TRACE("reconfig sniff op opRemDev:%x\n", opRemDev);
                    opRemDev = NULL;
                    currbtif_cmgr_handler_t = btif_cmgr_get_acl_handler(btif_me_get_callback_event_rem_dev( Event));
                    if (currbtif_cmgr_handler_t){
                        app_bt_CMGR_SetSniffTimer(currbtif_cmgr_handler_t, NULL, CMGR_SNIFF_TIMER);
                    }
                }
            }
            */
            break;
        case BTIF_BTEVENT_ACL_DATA_ACTIVE:
            btif_cmgr_handler_t    *cmgrHandler;
            /* Start the sniff timer */
            cmgrHandler = btif_cmgr_get_acl_handler(btif_me_get_callback_event_rem_dev( Event));
            if (cmgrHandler)
                app_bt_CMGR_SetSniffTimer(cmgrHandler, NULL, BTIF_CMGR_SNIFF_TIMER);
            break;
        case BTIF_BTEVENT_SCO_CONNECT_IND:
        case BTIF_BTEVENT_SCO_CONNECT_CNF:
            TRACE("BTEVENT_SCO_CONNECT_IND/CNF cur_remDev = %x",btif_me_get_callback_event_rem_dev( Event));
            currbtif_cmgr_handler_t = btif_cmgr_get_conn_ind_handler(btif_me_get_callback_event_rem_dev( Event));
            app_bt_Me_SetLinkPolicy( btif_me_get_callback_event_sco_connect_rem_dev(Event), BTIF_BLP_DISABLE_ALL);
            if (btif_me_get_activeCons() > 1){
                for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
                    remDev = btif_me_enumerate_remote_devices(i);
                    TRACE("other_remDev = %x",remDev);
                    if (btif_me_get_callback_event_rem_dev( Event) == remDev){
                        continue;
                    }

                    otherbtif_cmgr_handler_t = btif_cmgr_get_conn_ind_handler(remDev);
                    if (otherbtif_cmgr_handler_t){
                        if (btif_cmgr_is_link_up(otherbtif_cmgr_handler_t)){
                            if ( btif_me_get_current_mode(remDev) == BTIF_BLM_ACTIVE_MODE){
                                TRACE("other dev disable sniff");
                                app_bt_Me_SetLinkPolicy(remDev, BTIF_BLP_DISABLE_ALL);
                            }else if (btif_me_get_current_mode(remDev) == BTIF_BLM_SNIFF_MODE){
                                TRACE(" ohter dev exit & disable sniff");
                                app_bt_ME_StopSniff(remDev);
                                app_bt_Me_SetLinkPolicy(remDev, BTIF_BLP_DISABLE_ALL);
                            }
                        }
                    }

#if defined (HFP_NO_PRERMPT)
                TRACE("cur_audio = %d other_audio = %d",btif_cmgr_is_audio_up(currbtif_cmgr_handler_t),
                    btif_cmgr_is_audio_up(otherbtif_cmgr_handler_t));
                if((btif_cmgr_is_audio_up(otherbtif_cmgr_handler_t) == true) &&
                    (btif_cmgr_is_audio_up(currbtif_cmgr_handler_t) == true)
                    /*(btapp_hfp_get_call_active()!=0)*/){
                    btif_cmgr_remove_audio_link(currbtif_cmgr_handler_t);
                    app_bt_Me_switch_sco(btif_cmgr_get_sco_connect_sco_Hcihandler(otherbtif_cmgr_handler_t));
                }
#endif
                }
            }
            break;
        case BTIF_BTEVENT_SCO_DISCONNECT:
            app_bt_profile_reconnect_pending_process();
            if (a2dp_is_music_ongoing())
            {
                break;
            }
            if (btif_me_get_activeCons() == 1){
                app_bt_Me_SetLinkPolicy( btif_me_get_callback_event_sco_connect_rem_dev(Event), BTIF_BLP_SNIFF_MODE);
            }else{
                uint8_t i;
                for (i=0; i<BT_DEVICE_NUM; i++){
                    remDev = btif_me_enumerate_remote_devices(i);
                    if (btif_me_get_callback_event_rem_dev( Event) == remDev){
                        break;
                    }
                }
                if(i==0)
                    remDev = btif_me_enumerate_remote_devices(1);
                else if(i==1)
                    remDev = btif_me_enumerate_remote_devices(0);
                else 
                    ASSERT(0,"error other remotedevice!!!");                
                otherbtif_cmgr_handler_t = btif_cmgr_get_conn_ind_handler(remDev);
                currbtif_cmgr_handler_t = btif_cmgr_get_conn_ind_handler(btif_me_get_callback_event_rem_dev( Event));
                
                TRACE("SCO_DISCONNECT:%d%/%d %x/%x\n", btif_cmgr_is_audio_up(currbtif_cmgr_handler_t), btif_cmgr_is_audio_up(otherbtif_cmgr_handler_t),
                                                 btif_cmgr_get_cmgrhandler_remdev(currbtif_cmgr_handler_t),btif_me_get_callback_event_rem_dev( Event));
                if (otherbtif_cmgr_handler_t){
                    if (!btif_cmgr_is_audio_up(otherbtif_cmgr_handler_t)){
                        TRACE("enable sniff to all\n");
                        app_bt_Me_SetLinkPolicy(  btif_me_get_callback_event_sco_connect_rem_dev(Event), BTIF_BLP_SNIFF_MODE);
                        app_bt_Me_SetLinkPolicy( btif_cmgr_get_cmgrhandler_remdev(otherbtif_cmgr_handler_t), BTIF_BLP_SNIFF_MODE);
                    }
                }else{
                    app_bt_Me_SetLinkPolicy( btif_me_get_callback_event_sco_connect_rem_dev(Event), BTIF_BLP_SNIFF_MODE);
                }
            }
            break;
        default:
            break;
    }
}

APP_BT_GOLBAL_HANDLE_HOOK_HANDLER app_bt_global_handle_hook_handler[APP_BT_GOLBAL_HANDLE_HOOK_USER_QTY] = {0};
void app_bt_global_handle_hook(const btif_event_t *Event)
{
    uint8_t i;
    for (i=0; i<APP_BT_GOLBAL_HANDLE_HOOK_USER_QTY; i++){
        if (app_bt_global_handle_hook_handler[i])
            app_bt_global_handle_hook_handler[i](Event);
    }
}

int app_bt_global_handle_hook_set(enum APP_BT_GOLBAL_HANDLE_HOOK_USER_T user, APP_BT_GOLBAL_HANDLE_HOOK_HANDLER handler)
{
    app_bt_global_handle_hook_handler[user] = handler;
    return 0;
}

APP_BT_GOLBAL_HANDLE_HOOK_HANDLER app_bt_global_handle_hook_get(enum APP_BT_GOLBAL_HANDLE_HOOK_USER_T user)
{
    return app_bt_global_handle_hook_handler[user];
}

/////There is a device connected, so stop PAIR_TIMER and POWEROFF_TIMER of earphone.
btif_handler app_bt_handler;
static void app_bt_global_handle(const btif_event_t *Event)
{
    switch (btif_me_get_callback_event_type(Event)) {
        case BTIF_BTEVENT_HCI_INITIALIZED:
#ifndef __IAG_BLE_INCLUDE__
            app_notify_stack_ready();
#endif
            break;
#if defined(IBRT)        
        case BTIF_BTEVENT_HCI_COMMAND_SENT:
            return;
#else
        case BTIF_BTEVENT_HCI_COMMAND_SENT:
        case BTIF_BTEVENT_ACL_DATA_NOT_ACTIVE:
            return;
        case BTIF_BTEVENT_ACL_DATA_ACTIVE:
            btif_cmgr_handler_t    *cmgrHandler;
            /* Start the sniff timer */
            cmgrHandler = btif_cmgr_get_acl_handler(btif_me_get_callback_event_rem_dev( Event));
            if (cmgrHandler)
                app_bt_CMGR_SetSniffTimer(cmgrHandler, NULL, BTIF_CMGR_SNIFF_TIMER);
            return;
#endif
         case BTIF_BTEVENT_AUTHENTICATED:
            TRACE("GLOBAL HANDER AUTH error=%x", btif_me_get_callback_event_err_code(Event));
            //after authentication completes, re-enable sniff mode.
            if(btif_me_get_callback_event_err_code(Event) == BTIF_BEC_NO_ERROR)
            {
                app_bt_Me_SetLinkPolicy(btif_me_get_callback_event_rem_dev( Event),BTIF_BLP_SNIFF_MODE);
            }
            break;
    }
    // trace filter
    switch (btif_me_get_callback_event_type(Event)) {
        case BTIF_BTEVENT_HCI_COMMAND_SENT:
        case BTIF_BTEVENT_ACL_DATA_NOT_ACTIVE:
        case BTIF_BTEVENT_ACL_DATA_ACTIVE:
            break;
        default:                
            TRACE("app_bt_global_handle evt = %d",btif_me_get_callback_event_type(Event));
            break;
    }
    
    switch (btif_me_get_callback_event_type(Event)) {
        case BTIF_BTEVENT_LINK_CONNECT_IND:
        case BTIF_BTEVENT_LINK_CONNECT_CNF:
#ifdef BTIF_DIP_DEVICE
            btif_dip_get_remote_info(btif_me_get_callback_event_rem_dev( Event));
#endif
            if (BTIF_BEC_NO_ERROR == btif_me_get_callback_event_err_code(Event))
            {
                connectedMobile = btif_me_get_callback_event_rem_dev( Event);
                uint8_t connectedDevId = app_bt_get_devId_from_RemDev(connectedMobile);
                app_bt_set_connecting_profiles_state(connectedDevId);
                TRACE("MEC(pendCons) is %d", btif_me_get_pendCons());
                if (1 == btif_me_get_pendCons())
                {
                    btif_me_set_pendCons(0) ;
                }
                app_bt_stay_active_rem_dev(btif_me_get_callback_event_rem_dev( Event));

#ifdef VOICE_DATAPATH
                app_voicepath_bt_link_connected_handler(btif_me_get_callback_event_rem_dev_bd_addr( Event)->address);
#endif
            }

            TRACE("CONNECT_IND/CNF evt:%d errCode:0x%0x newRole:%d activeCons:%d",btif_me_get_callback_event_type(Event),
                btif_me_get_callback_event_err_code(Event),btif_me_get_callback_event_rem_dev_role (Event), btif_me_get_activeCons());

#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)  && !defined(FPGA)
            if (btif_me_get_activeCons() == 0){
                app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
            }else{
                app_stop_10_second_timer(APP_POWEROFF_TIMER_ID);
            }
#endif
#ifdef __BT_ONE_BRING_TWO__
            if (btif_me_get_activeCons() > 2){
                app_bt_MeDisconnectLink(btif_me_get_callback_event_rem_dev( Event));
            }
#else
            if (btif_me_get_activeCons() > 1){
                app_bt_MeDisconnectLink(btif_me_get_callback_event_rem_dev( Event));
            }
#endif
            // nv_record_flash_flush();
            break;
        case BTIF_BTEVENT_LINK_DISCONNECT:
        {
            connectedMobile = btif_me_get_callback_event_rem_dev( Event);
            uint8_t disconnectedDevId = app_bt_get_devId_from_RemDev(connectedMobile);
            connectedMobile = NULL;
            app_bt_clear_connecting_profiles_state(disconnectedDevId);

            TRACE("DISCONNECT evt = %d encryptState:%d",btif_me_get_callback_event_type(Event),  btif_me_get_remote_sevice_encrypt_state(  btif_me_get_callback_event_rem_dev( Event)));
            #ifdef CHIP_BEST2000
                 bt_drv_patch_force_disconnect_ack();
            #endif
             //disconnect from reconnect connection, and the HF don't connect successful once
             //(whitch will release the saved_reconnect_mode ). so we are reconnect fail with remote link key loss.
             // goto pairing.
             //reason 07 maybe from the controller's error .
             //05  auth error
             //16  io cap reject.

#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__) && !defined(FPGA)
            if (btif_me_get_activeCons() == 0){
                app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
            }
#endif

#ifdef VOICE_DATAPATH
            app_voicepath_bt_link_disconnected_handler( btif_me_get_remote_device_bdaddr(btif_me_get_callback_event_rem_dev( Event))->address);
#endif
            // nv_record_flash_flush();
            break;
        }
        case BTIF_BTEVENT_ROLE_CHANGE:
            TRACE("ROLE_CHANGE eType:0x%x errCode:0x%x newRole:%d activeCons:%d", btif_me_get_callback_event_type(Event),
                btif_me_get_callback_event_err_code(Event), btif_me_get_callback_event_role_change_new_role(Event), btif_me_get_activeCons());
            break;
        case BTIF_BTEVENT_MODE_CHANGE:
            TRACE("MODE_CHANGE evt:%d errCode:0x%0x curMode=0x%0x, interval=%d ",btif_me_get_callback_event_type(Event),
                btif_me_get_callback_event_err_code(Event), btif_me_get_callback_event_mode_change_curMode(Event),
                btif_me_get_callback_event_mode_change_interval(Event));
            break;
        case BTIF_BTEVENT_ACCESSIBLE_CHANGE:
#if !defined(IBRT)
            TRACE("ACCESSIBLE_CHANGE evt:%d errCode:0x%0x aMode=0x%0x",btif_me_get_callback_event_type(Event), btif_me_get_callback_event_err_code(Event),
            btif_me_get_callback_event_a_mode(Event));
            if (app_is_access_mode_set_pending())
            {
                app_set_pending_access_mode();
            }
            else
            {
                if (BTIF_BEC_NO_ERROR != btif_me_get_callback_event_err_code(Event))
                {
                    app_retry_setting_access_mode();
                }
            }
#endif
            break;
        case BTIF_BTEVENT_LINK_POLICY_CHANGED:
        {
            BT_SET_LINKPOLICY_REQ_T* pReq = app_bt_pop_pending_set_linkpolicy();
            if (NULL != pReq)
            {
                app_bt_Me_SetLinkPolicy(pReq->remDev, pReq->policy);
            }
            break;
        }
        case BTIF_BTEVENT_NAME_RESULT:
        {
            uint8_t* ptrName;
            uint8_t nameLen;
            nameLen = btif_me_get_callback_event_remote_dev_name(Event, &ptrName);      
            TRACE("remote dev name len %d", nameLen);
            if (nameLen > 0)
            {
                TRACE("remote dev name: %s", ptrName);
            }
            return;
        }
        default:
            break;
    }

#ifdef MULTIPOINT_DUAL_SLAVE
    app_bt_role_manager_process_dual_slave(Event);
#else
    app_bt_role_manager_process(Event);
#endif
    app_bt_accessible_manager_process(Event);
#if !defined(IBRT)
    app_bt_sniff_manager_process(Event);
#endif
    app_bt_global_handle_hook(Event);
#if defined(IBRT)
    app_tws_ibrt_global_callback(Event);
#endif
}

#include "app_bt_media_manager.h"
osTimerId bt_sco_recov_timer = NULL;
static void bt_sco_recov_timer_handler(void const *param);
osTimerDef (BT_SCO_RECOV_TIMER, (void (*)(void const *))bt_sco_recov_timer_handler);                      // define timers
void hfp_reconnect_sco(uint8_t flag);
static void bt_sco_recov_timer_handler(void const *param)
{
    TRACE("%s",__func__);
    hfp_reconnect_sco(0);
}
static void bt_sco_recov_timer_start()
{
    osTimerStop(bt_sco_recov_timer);
    osTimerStart(bt_sco_recov_timer, 2500);
}


enum{
    SCO_DISCONNECT_RECONN_START,
    SCO_DISCONNECT_RECONN_RUN,
    SCO_DISCONNECT_RECONN_NONE,
};

static uint8_t sco_reconnect_status =  SCO_DISCONNECT_RECONN_NONE;

void hfp_reconnect_sco(uint8_t set)
{
    TRACE("%s cur_chl_id=%d reconnect_status =%d",__func__,app_bt_device.curr_hf_channel_id,
        sco_reconnect_status);
    if(set == 1){
        sco_reconnect_status = SCO_DISCONNECT_RECONN_START;
    }
    if(sco_reconnect_status == SCO_DISCONNECT_RECONN_START){
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,BT_STREAM_VOICE, app_bt_device.curr_hf_channel_id,MAX_RECORD_NUM);
        app_bt_HF_DisconnectAudioLink(app_bt_device.hf_channel[app_bt_device.curr_hf_channel_id]);
        sco_reconnect_status = SCO_DISCONNECT_RECONN_RUN;
        bt_sco_recov_timer_start();
    }else if(sco_reconnect_status == SCO_DISCONNECT_RECONN_RUN){
        app_bt_HF_CreateAudioLink(app_bt_device.hf_channel[app_bt_device.curr_hf_channel_id]);
        sco_reconnect_status = SCO_DISCONNECT_RECONN_NONE;
    }
}


void app_bt_global_handle_init(void)
{
    btif_event_mask_t mask = BTIF_BEM_NO_EVENTS;
    btif_me_init_handler(&app_bt_handler);
    app_bt_handler.callback = app_bt_global_handle;
    btif_me_register_global_handler(&app_bt_handler);
#if defined(IBRT)
    btif_me_register_accept_handler(&app_bt_handler);
#endif    
#ifdef IBRT_SEARCH_UI 
    app_bt_global_handle_hook_set(APP_BT_GOLBAL_HANDLE_HOOK_USER_0,app_bt_manager_ibrt_role_process);
#endif

    mask |= BTIF_BEM_ROLE_CHANGE | BTIF_BEM_SCO_CONNECT_CNF | BTIF_BEM_SCO_DISCONNECT | BTIF_BEM_SCO_CONNECT_IND;
    mask |= BTIF_BEM_AUTHENTICATED;
    mask |= BTIF_BEM_LINK_CONNECT_IND;
    mask |= BTIF_BEM_LINK_DISCONNECT;
    mask |= BTIF_BEM_LINK_CONNECT_CNF;
    mask |= BTIF_BEM_ACCESSIBLE_CHANGE;
    mask |= BTIF_BEM_ENCRYPTION_CHANGE;
#if (defined(__BT_ONE_BRING_TWO__)||defined(IBRT))
    mask |= BTIF_BEM_MODE_CHANGE;
#endif
    mask |= BTIF_BEM_LINK_POLICY_CHANGED;

    app_bt_ME_SetConnectionRole(BTIF_BCR_ANY);
    for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
        app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_stream[i]->a2dp_stream, FALSE);
#if defined(A2DP_LHDC_ON)
        app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_lhdc_stream[i]->a2dp_stream, FALSE);
#endif
#if defined(A2DP_AAC_ON)
        app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_aac_stream[i]->a2dp_stream, FALSE);
#endif
#if defined(A2DP_SCALABLE_ON)
        app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_scalable_stream[i]->a2dp_stream, FALSE);
#endif
#if defined(A2DP_LDAC_ON)
        app_bt_A2DP_SetMasterRole(app_bt_device.a2dp_ldac_stream[i]->a2dp_stream, FALSE);
#endif

        app_bt_HF_SetMasterRole(app_bt_device.hf_channel[i], FALSE);
#if defined (__HSP_ENABLE__)
        HS_SetMasterRole(&app_bt_device.hs_channel[i], FALSE);
#endif
    }
    btif_me_set_event_mask(&app_bt_handler, mask);
    app_bt_sniff_manager_init();
    app_bt_accessmode_timer = osTimerCreate (osTimer(APP_BT_ACCESSMODE_TIMER), osTimerOnce, &app_bt_accessmode_timer_argument);
    bt_sco_recov_timer = osTimerCreate (osTimer(BT_SCO_RECOV_TIMER), osTimerOnce, NULL);
}

void app_bt_send_request(uint32_t message_id, uint32_t param0, uint32_t param1, uint32_t ptr)
{
    APP_MESSAGE_BLOCK msg;

    msg.mod_id = APP_MODUAL_BT;
    msg.msg_body.message_id = message_id;
    msg.msg_body.message_Param0 = param0;
    msg.msg_body.message_Param1 = param1;
    msg.msg_body.message_ptr = ptr;
    app_mailbox_put(&msg);
}

extern void app_start_10_second_timer(uint8_t timer_id);

static int app_bt_handle_process(APP_MESSAGE_BODY *msg_body)
{
    btif_accessible_mode_t old_access_mode;

    switch (msg_body->message_id) {
        case APP_BT_REQ_ACCESS_MODE_SET:
            old_access_mode = g_bt_access_mode;
            app_bt_accessmode_set(msg_body->message_Param0);
            if (msg_body->message_Param0 == BTIF_BAM_GENERAL_ACCESSIBLE &&
                old_access_mode != BTIF_BAM_GENERAL_ACCESSIBLE){
#ifndef FPGA
                app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
                app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
                app_start_10_second_timer(APP_PAIR_TIMER_ID);
#endif
            }else{
#ifndef FPGA
                app_status_indication_set(APP_STATUS_INDICATION_PAGESCAN);
#endif
            }
            break;
        default:
            break;
    }

    return 0;
}

void *app_bt_profile_active_store_ptr_get(uint8_t *bdAddr)
{
#if defined(IBRT)
    static btdevice_profile device_profile = {true, false, true,0};
#else
    static btdevice_profile device_profile = {false, false, false,0};
#endif
    btdevice_profile *ptr;

#ifndef FPGA
    nvrec_btdevicerecord *record = NULL;
    if (!nv_record_btdevicerecord_find((bt_bdaddr_t *)bdAddr,&record)){
        ptr = &(record->device_plf);
        DUMP8("0x%02x ", bdAddr, BTIF_BD_ADDR_SIZE);
        TRACE("%s hfp_act:%d hsp_act:%d a2dp_act:0x%x codec_type=%x", __func__, ptr->hfp_act, ptr->hsp_act, ptr->a2dp_act,ptr->a2dp_codectype);
    }else
#endif
    {
        ptr = &device_profile;
        TRACE("%s default", __func__);
    }
    return (void *)ptr;
}

static void app_bt_profile_reconnect_timehandler(void const *param);

osTimerDef (BT_PROFILE_CONNECT_TIMER0, app_bt_profile_reconnect_timehandler);                      // define timers
#ifdef __BT_ONE_BRING_TWO__
osTimerDef (BT_PROFILE_CONNECT_TIMER1, app_bt_profile_reconnect_timehandler);
#endif

#ifdef __AUTO_CONNECT_OTHER_PROFILE__
static void app_bt_profile_connect_hf_retry_handler(void)
{
    struct app_bt_profile_manager *bt_profile_manager_p = (struct app_bt_profile_manager *)param;
    if (MEC(pendCons) > 0)
    {
        TRACE("Former link is not down yet, reset the timer %s.", __FUNCTION__);
        osTimerStart(bt_profile_manager_p->connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
    }
    else
    {
        app_bt_precheck_before_starting_connecting(bt_profile_manager_p->has_connected);
        if (bt_profile_manager_p->hfp_connect != bt_profile_connect_status_success)
        {
            app_bt_HF_CreateServiceLink(bt_profile_manager_p->chan, &bt_profile_manager_p->rmt_addr);
        }
    }
}

static void app_bt_profile_connect_hf_retry_timehandler(void const *param)
{
    app_bt_start_custom_function_in_bt_thread(0, 0,
        (uint32_t)app_bt_profile_connect_hf_retry_handler);
}

#if defined (__HSP_ENABLE__)
static void app_bt_profile_connect_hs_retry_timehandler(void const *param)
{
    struct app_bt_profile_manager *bt_profile_manager_p = (struct app_bt_profile_manager *)param;
    if (MEC(pendCons) > 0)
    {
        if (bt_profile_manager_p->reconnect_cnt < APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT)
        {
            bt_profile_manager_p->reconnect_cnt++;
        }
        TRACE("Former link is not down yet, reset the timer %s.", __FUNCTION__);
        osTimerStart(bt_profile_manager_p->connect_timer,
            BTIF_BT_DEFAULT_PAGE_TIMEOUT_IN_MS+APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
    }
    else
    {
        if (bt_profile_manager_p->hsp_connect != bt_profile_connect_status_success)
        {
            app_bt_HS_CreateServiceLink(bt_profile_manager_p->hs_chan, &bt_profile_manager_p->rmt_addr);
        }
    }
}
#endif

static bool app_bt_profile_manager_connect_a2dp_filter_connected_a2dp_stream(BT_BD_ADDR bd_addr)
{
    uint8_t i =0;
    BtRemoteDevice *StrmRemDev;
    A2dpStream * connected_stream;

    for(i =0;i<BT_DEVICE_NUM;i++){
        if((app_bt_device.a2dp_stream[i].stream.state == AVDTP_STRM_STATE_STREAMING ||
            app_bt_device.a2dp_stream[i].stream.state == AVDTP_STRM_STATE_OPEN)){
            connected_stream = &app_bt_device.a2dp_stream[i];
            StrmRemDev = A2DP_GetRemoteDevice(connected_stream);
            if(memcmp(StrmRemDev->bdAddr.addr,bd_addr.addr,BD_ADDR_SIZE) == 0){
                return true;
            }
        }
    }
    return false;
}

static void app_bt_profile_connect_a2dp_retry_handler(void)
{
    struct app_bt_profile_manager *bt_profile_manager_p = (struct app_bt_profile_manager *)param;
    if (MEC(pendCons) > 0)
    {
        if (bt_profile_manager_p->reconnect_cnt < APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT)
        {
            bt_profile_manager_p->reconnect_cnt++;
        }
        TRACE("Former link is not down yet, reset the timer %s.", __FUNCTION__);
        osTimerStart(bt_profile_manager_p->connect_timer,
            BTIF_BT_DEFAULT_PAGE_TIMEOUT_IN_MS+APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
    }
    else
    {
        if(app_bt_profile_manager_connect_a2dp_filter_connected_a2dp_stream(bt_profile_manager_p->rmt_addr) == true){
            TRACE("has been connected , no need to init connect again");
            return ;
        }
        app_bt_precheck_before_starting_connecting(bt_profile_manager_p->has_connected);
        if (bt_profile_manager_p->a2dp_connect != bt_profile_connect_status_success)
        {
            app_bt_A2DP_OpenStream(bt_profile_manager_p->stream, &bt_profile_manager_p->rmt_addr);
        }
    }
}

static void app_bt_profile_connect_a2dp_retry_timehandler(void const *param)
{
    app_bt_start_custom_function_in_bt_thread(0, 0,
        (uint32_t)app_bt_profile_connect_a2dp_retry_handler);
}
#endif

void app_bt_reset_reconnect_timer(bt_bdaddr_t *pBdAddr)
{
    uint8_t devId = 0;
    for (uint8_t i = 0; i < BT_DEVICE_NUM; i++)
    {
        if (pBdAddr == &(bt_profile_manager[i].rmt_addr))
        {
            devId = i;
            break;
        }
    }

    TRACE("Resart the reconnecting timer of dev %d", devId);
    osTimerStart(bt_profile_manager[devId].connect_timer,
        BTIF_BT_DEFAULT_PAGE_TIMEOUT_IN_MS+APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
}

static void app_bt_profile_reconnect_handler(void const *param)
{
#if !defined(IBRT)
    struct app_bt_profile_manager *bt_profile_manager_p = (struct app_bt_profile_manager *)param;
    if ( btif_me_get_pendCons() > 0)
    {
        if (bt_profile_manager_p->reconnect_cnt < APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT)
        {
            bt_profile_manager_p->reconnect_cnt++;
        }
        TRACE("Former link is not down yet, reset the timer %s.", __FUNCTION__);
        osTimerStart(bt_profile_manager_p->connect_timer,
            BTIF_BT_DEFAULT_PAGE_TIMEOUT_IN_MS+APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
    }
    else
    {
        btdevice_profile *btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(bt_profile_manager_p->rmt_addr.address);
        if (bt_profile_manager_p->connect_timer_cb){
            bt_profile_manager_p->connect_timer_cb(param);
            bt_profile_manager_p->connect_timer_cb = NULL;
        }else{
            if (btdevice_plf_p->hfp_act){
                TRACE("try connect hf");
                app_bt_precheck_before_starting_connecting(bt_profile_manager_p->has_connected);
                app_bt_HF_CreateServiceLink(bt_profile_manager_p->chan, (bt_bdaddr_t *)&bt_profile_manager_p->rmt_addr);
            }
#if defined (__HSP_ENABLE__)
            else if(btdevice_plf_p->hsp_act){
                TRACE("try connect hs");
                app_bt_precheck_before_starting_connecting(bt_profile_manager_p->has_connected);
                app_bt_HS_CreateServiceLink(bt_profile_manager_p->hs_chan, &bt_profile_manager_p->rmt_addr);
            }
#endif
            else if(btdevice_plf_p->a2dp_act){
                TRACE("try connect a2dp");
                app_bt_precheck_before_starting_connecting(bt_profile_manager_p->has_connected);
                app_bt_A2DP_OpenStream(bt_profile_manager_p->stream, &bt_profile_manager_p->rmt_addr);
            }
        }
    }
#else
    TRACE("ibrt_ui_log:app_bt_profile_reconnect_timehandler called");
#endif
}

static void app_bt_profile_reconnect_timehandler(void const *param)
{
    app_bt_start_custom_function_in_bt_thread((uint32_t)param, 0,
        (uint32_t)app_bt_profile_reconnect_handler);
}
    
bool app_bt_is_in_connecting_profiles_state(void)
{
    for (uint8_t devId = 0;devId < BT_DEVICE_NUM;devId++)
    {
        if (APP_BT_IN_CONNECTING_PROFILES_STATE == bt_profile_manager[devId].connectingState)
        {
            return true;
        }
    }

    return false;
}

void app_bt_clear_connecting_profiles_state(uint8_t devId)
{
    TRACE("Dev %d exists connecting profiles state", devId);

    bt_profile_manager[devId].connectingState = APP_BT_IDLE_STATE;
    if (!app_bt_is_in_connecting_profiles_state())
    {
#ifdef __IAG_BLE_INCLUDE__
        app_start_fast_connectable_ble_adv(BLE_FAST_ADVERTISING_INTERVAL);
#endif
    }
}

void app_bt_set_connecting_profiles_state(uint8_t devId)
{
    TRACE("Dev %d enters connecting profiles state", devId);

    bt_profile_manager[devId].connectingState = APP_BT_IN_CONNECTING_PROFILES_STATE;
#ifdef  __IAG_BLE_INCLUDE__
    // stop BLE adv
    ble_stop_activities();
#endif
}

void app_bt_profile_connect_manager_open(void)
{
    uint8_t i=0;
    for (i=0;i<BT_DEVICE_NUM;i++){
        bt_profile_manager[i].has_connected = false;
        bt_profile_manager[i].hfp_connect = bt_profile_connect_status_unknow;
        bt_profile_manager[i].hsp_connect = bt_profile_connect_status_unknow;
        bt_profile_manager[i].a2dp_connect = bt_profile_connect_status_unknow;
        memset(bt_profile_manager[i].rmt_addr.address, 0, BTIF_BD_ADDR_SIZE);
        bt_profile_manager[i].reconnect_mode = bt_profile_reconnect_null;
        bt_profile_manager[i].saved_reconnect_mode = bt_profile_reconnect_null;
        bt_profile_manager[i].stream = NULL;
        bt_profile_manager[i].chan = NULL;
#if defined (__HSP_ENABLE__)
        bt_profile_manager[i].hs_chan = NULL;
#endif
        bt_profile_manager[i].reconnect_cnt = 0;
        bt_profile_manager[i].connect_timer_cb = NULL;
        bt_profile_manager[i].connectingState = APP_BT_IDLE_STATE;
    }

    bt_profile_manager[BT_DEVICE_ID_1].connect_timer = osTimerCreate (osTimer(BT_PROFILE_CONNECT_TIMER0), osTimerOnce, &bt_profile_manager[BT_DEVICE_ID_1]);
#ifdef __BT_ONE_BRING_TWO__
    bt_profile_manager[BT_DEVICE_ID_2].connect_timer = osTimerCreate (osTimer(BT_PROFILE_CONNECT_TIMER1), osTimerOnce, &bt_profile_manager[BT_DEVICE_ID_2]);
#endif

    app_delay_connect_a2dp_timer = osTimerCreate (osTimer(APP_DELAY_CONNECT_A2DP_TIMER), osTimerOnce, 0);
    app_delay_connect_hfp_timer = osTimerCreate (osTimer(APP_DELAY_CONNECT_HFP_TIMER), osTimerOnce, 0);
}

BOOL app_bt_profile_connect_openreconnecting(void *ptr)
{
    bool nRet = false;
    uint8_t i;

    /*
     * If launched from peer device,stop reconnecting and accept connection
     */
    if ((ptr != NULL) && (btif_me_get_remote_device_initiator((btif_remote_device_t *)ptr) == FALSE))
    {
        //Peer device launch reconnet,then we give up reconnect procedure
        TRACE("give up reconnecting");
        app_bt_connectable_mode_stop_reconnecting();
        return false;
    }

    for (i=0;i<BT_DEVICE_NUM;i++){
        nRet |= bt_profile_manager[i].reconnect_mode == bt_profile_reconnect_openreconnecting ? true : false;
        if(nRet){
            TRACE("io cap rj [%d]: %d", i, bt_profile_manager[i].reconnect_mode);
        }
    }

    return nRet;
}

bool app_bt_is_in_reconnecting(void)
{
    uint8_t devId;
    for (devId = 0;devId < BT_DEVICE_NUM;devId++)
    {
        if (bt_profile_reconnect_null != bt_profile_manager[devId].reconnect_mode)
        {
            return true;
        }
    }

    return false;
}

void app_bt_profile_connect_manager_opening_reconnect(void)
{
    int ret;
    btif_device_record_t record1;
    btif_device_record_t record2;
    btdevice_profile *btdevice_plf_p;
    int find_invalid_record_cnt;
#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
    if(app_bt_device.src_or_snk==BT_DEVICE_SRC)
    {
        return ;
    }
#endif
    osapi_lock_stack();
    do{
        find_invalid_record_cnt = 0;
        ret = nv_record_enum_latest_two_paired_dev(&record1,&record2);
        if(ret == 1){
            btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(record1.bdAddr.address);
            if (!(btdevice_plf_p->hfp_act)&&!(btdevice_plf_p->a2dp_act)){
                nv_record_ddbrec_delete((bt_bdaddr_t *)&record1.bdAddr);
                find_invalid_record_cnt++;
            }
        }else if(ret == 2){
            btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(record1.bdAddr.address);
            if (!(btdevice_plf_p->hfp_act)&&!(btdevice_plf_p->a2dp_act)){
                nv_record_ddbrec_delete((bt_bdaddr_t *)&record1.bdAddr);
                find_invalid_record_cnt++;
            }
            btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(record2.bdAddr.address);
            if (!(btdevice_plf_p->hfp_act)&&!(btdevice_plf_p->a2dp_act)){
                nv_record_ddbrec_delete((bt_bdaddr_t *)&record2.bdAddr);
                find_invalid_record_cnt++;
            }
        }
    }while(find_invalid_record_cnt);

    TRACE("!!!app_bt_opening_reconnect:\n");
    DUMP8("%02x ", &record1.bdAddr, 6);
    DUMP8("%02x ", &record2.bdAddr, 6);


    if(ret > 0){
        TRACE("!!!start reconnect first device\n");

        if(  btif_me_get_pendCons() == 0){
            bt_profile_manager[BT_DEVICE_ID_1].reconnect_mode = bt_profile_reconnect_openreconnecting;
            bt_profile_manager[BT_DEVICE_ID_1].reconnect_cnt = 0;
            memcpy(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr.address, record1.bdAddr.address, BTIF_BD_ADDR_SIZE);
            btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr.address);

#if defined(A2DP_LHDC_ON)
            if(btdevice_plf_p->a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
                bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream;
            else
#endif
#if defined(A2DP_AAC_ON)
            if(btdevice_plf_p->a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
                bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream;
            else
#endif
#if defined(A2DP_LDAC_ON)  //workaround for mate10 no a2dp issue when link back 
            if(btdevice_plf_p->a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_NON_A2DP){
                //bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream;
                //btdevice_plf_p->a2dp_codectype = BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC;
                bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream;
                btdevice_plf_p->a2dp_codectype = BTIF_AVDTP_CODEC_TYPE_NON_A2DP;
            }
            else
#endif

#if defined(A2DP_SCALABLE_ON)
            if(btdevice_plf_p->a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
                bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]->a2dp_stream;
            else
#endif
            {
                bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream;
            }

            bt_profile_manager[BT_DEVICE_ID_1].chan = app_bt_device.hf_channel[BT_DEVICE_ID_1];
#if defined (__HSP_ENABLE__)
            bt_profile_manager[BT_DEVICE_ID_1].hs_chan = &app_bt_device.hs_channel[BT_DEVICE_ID_1];
#endif

            if(btdevice_plf_p->a2dp_act) {
                TRACE("try connect a2dp");
                app_bt_precheck_before_starting_connecting(bt_profile_manager[BT_DEVICE_ID_1].has_connected);
                app_bt_A2DP_OpenStream(bt_profile_manager[BT_DEVICE_ID_1].stream, &bt_profile_manager[BT_DEVICE_ID_1].rmt_addr);
            }
            else if (btdevice_plf_p->hfp_act){
                TRACE("try connect hf");
                app_bt_precheck_before_starting_connecting(bt_profile_manager[BT_DEVICE_ID_1].has_connected);
                app_bt_HF_CreateServiceLink(bt_profile_manager[BT_DEVICE_ID_1].chan, (bt_bdaddr_t *)&bt_profile_manager[BT_DEVICE_ID_1].rmt_addr);
            }
#if defined (__HSP_ENABLE__)
            else if (btdevice_plf_p->hsp_act){
                TRACE("try connect hs");
                app_bt_precheck_before_starting_connecting(bt_profile_manager[BT_DEVICE_ID_1].has_connected);
                app_bt_HS_CreateServiceLink(bt_profile_manager[BT_DEVICE_ID_1].hs_chan, &bt_profile_manager[BT_DEVICE_ID_1].rmt_addr);
            }
#endif

        }
#ifdef __BT_ONE_BRING_TWO__
        if(ret > 1){
            TRACE("!!!need reconnect second device\n");
            bt_profile_manager[BT_DEVICE_ID_2].reconnect_mode = bt_profile_reconnect_openreconnecting;
            bt_profile_manager[BT_DEVICE_ID_2].reconnect_cnt = 0;
            memcpy(bt_profile_manager[BT_DEVICE_ID_2].rmt_addr.address, record2.bdAddr.address, BTIF_BD_ADDR_SIZE);
            btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(bt_profile_manager[BT_DEVICE_ID_2].rmt_addr.address);

#if defined(A2DP_LHDC_ON)
            if(btdevice_plf_p->a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
                bt_profile_manager[BT_DEVICE_ID_2].stream = app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_2]->a2dp_stream;
            else
#endif
#if defined(A2DP_AAC_ON)
            if(btdevice_plf_p->a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
                bt_profile_manager[BT_DEVICE_ID_2].stream = app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_2]->a2dp_stream;
            else
#endif
#if defined(A2DP_SCALABLE_ON)
            if(btdevice_plf_p->a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_NON_A2DP)
                bt_profile_manager[BT_DEVICE_ID_2].stream = app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_2]->a2dp_stream;
            else
#endif
            {
                bt_profile_manager[BT_DEVICE_ID_2].stream = app_bt_device.a2dp_stream[BT_DEVICE_ID_2]->a2dp_stream;
            }

            bt_profile_manager[BT_DEVICE_ID_2].chan = app_bt_device.hf_channel[BT_DEVICE_ID_2];
#if defined (__HSP_ENABLE__)
            bt_profile_manager[BT_DEVICE_ID_2].hs_chan = &app_bt_device.hs_channel[BT_DEVICE_ID_2];
#endif
        }
#endif
    }

    else
    {
        TRACE("!!!go to pairing\n");
#ifdef __EARPHONE_STAY_BOTH_SCAN__
        app_bt_accessmode_set_req(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#else
        app_bt_accessmode_set_req(BTIF_BAM_CONNECTABLE_ONLY);
#endif

    }
    osapi_unlock_stack();
}


void app_bt_resume_sniff_mode(uint8_t deviceId)
{
    if (bt_profile_connect_status_success == bt_profile_manager[deviceId].a2dp_connect||
        bt_profile_connect_status_success == bt_profile_manager[deviceId].hfp_connect||
        bt_profile_connect_status_success == bt_profile_manager[deviceId].hsp_connect)
    {
        app_bt_allow_sniff(deviceId);
        btif_remote_device_t* currentRemDev = app_bt_get_remoteDev(deviceId);
        app_bt_sniff_config(currentRemDev);
    }
}
#if !defined(IBRT)
static int8_t app_bt_profile_reconnect_pending(enum BT_DEVICE_ID_T id)
{
    if(btapp_hfp_is_dev_call_active(id) == true){
        bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_reconnect_pending;
        return 0;
    }
    return -1;
}
#endif
static int8_t app_bt_profile_reconnect_pending_process(void)
{
    uint8_t i =BT_DEVICE_NUM;

    btif_remote_device_t *remDev = NULL;
    btif_cmgr_handler_t    *cmgrHandler;


    for (i=0; i<BT_DEVICE_NUM; i++){
        remDev = btif_me_enumerate_remote_devices(i);
        cmgrHandler = btif_cmgr_get_acl_handler(remDev);
        if(btif_cmgr_is_audio_up(cmgrHandler) == 1)
            return -1;
    }
    for(i = 0;i < BT_DEVICE_NUM;i++){
        if(bt_profile_manager[i].reconnect_mode == bt_profile_reconnect_reconnect_pending)
            break;
    }

    if(i == BT_DEVICE_NUM)
        return -1;

    bt_profile_manager[i].reconnect_mode = bt_profile_reconnect_reconnecting;
#ifdef __IAG_BLE_INCLUDE__
    app_start_connectable_ble_adv(BLE_ADVERTISING_INTERVAL);
#endif
    osTimerStart(bt_profile_manager[i].connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
    return 0;
}

uint8_t app_bt_get_num_of_connected_dev(void)
{
    uint8_t num_of_connected_dev = 0;
    uint8_t deviceId;
    for (deviceId = 0; deviceId < BT_DEVICE_NUM; deviceId++)
    {
        if (bt_profile_manager[deviceId].has_connected)
        {
            num_of_connected_dev++;
        }
    }

    return num_of_connected_dev;
}

static uint8_t recorded_latest_connected_service_device_id = BT_DEVICE_ID_1;
void app_bt_record_latest_connected_service_device_id(uint8_t device_id)
{
    recorded_latest_connected_service_device_id = device_id;
}

uint8_t app_bt_get_recorded_latest_connected_service_device_id(void)
{
    return recorded_latest_connected_service_device_id;
}


static void app_bt_precheck_before_starting_connecting(uint8_t isBtConnected)
{
#ifdef __IAG_BLE_INCLUDE__
    if (!isBtConnected)
    {
        ble_stop_activities();
    }
#endif
}

static void app_bt_restore_reconnecting_idle_mode(uint8_t deviceId)
{
    bt_profile_manager[deviceId].reconnect_mode = bt_profile_reconnect_null;
#ifdef __IAG_BLE_INCLUDE__
    app_start_fast_connectable_ble_adv(BLE_FAST_ADVERTISING_INTERVAL);
#endif
}

#ifdef __BT_ONE_BRING_TWO__
static void app_bt_update_connectable_mode_after_connection_management(void)
{
    uint8_t deviceId;
    bool isEnterConnetableOnlyState = true;
    for (deviceId = 0; deviceId < BT_DEVICE_NUM; deviceId++)
    {
        // assure none of the device is in reconnecting mode
        if (bt_profile_manager[deviceId].reconnect_mode != bt_profile_reconnect_null)
        {
            isEnterConnetableOnlyState = false;
            break;
        }
    }

    if (isEnterConnetableOnlyState)
    {
        for (deviceId = 0; deviceId < BT_DEVICE_NUM; deviceId++)
        {
            if (!bt_profile_manager[deviceId].has_connected)
            {
                app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                return;
            }
        }
    }
}
#endif

static void app_bt_connectable_mode_stop_reconnecting_handler(void)
{
    uint8_t deviceId;
    btif_remote_device_t*remDev;
    btif_cmgr_handler_t * cmgrHandler;
    for (deviceId = 0; deviceId < BT_DEVICE_NUM; deviceId++)
    {
        if (bt_profile_manager[deviceId].reconnect_mode != bt_profile_reconnect_null){
            bt_profile_manager[deviceId].hfp_connect = bt_profile_connect_status_failure;
            bt_profile_manager[deviceId].reconnect_mode = bt_profile_reconnect_null;
            bt_profile_manager[deviceId].saved_reconnect_mode = bt_profile_reconnect_null;
            bt_profile_manager[deviceId].reconnect_cnt = 0;
            if(bt_profile_manager[deviceId].connect_timer !=NULL)
                osTimerStop(bt_profile_manager[deviceId].connect_timer);
            remDev = btif_me_enumerate_remote_devices(deviceId);
            cmgrHandler = btif_cmgr_get_acl_handler(remDev);
            btif_me_cancel_create_link( btif_cmgr_get_cmgrhandler_remdev_bthandle(cmgrHandler),remDev);

        }
    }
}

void app_bt_connectable_mode_stop_reconnecting(void)
{
    app_bt_start_custom_function_in_bt_thread(0, 0,
        (uint32_t)app_bt_connectable_mode_stop_reconnecting_handler);
}

void app_bt_enter_pairing_state(void)
{
    app_bt_accessmode_set(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
    app_bt_connectable_mode_stop_reconnecting();
#ifdef _GFPS_    
    app_enter_fastpairing_mode();
#endif
}

#if defined (__HSP_ENABLE__)
void app_bt_profile_connect_manager_hs(enum BT_DEVICE_ID_T id, HsChannel *Chan, HsCallbackParms *Info)
{
    btdevice_profile *btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get((uint8_t *)Info->p.remDev->bdAddr.address);

    osTimerStop(bt_profile_manager[id].connect_timer);
    bt_profile_manager[id].connect_timer_cb = NULL;
    if (Chan&&Info){
        switch(Info->event)
        {
            case HF_EVENT_SERVICE_CONNECTED:
                TRACE("%s HS_EVENT_SERVICE_CONNECTED",__func__);
                btdevice_plf_p->hsp_act = true;
#ifndef FPGA
                nv_record_touch_cause_flush();
#endif
                bt_profile_manager[id].hsp_connect = bt_profile_connect_status_success;
                bt_profile_manager[id].reconnect_cnt = 0;
                bt_profile_manager[id].hs_chan = &app_bt_device.hs_channel[id];
                memcpy(bt_profile_manager[id].rmt_addr.address, Info->p.remDev->bdAddr.address, BTIF_BD_ADDR_SIZE);
                if (false == bt_profile_manager[id].has_connected)
                {
                    app_bt_resume_sniff_mode(id);
                }
                if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_openreconnecting){
                    //do nothing
                }else if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_reconnecting){
                    if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect != bt_profile_connect_status_success){
                        TRACE("!!!continue connect a2dp\n");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[id].has_connected);
                        app_bt_A2DP_OpenStream(bt_profile_manager[id].stream, &bt_profile_manager[id].rmt_address);
                    }
                }
#ifdef __AUTO_CONNECT_OTHER_PROFILE__
                else{
                    if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect != bt_profile_connect_status_success){
                        bt_profile_manager[id].connect_timer_cb = app_bt_profile_connect_a2dp_retry_timehandler;
                        app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_CONNECT_RETRY_MS);
                    }
                }
#endif
                break;
            case HF_EVENT_SERVICE_DISCONNECTED:
                TRACE("%s HS_EVENT_SERVICE_DISCONNECTED discReason:%d",__func__, Info->p.remDev->discReason);
                bt_profile_manager[id].hsp_connect = bt_profile_connect_status_failure;
                if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_openreconnecting){
                    if (++bt_profile_manager[id].reconnect_cnt < APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT){
                        app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                    #ifdef __IAG_BLE_INCLUDE__
                        app_start_connectable_ble_adv(BLE_ADVERTISING_INTERVAL);
                    #endif
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
                        bt_profile_manager[id].hfp_connect = bt_profile_connect_status_unknow;
                    }
                }else if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_reconnecting){
                    if (++bt_profile_manager[id].reconnect_cnt < APP_BT_PROFILE_RECONNECT_RETRY_LIMIT_CNT){
                        app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                    #ifdef __IAG_BLE_INCLUDE__
                        app_start_connectable_ble_adv(BLE_ADVERTISING_INTERVAL);
                    #endif
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
                    }else{
                        app_bt_restore_reconnecting_idle_mode(id);
                        //bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
                    }
                    TRACE("%s try to reconnect cnt:%d",__func__, bt_profile_manager[id].reconnect_cnt);
#if !defined(IBRT)
                }else if(Info->p.remDev->discReason == 0x8){
                    bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_reconnecting;
                    app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                    TRACE("%s try to reconnect",__func__);
                    if(app_bt_profile_reconnect_pending(id) != 0)
                    {
                    #ifdef __IAG_BLE_INCLUDE__
                        app_start_connectable_ble_adv(BLE_ADVERTISING_INTERVAL);
                    #endif
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
                    }
#endif
                }else{
                    bt_profile_manager[id].hsp_connect = bt_profile_connect_status_unknow;
                }
                break;
            default:
                break;
        }
    }

    if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_reconnecting){
        bool reconnect_hsp_proc_final = true;
        bool reconnect_a2dp_proc_final = true;
        if (bt_profile_manager[id].hsp_connect == bt_profile_connect_status_failure){
            reconnect_hsp_proc_final = false;
        }
        if (bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_failure){
            reconnect_a2dp_proc_final = false;
        }
        if (reconnect_hsp_proc_final && reconnect_a2dp_proc_final){
            TRACE("!!!reconnect success %d/%d/%d\n", bt_profile_manager[id].hfp_connect, bt_profile_manager[id].hsp_connect, bt_profile_manager[id].a2dp_connect);
            app_bt_restore_reconnecting_idle_mode(id);
            // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
        }
    }else if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_openreconnecting){
        bool opening_hsp_proc_final = false;
        bool opening_a2dp_proc_final = false;

        if (btdevice_plf_p->hsp_act && bt_profile_manager[id].hsp_connect == bt_profile_connect_status_unknow){
            opening_hsp_proc_final = false;
        }else{
            opening_hsp_proc_final = true;
        }

        if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_unknow){
            opening_a2dp_proc_final = false;
        }else{
            opening_a2dp_proc_final = true;
        }

        if ((opening_hsp_proc_final && opening_a2dp_proc_final) ||
            (bt_profile_manager[id].hsp_connect == bt_profile_connect_status_failure)){
            TRACE("!!!reconnect success %d/%d/%d\n", bt_profile_manager[id].hfp_connect, bt_profile_manager[id].hsp_connect, bt_profile_manager[id].a2dp_connect);
            app_bt_restore_reconnecting_idle_mode(id);
            // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
        }

        if (btdevice_plf_p->hsp_act && bt_profile_manager[id].hsp_connect == bt_profile_connect_status_success){
            if (btdevice_plf_p->a2dp_act && !opening_a2dp_proc_final){
                TRACE("!!!continue connect a2dp\n");
                app_bt_precheck_before_starting_connecting(id);
                app_bt_A2DP_OpenStream(bt_profile_manager[id].stream, &bt_profile_manager[id].rmt_addr);
            }
        }

        if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_null){
            for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
                if (bt_profile_manager[i].reconnect_mode == bt_profile_reconnect_openreconnecting){
                    TRACE("!!!hs->start reconnect second device\n");
                    if ((btdevice_plf_p->hfp_act)&&(!bt_profile_manager[i].hfp_connect)){
                        TRACE("try connect hf");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[i].has_connected);
                        app_bt_HF_CreateServiceLink(bt_profile_manager[i].chan, &bt_profile_manager[i].rmt_addr);
                    }
                    else if ((btdevice_plf_p->hsp_act)&&(!bt_profile_manager[i].hsp_connect)){
                        TRACE("try connect hs");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[i].has_connected);
                        app_bt_HS_CreateServiceLink(bt_profile_manager[i].hs_chan, &bt_profile_manager[i].rmt_addr);

                    } else if((btdevice_plf_p->a2dp_act)&&(!bt_profile_manager[i].a2dp_connect)) {
                        TRACE("try connect a2dp");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[i].has_connected);
                        app_bt_A2DP_OpenStream(bt_profile_manager[i].stream, &bt_profile_manager[i].rmt_addr);
                    }
                    break;
                }
            }
        }
    }

    if (!bt_profile_manager[id].has_connected &&
        (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_success ||
         bt_profile_manager[id].hsp_connect == bt_profile_connect_status_success||
         bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_success)){

        bt_profile_manager[id].has_connected = true;
        TRACE("BT connected!!!");
        
        btif_me_get_remote_device_name(&(ctx->remote_dev_bdaddr), app_bt_global_handle);
#if defined(MEDIA_PLAYER_SUPPORT)&& !defined(IBRT)
        app_voice_report(APP_STATUS_INDICATION_CONNECTED, id);
#endif
#ifdef __INTERCONNECTION__
        app_interconnection_start_disappear_adv(BLE_ADVERTISING_INTERVAL, APP_INTERCONNECTION_DISAPPEAR_ADV_IN_MS);
        app_interconnection_spp_open();
#endif
    }

    if (bt_profile_manager[id].has_connected &&
        (bt_profile_manager[id].hfp_connect != bt_profile_connect_status_success &&
         bt_profile_manager[id].hsp_connect != bt_profile_connect_status_success &&
         bt_profile_manager[id].a2dp_connect != bt_profile_connect_status_success)){

        bt_profile_manager[id].has_connected = false;
        TRACE("BT disconnected!!!");

    #if _GFPS_
        if (app_gfps_is_last_response_pending())
        {
            app_gfps_enter_connectable_mode_req_handler(app_gfps_get_last_response());
        }
    #endif
    
#if defined(MEDIA_PLAYER_SUPPORT)&& !defined(IBRT)
        app_voice_report(APP_STATUS_INDICATION_DISCONNECTED, id);
#endif
#ifdef __INTERCONNECTION__
        app_interconnection_disconnected_callback();
#endif
   }

#ifdef __BT_ONE_BRING_TWO__
    app_bt_update_connectable_mode_after_connection_management();
#endif
}
#endif

void app_audio_switch_flash_flush_req(void);
//TODO: add dip features to check android or iphone
#ifdef BTIF_DIP_DEVICE
bool app_device_is_ios(hf_chan_handle_t hf_chan)
{
    btif_remote_device_t* currentRemDev = (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(hf_chan);
    return btif_dip_check_is_ios_device(currentRemDev);
}

bool app_bt_check_whether_ios_device_connected(void)
{
    uint8_t devId;
    for (devId = 0;devId < BT_DEVICE_NUM;devId++)
    {
        if (bt_profile_manager[devId].has_connected)
        {
            btif_remote_device_t* remDev = app_bt_get_remoteDev(devId);
            if (remDev)
            {
                if (btif_dip_check_is_ios_device(remDev))
                {
                    return true;
                }
            }
        }
    }
    return false;
}
#endif
//void app_bt_profile_connect_manager_hf(enum BT_DEVICE_ID_T id, HfChannel *Chan, HfCallbackParms *Info)
void app_bt_profile_connect_manager_hf(enum BT_DEVICE_ID_T id, hf_chan_handle_t Chan, struct hfp_context *ctx)
{
    //btdevice_profile *btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get((uint8_t *)Info->p.remDev->bdAddr.address);
    btdevice_profile *btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get((uint8_t *)ctx->remote_dev_bdaddr.address);

    osTimerStop(bt_profile_manager[id].connect_timer);
    bt_profile_manager[id].connect_timer_cb = NULL;
    //if (Chan&&Info){
    if (Chan){
        switch(ctx->event)
        {
            case BTIF_HF_EVENT_SERVICE_CONNECTED:
                TRACE("%s HF_EVENT_SERVICE_CONNECTED",__func__);
                btdevice_plf_p->hfp_act = true;
#ifndef FPGA
                nv_record_touch_cause_flush();
#endif
                bt_profile_manager[id].hfp_connect = bt_profile_connect_status_success;
                bt_profile_manager[id].saved_reconnect_mode =bt_profile_reconnect_null;
                bt_profile_manager[id].reconnect_cnt = 0;
                bt_profile_manager[id].chan = app_bt_device.hf_channel[id];
                memcpy(bt_profile_manager[id].rmt_addr.address, ctx->remote_dev_bdaddr.address, BTIF_BD_ADDR_SIZE);
                if (false == bt_profile_manager[id].has_connected)
                {
                    app_bt_resume_sniff_mode(id);
                }
                if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_openreconnecting){
                    //do nothing
                }
#if defined(IBRT)
                else if (app_tws_ibrt_is_reconnecting_mobile())
#else
                else if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_reconnecting)
#endif
                {
                    if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect != bt_profile_connect_status_success){
                        TRACE("!!!continue connect a2dp\n");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[id].has_connected);
                        app_bt_A2DP_OpenStream(bt_profile_manager[id].stream, &bt_profile_manager[id].rmt_addr);
                    }
                }
#ifdef __AUTO_CONNECT_OTHER_PROFILE__
                else{
                    //befor auto connect a2dp profile, check whether a2dp is supported
                    if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect != bt_profile_connect_status_success) {
                        bt_profile_manager[id].connect_timer_cb = app_bt_profile_connect_a2dp_retry_timehandler;
                        app_bt_accessmode_set(BAM_CONNECTABLE_ONLY);
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_CONNECT_RETRY_MS);
                    }
                }
#endif
                break;
            case BTIF_HF_EVENT_SERVICE_DISCONNECTED:
                //TRACE("%s HF_EVENT_SERVICE_DISCONNECTED discReason:%d/%d",__func__, Info->p.remDev->discReason, Info->p.remDev->discReason_saved);
                TRACE("%s HF_EVENT_SERVICE_DISCONNECTED discReason:%d/%d",__func__, ctx->disc_reason, ctx->disc_reason_saved);
                bt_profile_manager[id].hfp_connect = bt_profile_connect_status_failure;
                if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_openreconnecting){
                    if (++bt_profile_manager[id].reconnect_cnt < APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT){
                        app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                    #ifdef  __IAG_BLE_INCLUDE__
                        app_start_connectable_ble_adv(BLE_ADVERTISING_INTERVAL);
                    #endif
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
                        bt_profile_manager[id].hfp_connect = bt_profile_connect_status_unknow;
                    }
                }else if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_reconnecting){
                    if (++bt_profile_manager[id].reconnect_cnt < APP_BT_PROFILE_RECONNECT_RETRY_LIMIT_CNT){
                        app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                    #ifdef  __IAG_BLE_INCLUDE__
                        app_start_connectable_ble_adv(BLE_ADVERTISING_INTERVAL);
                    #endif
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
                    }else{
                        app_bt_restore_reconnecting_idle_mode(id);
                        // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
                    }
                    TRACE("%s try to reconnect cnt:%d",__func__, bt_profile_manager[id].reconnect_cnt);
                    /*
                }else if ((Info->p.remDev->discReason == 0x8)||
                          (Info->p.remDev->discReason_saved == 0x8)){
                          */
                }
#if !defined(IBRT)
                else if ((ctx->disc_reason == 0x8)||
                          (ctx->disc_reason_saved == 0x8)){
                    bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_reconnecting;
                    app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                    TRACE("%s try to reconnect",__func__);
                    if(app_bt_profile_reconnect_pending(id) != 0)
                    {
                    #ifdef  __IAG_BLE_INCLUDE__
                        app_start_connectable_ble_adv(BLE_ADVERTISING_INTERVAL);
                    #endif
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
                    }
                }
#endif
                else{
                    bt_profile_manager[id].hfp_connect = bt_profile_connect_status_unknow;
                }
                break;
            default:
                break;
        }
    }

    if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_reconnecting){
        bool reconnect_hfp_proc_final = true;
        bool reconnect_a2dp_proc_final = true;
        if (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_failure){
            reconnect_hfp_proc_final = false;
        }
        if (bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_failure){
            reconnect_a2dp_proc_final = false;
        }
        if (reconnect_hfp_proc_final && reconnect_a2dp_proc_final){
            TRACE("!!!reconnect success %d/%d/%d\n", bt_profile_manager[id].hfp_connect, bt_profile_manager[id].hsp_connect, bt_profile_manager[id].a2dp_connect);
            app_bt_restore_reconnecting_idle_mode(id);
            // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
        }
    }else if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_openreconnecting){
        bool opening_hfp_proc_final = false;
        bool opening_a2dp_proc_final = false;

        if (btdevice_plf_p->hfp_act && bt_profile_manager[id].hfp_connect == bt_profile_connect_status_unknow){
            opening_hfp_proc_final = false;
        }else{
            opening_hfp_proc_final = true;
        }

        if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_unknow){
            opening_a2dp_proc_final = false;
        }else{
            opening_a2dp_proc_final = true;
        }

        if ((opening_hfp_proc_final && opening_a2dp_proc_final) ||
            (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_failure)){
            TRACE("!!!reconnect success %d/%d/%d\n", bt_profile_manager[id].hfp_connect, bt_profile_manager[id].hsp_connect, bt_profile_manager[id].a2dp_connect);
            bt_profile_manager[id].saved_reconnect_mode =  bt_profile_reconnect_openreconnecting;
            app_bt_restore_reconnecting_idle_mode(id);
            // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
        }

        if (btdevice_plf_p->hfp_act && bt_profile_manager[id].hfp_connect == bt_profile_connect_status_success){
            if (btdevice_plf_p->a2dp_act && !opening_a2dp_proc_final){
                TRACE("!!!continue connect a2dp\n");
                app_bt_precheck_before_starting_connecting(bt_profile_manager[id].has_connected);
                app_bt_A2DP_OpenStream(bt_profile_manager[id].stream, &bt_profile_manager[id].rmt_addr);
            }
        }

        if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_null){
            for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
                if (bt_profile_manager[i].reconnect_mode == bt_profile_reconnect_openreconnecting){
                    TRACE("!!!hf->start reconnect second device\n");
                    btdevice_profile *btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get((uint8_t *)bt_profile_manager[i].rmt_addr.address);
                    if ((btdevice_plf_p->hfp_act)&&(!bt_profile_manager[i].hfp_connect)){
                        TRACE("try connect hf");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[id].has_connected);
                        app_bt_HF_CreateServiceLink(bt_profile_manager[i].chan, (bt_bdaddr_t *)&bt_profile_manager[i].rmt_addr);
                    }
#if defined (__HSP_ENABLE__)
                    else if((btdevice_plf_p->hsp_act)&&(!bt_profile_manager[i].hsp_connect)) {
                        TRACE("try connect hs");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[id].has_connected);
                        app_bt_HS_CreateServiceLink(bt_profile_manager[i].hs_chan, &bt_profile_manager[i].rmt_addr);
                    }
#endif
                    else if((btdevice_plf_p->a2dp_act)&&(!bt_profile_manager[i].a2dp_connect)) {
                        TRACE("try connect a2dp");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[id].has_connected);
                        app_bt_A2DP_OpenStream(bt_profile_manager[i].stream, &bt_profile_manager[i].rmt_addr);
                    }
                    break;
                }
            }
        }
    }

    if (!bt_profile_manager[id].has_connected &&
        (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_success ||
#ifdef __HSP_ENABLE__
         bt_profile_manager[id].hsp_connect == bt_profile_connect_status_success||
#endif
         bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_success)){

        bt_profile_manager[id].has_connected = true;
        TRACE("BT connected!!!");
        btif_me_get_remote_device_name(&(ctx->remote_dev_bdaddr), app_bt_global_handle);

#if defined(MEDIA_PLAYER_SUPPORT)&& !defined(IBRT)
        app_voice_report(APP_STATUS_INDICATION_CONNECTED, id);
#endif
#ifdef __AI_VOICE__
    //TODO: dip check android or iphone
#ifdef BTIF_DIP_DEVICE
    if (app_device_is_ios(Chan))
#endif
    {
        // restart ble adv to add the ai uuid
        app_ai_set_ble_adv_uuid(true);
    }
#endif
#ifdef __INTERCONNECTION__
        app_interconnection_start_disappear_adv(BLE_ADVERTISING_INTERVAL, APP_INTERCONNECTION_DISAPPEAR_ADV_IN_MS);
        app_interconnection_spp_open();
#endif
    }

    if (bt_profile_manager[id].has_connected &&
        (bt_profile_manager[id].hfp_connect != bt_profile_connect_status_success &&
#ifdef __HSP_ENABLE__
         bt_profile_manager[id].hsp_connect != bt_profile_connect_status_success &&
#endif
         bt_profile_manager[id].a2dp_connect != bt_profile_connect_status_success)){

        bt_profile_manager[id].has_connected = false;
        TRACE("BT disconnected!!!");
#ifdef __AI_VOICE__
        // TODO: should check whether the disconnected device is an ama phone
#ifdef IS_MULTI_AI_ENABLED
        if (AI_SPEC_GSOUND != ai_manager_get_current_spec())
#endif
        {
            //TODO: dip check android or iphone
#ifdef BTIF_DIP_DEVICE
            if (!app_bt_check_whether_ios_device_connected())
#endif
			{
                app_ai_set_ble_adv_uuid(false);
            }
        }
#endif

#if _GFPS_
    if (app_gfps_is_last_response_pending())
    {
        app_gfps_enter_connectable_mode_req_handler(app_gfps_get_last_response());
    }
#endif

#if defined(MEDIA_PLAYER_SUPPORT)&& !defined(IBRT)
        app_voice_report(APP_STATUS_INDICATION_DISCONNECTED, id);
#endif
#ifdef __INTERCONNECTION__
        app_interconnection_disconnected_callback();
#endif
   }

#ifdef __BT_ONE_BRING_TWO__
    app_bt_update_connectable_mode_after_connection_management();
#endif
}

void app_bt_profile_connect_manager_a2dp(enum BT_DEVICE_ID_T id, a2dp_stream_t *Stream, const   a2dp_callback_parms_t *info)
{
    btdevice_profile *btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(
        (uint8_t *)  btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream))->address);
    btif_a2dp_callback_parms_t* Info = (btif_a2dp_callback_parms_t*)info;
    osTimerStop(bt_profile_manager[id].connect_timer);
    bt_profile_manager[id].connect_timer_cb = NULL;

    if (Stream&&Info){

        switch(Info->event)
        {
            case BTIF_A2DP_EVENT_STREAM_OPEN:
                TRACE("%s A2DP_EVENT_STREAM_OPEN,codec type=%x",__func__,Info->p.configReq->codec.codecType);

                btdevice_plf_p->a2dp_act = true;
                btdevice_plf_p->a2dp_codectype = Info->p.configReq->codec.codecType;
#ifndef FPGA
                nv_record_touch_cause_flush();
#endif
                if(bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_success)
               {

                    TRACE("!!!a2dp has opened   force return ");
                     return;
                }
                bt_profile_manager[id].a2dp_connect = bt_profile_connect_status_success;
                bt_profile_manager[id].reconnect_cnt = 0;
                bt_profile_manager[id].stream = app_bt_device.a2dp_connected_stream[id];
                memcpy(bt_profile_manager[id].rmt_addr.address,  btif_me_get_remote_device_bdaddr(btif_a2dp_get_stream_conn_remDev(Stream))->address, BTIF_BD_ADDR_SIZE);
                app_bt_record_latest_connected_service_device_id(id);
                if (false == bt_profile_manager[id].has_connected)
                {
                    app_bt_resume_sniff_mode(id);
                }
                if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_openreconnecting){

                }else if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_reconnecting){
                    if (btdevice_plf_p->hfp_act&& bt_profile_manager[id].hfp_connect != bt_profile_connect_status_success){
                        TRACE("!!!continue connect hfp\n");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[id].has_connected);
                        app_bt_HF_CreateServiceLink(bt_profile_manager[id].chan, (bt_bdaddr_t *)&bt_profile_manager[id].rmt_addr);
                    }
#if defined (__HSP_ENABLE__)
                    else if(btdevice_plf_p->hsp_act&& bt_profile_manager[id].hsp_connect != bt_profile_connect_status_success){
                        TRACE("!!!continue connect hsp\n");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[id].has_connected);
                        app_bt_HS_CreateServiceLink(bt_profile_manager[id].hs_chan, &bt_profile_manager[id].rmt_addr);
                    }
#endif
                }
#ifdef __AUTO_CONNECT_OTHER_PROFILE__
                else{
                    if(btdevice_plf_p->hfp_act && bt_profile_manager[id].hfp_connect != bt_profile_connect_status_success)
                    {
                        bt_profile_manager[id].connect_timer_cb = app_bt_profile_connect_hf_retry_timehandler;
                        app_bt_accessmode_set(BAM_CONNECTABLE_ONLY);
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_CONNECT_RETRY_MS);
                    }
#if defined (__HSP_ENABLE__)
                    else if(btdevice_plf_p->hsp_act && bt_profile_manager[id].hsp_connect != bt_profile_connect_status_success)
                    {
                        bt_profile_manager[id].connect_timer_cb = app_bt_profile_connect_hs_retry_timehandler;
                        app_bt_accessmode_set(BAM_CONNECTABLE_ONLY);
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_CONNECT_RETRY_MS);
                    }
#endif
                }
#endif
#ifdef APP_DISABLE_PAGE_SCAN_AFTER_CONN
                disable_page_scan_check_timer_start();
#endif
                break;
            case BTIF_A2DP_EVENT_STREAM_CLOSED:

                TRACE("%s A2DP_EVENT_STREAM_CLOSED discReason:%d/%d",__func__, Info->discReason,
                    btif_me_get_remote_device_disc_reason_saved(btif_a2dp_get_remote_device(Stream)));
                bt_profile_manager[id].a2dp_connect = bt_profile_connect_status_failure;

                if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_openreconnecting){
                   if (++bt_profile_manager[id].reconnect_cnt < APP_BT_PROFILE_OPENNING_RECONNECT_RETRY_LIMIT_CNT){
                       app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                   #ifdef __IAG_BLE_INCLUDE__
                       app_start_connectable_ble_adv(BLE_ADVERTISING_INTERVAL);
                   #endif
                       osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
                       bt_profile_manager[id].a2dp_connect = bt_profile_connect_status_unknow;
                   }
                }else if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_reconnecting){
                   if (++bt_profile_manager[id].reconnect_cnt < APP_BT_PROFILE_RECONNECT_RETRY_LIMIT_CNT){
                       app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                   #ifdef __IAG_BLE_INCLUDE__
                       app_start_connectable_ble_adv(BLE_ADVERTISING_INTERVAL);
                   #endif
                       osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
                   }else{
                       app_bt_restore_reconnecting_idle_mode(id);
                       // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
                   }
                   TRACE("%s try to reconnect cnt:%d",__func__, bt_profile_manager[id].reconnect_cnt);
                }
#if !defined(IBRT)
                else if(((Info->discReason == 0x8)||
                           ( btif_me_get_remote_device_disc_reason_saved( btif_a2dp_get_remote_device(Stream)) == 0x8)) &&
                          (btdevice_plf_p->a2dp_act)&&
                          (!btdevice_plf_p->hfp_act) &&
                          (!btdevice_plf_p->hsp_act)){
                    bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_reconnecting;
                    TRACE("%s try to reconnect cnt:%d",__func__, bt_profile_manager[id].reconnect_cnt);
                    app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
                    if(app_bt_profile_reconnect_pending(id) != 0)
                    {
                    #ifdef __IAG_BLE_INCLUDE__
                        app_start_connectable_ble_adv(BLE_ADVERTISING_INTERVAL);
                    #endif
                        osTimerStart(bt_profile_manager[id].connect_timer, APP_BT_PROFILE_RECONNECT_RETRY_INTERVAL_MS);
                    }
                }
#endif
                else{
                    bt_profile_manager[id].a2dp_connect = bt_profile_connect_status_unknow;
               }
               break;
            default:
                break;
        }
    }

    if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_reconnecting){
        bool reconnect_hfp_proc_final = true;
        bool reconnect_a2dp_proc_final = true;
        if (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_failure){
            reconnect_hfp_proc_final = false;
        }
#if defined (__HSP_ENABLE__)
        if(btdevice_plf_p->hsp_act !=0){    //has HSP
            reconnect_hfp_proc_final = true;
            if (bt_profile_manager[id].hsp_connect == bt_profile_connect_status_failure){
                reconnect_hfp_proc_final = false;
            }
        }
#endif
        if (bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_failure){
            reconnect_a2dp_proc_final = false;
        }
        if (reconnect_hfp_proc_final && reconnect_a2dp_proc_final){
            TRACE("!!!reconnect success %d/%d/%d\n", bt_profile_manager[id].hfp_connect, bt_profile_manager[id].hsp_connect, bt_profile_manager[id].a2dp_connect);
            app_bt_restore_reconnecting_idle_mode(id);
            // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
        }
    }else if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_openreconnecting){
        bool opening_hfp_proc_final = false;
        bool opening_a2dp_proc_final = false;

        if (btdevice_plf_p->hfp_act && bt_profile_manager[id].hfp_connect == bt_profile_connect_status_unknow){
            opening_hfp_proc_final = false;
        }else{
            opening_hfp_proc_final = true;
        }

        if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_unknow){
            opening_a2dp_proc_final = false;
        }else{
            opening_a2dp_proc_final = true;
        }

        if ((opening_hfp_proc_final && opening_a2dp_proc_final) ||
            (bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_failure)){
            TRACE("!!!reconnect success %d/%d/%d\n", bt_profile_manager[id].hfp_connect, bt_profile_manager[id].hsp_connect, bt_profile_manager[id].a2dp_connect);
            app_bt_restore_reconnecting_idle_mode(id);
            // bt_profile_manager[id].reconnect_mode = bt_profile_reconnect_null;
        }

        if (btdevice_plf_p->a2dp_act && bt_profile_manager[id].a2dp_connect== bt_profile_connect_status_success){
            if (btdevice_plf_p->hfp_act && !opening_hfp_proc_final){
                TRACE("!!!continue connect hf\n");
                app_bt_precheck_before_starting_connecting(bt_profile_manager[id].has_connected);
                app_bt_HF_CreateServiceLink(bt_profile_manager[id].chan, (bt_bdaddr_t *)&bt_profile_manager[id].rmt_addr);
            }
#if defined (__HSP_ENABLE)
            else if(btdevice_plf_p->hsp_act && !opening_hfp_hsp_proc_final){
                TRACE("!!!continue connect hs\n");
                app_bt_precheck_before_starting_connecting(bt_profile_manager[id].has_connected);
                app_bt_HS_CreateServiceLink(bt_profile_manager[id].hs_chan, &bt_profile_manager[id].rmt_addr);
            }
#endif
        }

        if (bt_profile_manager[id].reconnect_mode == bt_profile_reconnect_null){
            for (uint8_t i=0; i<BT_DEVICE_NUM; i++){
                if (bt_profile_manager[i].reconnect_mode == bt_profile_reconnect_openreconnecting){
                    TRACE("!!!a2dp->start reconnect device %d\n", i);
                    if ((btdevice_plf_p->hfp_act)&&(!bt_profile_manager[i].hfp_connect)){
                        TRACE("try connect hf");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[i].has_connected);
                        app_bt_HF_CreateServiceLink(bt_profile_manager[i].chan, (bt_bdaddr_t *)&bt_profile_manager[i].rmt_addr);
                    }
#if defined (__HSP_ENABLE__)
                    else if((btdevice_plf_p->hsp_act)&&(!bt_profile_manager[i].hsp_connect)) {
                        TRACE("try connect hs");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[i].has_connected);
                        app_bt_HS_CreateServiceLink(bt_profile_manager[i].hs_chan, &bt_profile_manager[i].rmt_addr);
                    }
#endif
                    else if((btdevice_plf_p->a2dp_act)&&(!bt_profile_manager[i].a2dp_connect)) {
                        TRACE("try connect a2dp");
                        app_bt_precheck_before_starting_connecting(bt_profile_manager[i].has_connected);
                        app_bt_A2DP_OpenStream(bt_profile_manager[i].stream, &bt_profile_manager[i].rmt_addr);
                    }
                    break;
                }
            }
        }
    }

    if (!bt_profile_manager[id].has_connected &&
        (bt_profile_manager[id].hfp_connect == bt_profile_connect_status_success ||
#ifdef __HSP_ENABLE__
        bt_profile_manager[id].hsp_connect == bt_profile_connect_status_success||
#endif
        bt_profile_manager[id].a2dp_connect == bt_profile_connect_status_success)){

        bt_profile_manager[id].has_connected = true;
        TRACE("BT connected!!!");

        
        btif_me_get_remote_device_name(&(bt_profile_manager[id].rmt_addr), app_bt_global_handle);
#if defined(MEDIA_PLAYER_SUPPORT)&& defined(IBRT)
        app_voice_report(APP_STATUS_INDICATION_CONNECTED, id);
#endif
#ifdef __AI_VOICE__
        //TODO: dip check android or iphone
#ifdef BTIF_DIP_DEVICE
        if (btif_dip_check_is_ios_device(btif_a2dp_get_stream_conn_remDev(Stream)))
#endif
        {
            app_ai_set_ble_adv_uuid(true);
        }
#endif
#ifdef __INTERCONNECTION__
        app_interconnection_start_disappear_adv(BLE_ADVERTISING_INTERVAL, APP_INTERCONNECTION_DISAPPEAR_ADV_IN_MS);
        app_interconnection_spp_open();
#endif
    }

    if (bt_profile_manager[id].has_connected &&
        (bt_profile_manager[id].hfp_connect != bt_profile_connect_status_success &&
#ifdef __HSP_ENABLE__
         bt_profile_manager[id].hsp_connect != bt_profile_connect_status_success &&
#endif
         bt_profile_manager[id].a2dp_connect != bt_profile_connect_status_success)){

        bt_profile_manager[id].has_connected = false;
        TRACE("BT disconnected!!!");
#ifdef __AI_VOICE__
#ifdef IS_MULTI_AI_ENABLED
        if (AI_SPEC_GSOUND != ai_manager_get_current_spec())
#endif
        {
             //TODO: dip check android or iphone
#ifdef BTIF_DIP_DEVICE
            if (!app_bt_check_whether_ios_device_connected())
#endif
            {
                app_ai_set_ble_adv_uuid(false);
            }
        }
#endif

#if _GFPS_
    if (app_gfps_is_last_response_pending())
    {
        app_gfps_enter_connectable_mode_req_handler(app_gfps_get_last_response());
    }
#endif

#if defined(MEDIA_PLAYER_SUPPORT)&& !defined(IBRT)
        app_voice_report(APP_STATUS_INDICATION_DISCONNECTED, id);
#endif
#ifdef __INTERCONNECTION__
        app_interconnection_disconnected_callback();
#endif
    }

#ifdef __BT_ONE_BRING_TWO__
    app_bt_update_connectable_mode_after_connection_management();
#endif
}

#ifdef BTIF_HID_DEVICE
void hid_exit_shutter_mode(void);
#endif

bt_status_t DeviceLinkDisconnectDirectly(uint8_t device_id)
{
    osapi_lock_stack();

    //TRACE("%s id1 hf:%d a2dp:%d",__func__, app_bt_device.hf_channel[device_id].state, btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[device_id]->a2dp_stream));
    TRACE("%s id1 hf:%d a2dp:%d",__func__, btif_get_hf_chan_state(app_bt_device.hf_channel[device_id]), btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[device_id]->a2dp_stream));

    //if(app_bt_device.hf_channel[device_id].state == HF_STATE_OPEN){
    if( btif_get_hf_chan_state(app_bt_device.hf_channel[device_id]) == BTIF_HF_STATE_OPEN){
        app_bt_HF_DisconnectServiceLink(app_bt_device.hf_channel[device_id]);
    }
#if defined (__HSP_ENABLE__)
    if(app_bt_device.hs_channel[device_id].state == HS_STATE_OPEN){
        app_bt_HS_DisconnectServiceLink(&app_bt_device.hs_channel[device_id]);
    }
#endif
    if(btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[device_id]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[device_id]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_OPEN){
        app_bt_A2DP_CloseStream(app_bt_device.a2dp_stream[device_id]->a2dp_stream);
    }
#if defined(A2DP_AAC_ON)
    if( btif_a2dp_get_stream_state(app_bt_device.a2dp_aac_stream[device_id]->a2dp_stream)  == BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(app_bt_device.a2dp_aac_stream[device_id]->a2dp_stream)  == BTIF_AVDTP_STRM_STATE_OPEN){
        app_bt_A2DP_CloseStream(app_bt_device.a2dp_aac_stream[device_id]->a2dp_stream);
    }
#endif
#if defined(A2DP_SCALABLE_ON)
    if(btif_a2dp_get_stream_state(app_bt_device.a2dp_scalable_stream[device_id]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state(app_bt_device.a2dp_scalable_stream[device_id]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_OPEN){
        app_bt_A2DP_CloseStream(app_bt_device.a2dp_scalable_stream[device_id]->a2dp_stream);
    }
#endif

    if(btif_avrcp_get_remote_device(app_bt_device.avrcp_channel[device_id]->avrcp_channel_handle))
    {
        btif_avrcp_disconnect(app_bt_device.avrcp_channel[device_id]->avrcp_channel_handle);
    }

    btif_remote_device_t* remDev = app_bt_get_remoteDev(device_id);
#ifdef VOICE_DATAPATH
    if (NULL != remDev)
    {
        if (app_voicepath_is_btaddr_connected(btif_me_get_remote_device_bdaddr(remDev)->address))
        {
            app_voicepath_disconnect_bt();
            app_voicepath_disconnect_ble();
        }
    }
#endif
#ifdef __AI_VOICE__
    if (NULL != remDev)
    {
        //app_ai_spp_disconnlink();
        app_ai_disconnect_ble();
    }
#endif

    osapi_unlock_stack();

    osDelay(500);

    if (NULL != remDev)
    {
        app_bt_MeDisconnectLink(remDev);
    }
    return BT_STS_SUCCESS;
}

extern "C" void appm_disconnect_all(void);

bt_status_t LinkDisconnectDirectly(void)
{
    //TRACE("osapi_lock_is_exist:%d",osapi_lock_is_exist());
    if(osapi_lock_is_exist())
        osapi_lock_stack();

    TRACE("activeCons:%d", btif_me_get_activeCons());
#ifdef __IAG_BLE_INCLUDE__
    TRACE("ble_connected_state:%d", app_ble_is_in_connected_state());
#endif
    uint8_t Tmp_activeCons = btif_me_get_activeCons();

    if(Tmp_activeCons)
    {
        //TRACE("%s id1 hf:%d a2dp:%d",__func__, app_bt_device.hf_channel[BT_DEVICE_ID_1].state, btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream));
        TRACE("%s id1 hf:%d a2dp:%d",__func__,  btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_1]), btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream));
#ifdef BTIF_HID_DEVICE
        hid_exit_shutter_mode();
#endif
        if (btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_1]) == BTIF_HF_STATE_OPEN){
            app_bt_HF_DisconnectServiceLink(app_bt_device.hf_channel[BT_DEVICE_ID_1]);
        }
#if defined (__HSP_ENABLE__)
        if(app_bt_device.hs_channel[BT_DEVICE_ID_1].state == HS_STATE_OPEN){
            app_bt_HS_DisconnectServiceLink(&app_bt_device.hs_channel[BT_DEVICE_ID_1]);
        }
#endif   //btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[device_id]->a2dp_stream)
        if(btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_STREAMING ||
            btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_OPEN){
            app_bt_A2DP_CloseStream(app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream);
        }
#if defined(A2DP_LHDC_ON)
        if(btif_a2dp_get_stream_state(app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_STREAMING ||
            btif_a2dp_get_stream_state(app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_OPEN){
            app_bt_A2DP_CloseStream(app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_1]->a2dp_stream);
        }
#endif
#if defined(A2DP_LDAC_ON)
       if(btif_a2dp_get_stream_state(app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_STREAMING ||
            btif_a2dp_get_stream_state(app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_OPEN){
            app_bt_A2DP_CloseStream(app_bt_device.a2dp_ldac_stream[BT_DEVICE_ID_1]->a2dp_stream);
       }
#endif

#if defined(A2DP_AAC_ON)
        if(btif_a2dp_get_stream_state( app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_STREAMING ||
            btif_a2dp_get_stream_state( app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_OPEN){
            app_bt_A2DP_CloseStream(app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream);
        }
#endif
#if defined(A2DP_SCALABLE_ON)
        if(btif_a2dp_get_stream_state( app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_STREAMING ||
            btif_a2dp_get_stream_state(app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_OPEN){
            app_bt_A2DP_CloseStream(app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_1]->a2dp_stream);
        }
#endif
        if( btif_avrcp_get_remote_device(app_bt_device.avrcp_channel[BT_DEVICE_ID_1]->avrcp_channel_handle))
        {
            btif_avrcp_disconnect(app_bt_device.avrcp_channel[BT_DEVICE_ID_1]->avrcp_channel_handle);
        }
#ifdef __BT_ONE_BRING_TWO__
            TRACE("%s id2 hf:%d a2dp:%d",__func__, btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_2]), btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[BT_DEVICE_ID_2]->a2dp_stream));
            //if(app_bt_device.hf_channel[BT_DEVICE_ID_2].state == HF_STATE_OPEN){
            if(btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_2]) == BTIF_HF_STATE_OPEN){
                app_bt_HF_DisconnectServiceLink(app_bt_device.hf_channel[BT_DEVICE_ID_2]);
            }

#if defined (__HSP_ENABLE__)
            if(app_bt_device.hs_channel[BT_DEVICE_ID_2].state == HS_STATE_OPEN){
                app_bt_HS_DisconnectServiceLink(&app_bt_device.hs_channel[BT_DEVICE_ID_2]);
            }
#endif // __HSP_ENABLE__

        if( btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[BT_DEVICE_ID_2]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_STREAMING ||
            btif_a2dp_get_stream_state(app_bt_device.a2dp_stream[BT_DEVICE_ID_2]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_OPEN){
            app_bt_A2DP_CloseStream(app_bt_device.a2dp_stream[BT_DEVICE_ID_2]->a2dp_stream);
        }
#if defined(A2DP_LHDC_ON)
        if(btif_a2dp_get_stream_state(app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_2]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_STREAMING ||
            btif_a2dp_get_stream_state(app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_2]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_OPEN){
            app_bt_A2DP_CloseStream(app_bt_device.a2dp_lhdc_stream[BT_DEVICE_ID_2]->a2dp_stream);
        }
#endif // A2DP_LHDC_ON
#if defined(A2DP_AAC_ON)
        if(btif_a2dp_get_stream_state(app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_2]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_STREAMING ||
            btif_a2dp_get_stream_state(app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_2]->a2dp_stream)== BTIF_AVDTP_STRM_STATE_OPEN){
            app_bt_A2DP_CloseStream(app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_2]->a2dp_stream);
        }
#endif // A2DP_AAC_ON
#if defined(A2DP_SCALABLE_ON)
            if(btif_a2dp_get_stream_state(app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_2]->a2dp_stream)== BTIF_AVDTP_STRM_STATE_STREAMING ||
                    btif_a2dp_get_stream_state(app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_2]->a2dp_stream) == BTIF_AVDTP_STRM_STATE_OPEN){
                app_bt_A2DP_CloseStream(app_bt_device.a2dp_scalable_stream[BT_DEVICE_ID_2]->a2dp_stream);
            }
#endif // A2DP_SCALABLE_ON
        if( btif_avrcp_get_remote_device(app_bt_device.avrcp_channel[BT_DEVICE_ID_2]->avrcp_channel_handle))    {
            btif_avrcp_disconnect(app_bt_device.avrcp_channel[BT_DEVICE_ID_2]->avrcp_channel_handle);
        }
#endif //__BT_ONE_BRING_TWO__

#ifdef VOICE_DATAPATH
        app_voicepath_disconnect_bt();
#endif // VOICE_DATAPATH
#ifdef __AI_VOICE__
        //app_ai_spp_disconnlink();
#endif // __AI_VOICE__
    }

#ifdef __IAG_BLE_INCLUDE__
    if(app_ble_is_in_connected_state())
    {
#ifdef _GFPS_
        if (!app_gfps_is_last_response_pending())
#endif
        appm_disconnect_all();
    }
#endif

    if(osapi_lock_is_exist())
        osapi_unlock_stack();

    osDelay(500);

    if(Tmp_activeCons)
    {
        btif_remote_device_t* remDev = app_bt_get_remoteDev(BT_DEVICE_ID_1);
        if (NULL != remDev)
        {
            app_bt_MeDisconnectLink(remDev);
        }

#ifdef __BT_ONE_BRING_TWO__
        remDev = app_bt_get_remoteDev(BT_DEVICE_ID_2);
        if (NULL != remDev)
        {
            osDelay(200);
            app_bt_MeDisconnectLink(remDev);
        }
#endif
    }

    //if(osapi_lock_is_exist())
        //osapi_unlock_stack();

    return BT_STS_SUCCESS;
}

void app_disconnect_all_bt_connections(void)
{
    LinkDisconnectDirectly();
}

void app_bt_init(void)
{
    app_bt_mail_init();
    app_set_threadhandle(APP_MODUAL_BT, app_bt_handle_process);
    btif_me_sec_set_io_cap_rsp_reject_ext(app_bt_profile_connect_openreconnecting);
}

extern "C" bool app_bt_has_connectivitys(void)
{
    int activeCons;
    osapi_lock_stack();
    activeCons = btif_me_get_activeCons();
    osapi_unlock_stack();

    if(activeCons > 0)
        return true;

    return false;
#if 0
    if(app_bt_device.hf_channel[BT_DEVICE_ID_1].cmgrHandler.remDev)
        return true;
    if(app_bt_device.a2dp_stream[BT_DEVICE_ID_1].device->cmgrHandler.remDev)
        return true;
#ifdef __BT_ONE_BRING_TWO__
    if(app_bt_device.hf_channel[BT_DEVICE_ID_2].cmgrHandler.remDev)
        return true;
    if(app_bt_device.a2dp_stream[BT_DEVICE_ID_2].device->cmgrHandler.remDev)
        return true;
#endif
    return false;
#endif
}


#ifdef __TWS_CHARGER_BOX__

extern "C" {
    bt_status_t ME_Ble_Clear_Whitelist(void);
    bt_status_t ME_Ble_Set_Private_Address(BT_BD_ADDR *addr);
    bt_status_t ME_Ble_Add_Dev_To_Whitelist(U8 addr_type,BT_BD_ADDR *addr);
    bt_status_t ME_Ble_SetAdv_data(U8 len, U8 *data);
    bt_status_t ME_Ble_SetAdv_parameters(adv_para_struct *para);
    bt_status_t ME_Ble_SetAdv_en(U8 en);
    bt_status_t ME_Ble_Setscan_parameter(scan_para_struct *para);
    bt_status_t ME_Ble_Setscan_en(U8 scan_en,  U8 filter_duplicate);
}


int8_t power_level=0;
#define TWS_BOX_OPEN 1
#define TWS_BOX_CLOSE 0
void app_tws_box_set_slave_adv_data(uint8_t power_level,uint8_t box_status)
{
    uint8_t adv_data[] = {
        0x02,0xfe, 0x00,
        0x02, 0xfd, 0x00  // manufacturer data
    };

    adv_data[2] = power_level;

    adv_data[5] = box_status;
    ME_Ble_SetAdv_data(sizeof(adv_data), adv_data);

}


void app_tws_box_set_slave_adv_para(void)
{
    uint8_t  peer_addr[BTIF_BD_ADDR_SIZE] = {0};
    adv_para_struct para;


    para.interval_min =             0x0040; // 20ms
    para.interval_max =             0x0040; // 20ms
    para.adv_type =                 0x03;
    para.own_addr_type =            0x01;
    para.peer_addr_type =           0x01;
    para.adv_chanmap =              0x07;
    para.adv_filter_policy =        0x00;
    memcpy(para.bd_addr.addr, peer_addr, BTIF_BD_ADDR_SIZE);

    ME_Ble_SetAdv_parameters(&para);

}


extern uint8_t bt_addr[6];
void app_tws_start_chargerbox_adv(void)
{
    app_tws_box_set_slave_adv_data(power_level,TWS_BOX_OPEN);
    ME_Ble_Set_Private_Address((BT_BD_ADDR *)bt_addr);
    app_tws_box_set_slave_adv_para();
    ME_Ble_SetAdv_en(1);

}



#endif

bool app_is_hfp_service_connected(void)
{
    return (bt_profile_manager[BT_DEVICE_ID_1].hfp_connect == bt_profile_connect_status_success);
}


btif_remote_device_t* app_bt_get_remoteDev(uint8_t deviceId)
{
    btif_remote_device_t* currentRemDev = NULL;

    if(btif_a2dp_get_stream_state( app_bt_device.a2dp_stream[deviceId]->a2dp_stream)
        == BTIF_AVDTP_STRM_STATE_STREAMING ||
        btif_a2dp_get_stream_state( app_bt_device.a2dp_stream[deviceId]->a2dp_stream)
        == BTIF_AVDTP_STRM_STATE_OPEN)
    {
        currentRemDev = btif_a2dp_get_stream_conn_remDev(app_bt_device.a2dp_stream[deviceId]->a2dp_stream);
    }
    else if (btif_get_hf_chan_state(app_bt_device.hf_channel[deviceId]) == BTIF_HF_STATE_OPEN)
    {
        currentRemDev = (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(app_bt_device.hf_channel[deviceId]);
    }

    TRACE("%s get current Remdev 0x%x", __FUNCTION__, currentRemDev);

    return currentRemDev;
}

void app_bt_stay_active_rem_dev(btif_remote_device_t* pRemDev)
{
    if (pRemDev)
    {
        btif_cmgr_handler_t    *cmgrHandler;
        /* Clear the sniff timer */
        cmgrHandler = btif_cmgr_get_acl_handler(pRemDev);
        btif_cmgr_clear_sniff_timer(cmgrHandler);
        btif_cmgr_disable_sniff_timer(cmgrHandler);
        app_bt_Me_SetLinkPolicy(pRemDev, BTIF_BLP_MASTER_SLAVE_SWITCH);
    }
}

void app_bt_stay_active(uint8_t deviceId)
{
    btif_remote_device_t* currentRemDev = app_bt_get_remoteDev(deviceId);
    app_bt_stay_active_rem_dev(currentRemDev);
}

void app_bt_allow_sniff_rem_dev(btif_remote_device_t* pRemDev)
{
    if (pRemDev && (BTIF_BDS_CONNECTED ==  btif_me_get_remote_device_state(pRemDev)))
    {
        btif_cmgr_handler_t    *cmgrHandler;
        /* Enable the sniff timer */
        cmgrHandler = btif_cmgr_get_acl_handler(pRemDev);

        /* Start the sniff timer */
        btif_sniff_info_t sniffInfo;
        sniffInfo.minInterval = BTIF_CMGR_SNIFF_MIN_INTERVAL;
        sniffInfo.maxInterval = BTIF_CMGR_SNIFF_MAX_INTERVAL;
        sniffInfo.attempt = BTIF_CMGR_SNIFF_ATTEMPT;
        sniffInfo.timeout = BTIF_CMGR_SNIFF_TIMEOUT;
        if (cmgrHandler){
            btif_cmgr_set_sniff_timer(cmgrHandler, &sniffInfo, BTIF_CMGR_SNIFF_TIMER);
        }
        app_bt_Me_SetLinkPolicy(pRemDev, BTIF_BLP_MASTER_SLAVE_SWITCH | BTIF_BLP_SNIFF_MODE);
    }
}

extern "C" uint8_t is_sco_mode (void);
void app_bt_allow_sniff(uint8_t deviceId)
{
    if (a2dp_is_music_ongoing() || is_sco_mode())
    {
        return;
    }
    btif_remote_device_t* currentRemDev = app_bt_get_remoteDev(deviceId);
    app_bt_allow_sniff_rem_dev(currentRemDev);
}

void app_bt_stop_sniff(uint8_t deviceId)
{
    btif_remote_device_t* currentRemDev = app_bt_get_remoteDev(deviceId);

    if (currentRemDev && (btif_me_get_remote_device_state(currentRemDev) == BTIF_BDS_CONNECTED)){
        if (btif_me_get_current_mode(currentRemDev) == BTIF_BLM_SNIFF_MODE){
            TRACE("!!! stop sniff currmode:%d\n", btif_me_get_current_mode(currentRemDev));
            app_bt_ME_StopSniff(currentRemDev);
        }
    }
}

bool app_bt_is_device_connected(uint8_t deviceId)
{
    if (deviceId < BT_DEVICE_NUM) {
        return bt_profile_manager[deviceId].has_connected;
    } else {
        // Indicate no connection is user passes invalid deviceId
        return false;
    }
}

#if defined(__BT_SELECT_PROF_DEVICE_ID__)
int8_t app_bt_a2dp_is_same_stream(a2dp_stream_t *src_Stream, a2dp_stream_t *dst_Stream)
{
    return btif_a2dp_is_register_codec_same(src_Stream, dst_Stream);
}
void app_bt_a2dp_find_same_unused_stream(a2dp_stream_t *in_Stream, a2dp_stream_t **out_Stream, uint32_t device_id)
{
    *out_Stream = NULL;
    if (app_bt_a2dp_is_same_stream(app_bt_device.a2dp_stream[device_id]->a2dp_stream, in_Stream))
        *out_Stream = app_bt_device.a2dp_stream[device_id]->a2dp_stream;
#if defined(A2DP_LHDC_ON)
    else if (app_bt_a2dp_is_same_stream(app_bt_device.a2dp_lhdc_stream[device_id]->a2dp_stream, in_Stream))
        *out_Stream = app_bt_device.a2dp_lhdc_stream[device_id]->a2dp_stream;
#endif
#if defined(A2DP_LDAC_ON)
    else if (app_bt_a2dp_is_same_stream(app_bt_device.a2dp_ldac_stream[device_id]->a2dp_stream, in_Stream))
        *out_Stream = app_bt_device.a2dp_ldac_stream[device_id]->a2dp_stream;
#endif
#if defined(A2DP_AAC_ON)
    else if (app_bt_a2dp_is_same_stream(app_bt_device.a2dp_aac_stream[device_id]->a2dp_stream, in_Stream))
        *out_Stream = app_bt_device.a2dp_aac_stream[device_id]->a2dp_stream;
#endif
#if defined(A2DP_SCALABLE_ON)
    else if (app_bt_a2dp_is_same_stream(app_bt_device.a2dp_scalable_stream[device_id]->a2dp_stream, in_Stream))
        *out_Stream = app_bt_device.a2dp_scalable_stream[device_id]->a2dp_stream;
#endif
}
int8_t app_bt_a2dp_is_stream_on_device_id(a2dp_stream_t *in_Stream, uint32_t device_id)
{
    if (app_bt_device.a2dp_stream[device_id]->a2dp_stream == in_Stream)
        return 1;
#if defined(A2DP_LHDC_ON)
    else if (app_bt_device.a2dp_lhdc_stream[device_id]->a2dp_stream == in_Stream)
        return 1;
#endif
#if defined(A2DP_LDAC_ON)
    else if (app_bt_device.a2dp_ldac_stream[device_id]->a2dp_stream == in_Stream)
        return 1;
#endif
#if defined(A2DP_AAC_ON)
    else if (app_bt_device.a2dp_aac_stream[device_id]->a2dp_stream == in_Stream)
        return 1;
#endif
#if defined(A2DP_SCALABLE_ON)
    else if (app_bt_device.a2dp_scalable_stream[device_id]->a2dp_stream == in_Stream)
        return 1;
#endif
    return 0;
}
int8_t app_bt_hfp_is_chan_on_device_id(hf_chan_handle_t chan, uint32_t device_id)
{
    if (app_bt_device.hf_channel[device_id] == chan)
        return 1;
    return 0;
}
int8_t app_bt_is_any_profile_connected(uint32_t device_id)
{
    // TODO avrcp?spp?hid?bisto?ama?dma?rfcomm?
    if ((bt_profile_manager[device_id].hfp_connect == bt_profile_connect_status_success) 
        || (bt_profile_manager[device_id].hsp_connect == bt_profile_connect_status_success)
             || (bt_profile_manager[device_id].a2dp_connect == bt_profile_connect_status_success)) {
        return 1;
    }

    return 0;
}
int8_t app_bt_is_a2dp_connected(uint32_t device_id)
{
    if (bt_profile_manager[device_id].a2dp_connect == bt_profile_connect_status_success)  {
        return 1;
    }

    return 0;
}
btif_remote_device_t *app_bt_get_connected_profile_remdev(uint32_t device_id)
{
    if (bt_profile_manager[device_id].a2dp_connect == bt_profile_connect_status_success) {
        return (btif_remote_device_t *)btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[device_id]);
    }
    else if (bt_profile_manager[device_id].hfp_connect == bt_profile_connect_status_success){
         return (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(app_bt_device.hf_channel[device_id]);
    }
#if defined (__HSP_ENABLE__)
    else if (bt_profile_manager[device_id].hsp_connect == bt_profile_connect_status_success){
        // TODO hsp support
        //return (btif_remote_device_t *)btif_hs_cmgr_get_remote_device(app_bt_device.hs_channel[i]);
    }
#endif

    return NULL;
}
#endif

bool app_bt_get_device_bdaddr(uint8_t deviceId, uint8_t* btAddr)
{
    if (app_bt_is_device_connected(deviceId))
    {
        btif_remote_device_t* currentRemDev = app_bt_get_remoteDev(deviceId);
        memcpy(btAddr,  btif_me_get_remote_device_bdaddr(currentRemDev)->address, BTIF_BD_ADDR_SIZE);
        return true;
    }
    else
    {
        return false;
    }
}

void fast_pair_enter_pairing_mode_handler(void)
{
    app_bt_accessmode_set(BTIF_BAM_GENERAL_ACCESSIBLE);
#ifdef __INTERCONNECTION__
    clear_discoverable_adv_timeout_flag();
    app_interceonnection_start_discoverable_adv(BLE_FAST_ADVERTISING_INTERVAL, APP_INTERCONNECTION_FAST_ADV_TIMEOUT_IN_MS);
#endif
}

bool app_bt_is_hfp_audio_on(void)
{
    return (BTIF_HF_AUDIO_CON == app_bt_device.hf_audio_state[BT_DEVICE_ID_1]);
}


#ifdef VOICE_DATAPATH
bool bt_app_check_device_conneted_to_voicepath(uint8_t device_id)
{
    if(BT_DEVICE_NUM <= device_id)
        return false;
    btif_remote_device_t* remDev = app_bt_get_remoteDev(device_id);
    if(NULL != remDev) {
        if(app_voicepath_is_btaddr_connected(btif_me_get_remote_device_bdaddr(remDev)->address))
            return true;
    }
    return false;
}
#endif

btif_remote_device_t* app_bt_get_connected_mobile_device_ptr(void)
{
    return connectedMobile;
}

#ifdef __AI_VOICE__
bool a2dp_service_is_connected(void)
{
    return (bt_profile_manager[BT_DEVICE_ID_1].a2dp_connect == bt_profile_connect_status_success);
}
#endif

#ifdef BT_USB_AUDIO_DUAL_MODE
#include "a2dp_api.h"
extern "C" a2dp_stream_t* app_bt_get_steam(enum BT_DEVICE_ID_T id)
{
    a2dp_stream_t* stream;

    stream = (a2dp_stream_t*)bt_profile_manager[id].stream;
    return stream;
}

extern "C" int app_bt_get_bt_addr(enum BT_DEVICE_ID_T id,bt_bdaddr_t *bdaddr)
{
    memcpy(bdaddr,&bt_profile_manager[id].rmt_addr,sizeof(bt_bdaddr_t));
    return 0;
}

extern "C" bool app_bt_a2dp_service_is_connected(void)
{
    return (bt_profile_manager[BT_DEVICE_ID_1].a2dp_connect == bt_profile_connect_status_success);
}
#endif

int8_t app_bt_get_rssi(void)
{
    int8_t rssi=127;
    uint8_t i;
    btif_remote_device_t *remDev = NULL;

    for (i=0; i<BT_DEVICE_NUM; i++){
        remDev = btif_me_enumerate_remote_devices(i);
        if(btif_me_get_remote_device_hci_handle(remDev))
        {
             rssi = bt_drv_read_rssi_in_dbm(btif_me_get_remote_device_hci_handle(remDev));
             rssi = bt_drv_rssi_correction(rssi);
             TRACE(" headset to mobile RSSI:%d dBm",rssi);
        }
    }
    return rssi;
}

#if defined(IBRT)
a2dp_stream_t * app_bt_get_mobile_a2dp_stream(uint32_t deviceId)
{
    return bt_profile_manager[deviceId].stream;
}
void app_bt_update_bt_profile_manager(void)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    memcpy(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr.address, p_ibrt_ctrl->mobile_addr.address, BTIF_BD_ADDR_SIZE);
#if defined(A2DP_LHDC_ON)
    if (p_ibrt_ctrl->a2dp_codec.codec_type == BTIF_AVDTP_CODEC_TYPE_LHDC)
    {
        bt_profile_manager[BT_DEVICE_ID_1].stream = &app_bt_device.a2dp_lhdc_stream;
    }
    else
#endif
#if defined(A2DP_AAC_ON)
    if(p_ibrt_ctrl->a2dp_codec.codec_type == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC)
    {
        bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream;

    }
    else
#endif
    {
        bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream;
    }
    bt_profile_manager[BT_DEVICE_ID_1].a2dp_connect = bt_profile_connect_status_success;
    bt_profile_manager[BT_DEVICE_ID_1].hfp_connect = bt_profile_connect_status_success;
    bt_profile_manager[BT_DEVICE_ID_1].has_connected = true;
}

void app_bt_ibrt_reconnect(bt_bdaddr_t mobile_addr)
{
    nvrec_btdevicerecord *mobile_record = NULL;

    bt_profile_manager[BT_DEVICE_ID_1].reconnect_mode = bt_profile_reconnect_null;
    bt_profile_manager[BT_DEVICE_ID_1].rmt_addr = mobile_addr;
    bt_profile_manager[BT_DEVICE_ID_1].chan = app_bt_device.hf_channel[BT_DEVICE_ID_1];

    if (!nv_record_btdevicerecord_find(&mobile_addr, &mobile_record)){
#if defined(A2DP_AAC_ON)
        if(mobile_record->device_plf.a2dp_codectype == BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC){
            bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream;
        }else
#endif
        {
            bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream;
        }
    }else{
        bt_profile_manager[BT_DEVICE_ID_1].stream = app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream;//default using SBC
    }

    TRACE("ibrt_ui_log:start reconnect mobile, addr below:");
    DUMP8("0x%02x ",&(mobile_addr.address[0]),BTIF_BD_ADDR_SIZE);
    app_bt_HF_CreateServiceLink(bt_profile_manager[BT_DEVICE_ID_1].chan, &(bt_profile_manager[BT_DEVICE_ID_1].rmt_addr));
    //osTimerStart(bt_profile_manager[BT_DEVICE_ID_1].connect_timer, APP_IBRT_RECONNECT_TIMEOUT_MS);
}
#endif
