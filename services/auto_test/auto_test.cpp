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
#if defined(_AUTO_TEST_)
#include "cmsis_os.h"
#include "pmu.h"
#include "hal_trace.h"
#include "hal_aud.h"
#include "hal_key.h"
#include "apps.h"
#include "app_thread.h"
#include "app_status_ind.h"
#include "nvrecord.h"
#include "hal_timer.h"
#include "hal_chipid.h"
extern "C" {
#include "besbt_cfg.h"
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "a2dp.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "avrcpi.h"
#include "sys/mei.h"
}
#include "app_bt.h"
#include "btapp.h"
#include "hal_norflash.h"
#include "norflash_api.h"
#include "app_utils.h"
#include "at_thread.h"
#include "at_thread_user.h"
// -------------------------------------------------
// auto test command list.
// AUTO_TEST                   AT_TEST                       Auto test ok.
// BT_INIT                    AT_INIT                       BT Init ok.
// POWER_ON                   POWER_ON                      Power on.
// POWER_OFF                  SYS_POWER_OFF                 System shutdown.
// REBOOT                     SYS_RESET                     System reset.
//                                                          Power on.
// PMU_SHUTDOWN               PMU_SHUTDOWN                  Pmu shutdown.
//                                                          Power on.
// CONNECT                    CONNECT                       Connect ok.
//                                                          Page timeout
// PAIRING                    PAIRING                       Paring ok.
// CONNECT_TWS                CONNECT_TWS                   Connect tws ok.
// CALL_SETUP                 CALL_SETUP:112                Call setup ok.
// CALL_HANGUP                CALL_HANGUP                   Call hangup ok.
// MUSIC_ON                   MUSIC_ON                      Music on ok.
// MUSIC_SUSPEND              MUSIC_SUSPEND                 Music suspend ok.
// SLEEP                      SLEEP_CHECK                   Sleep ok.
// SLEEP_CLEAN                SLEEP_CLEAN                   Sleep clean ok.
// WAKEUP                     WAKEUP                        Wakeup ok.
// STANDBY                    STANDBY                       <ANY_STRING>
// SWITCH_FREQ                SWITCH_FREQ<n><m>             Switch freq ok.
// SIMULATE_KEY               SIMULATE_KEY=<code>,<event>   Simulate key ok.
// NORFALSH                   NORFLASH                      Norflash ok.
// NORFALSH_SUSPEND           NORFLASH_SUSPEND              Norflash suspend ok.
// MEMORY                     MEMORY                        Memory ok.
//---------------------------------------------------------
typedef enum
{
    AT_CMD_ID_TEST        ,
    AT_CMD_ID_POWER_OFF   ,
    AT_CMD_ID_REBOOT      ,
    AT_CMD_ID_PMU_SHUTDOWN,
    AT_CMD_ID_CONNECT     ,
    AT_CMD_ID_DISCONNECT  ,
    AT_CMD_ID_PAIRING     ,
    AT_CMD_ID_CONNECT_TWS ,
    AT_CMD_ID_CALL_SETUP  ,
    AT_CMD_ID_CALL_HANGUP ,
    AT_CMD_ID_MUSIC_ON    ,
    AT_CMD_ID_MUSIC_SUSPEND,
    AT_CMD_ID_SLEEP       ,
    AT_CMD_ID_SLEEP_CLEAN ,
    AT_CMD_ID_WAKEUP      ,
    AT_CMD_ID_SWITCH_FREQ ,
    AT_CMD_ID_POWER_ON    ,
    AT_CMD_ID_BT_INIT     ,
    AT_CMD_ID_SIMULATE_KEY ,
    AT_CMD_ID_NORFLASH,
    AT_CMD_ID_NORFLASH_SUSPEND,
    AT_CMD_ID_MEMORY,
    AT_CMD_ID_COUNT,

}AT_CMD_ID_ENUM_T;


#define AT_CMD_TEST           "AUTO_TEST"
#define AT_CMD_POWER_OFF      "POWER_OFF"
#define AT_CMD_REBOOT         "SYS_RESET"
#define AT_CMD_PMU_SHUTDOWN   "PMU_SHUTDOWN"
#define AT_CMD_CONNECT        "CONNECT"
#define AT_CMD_DISCONNECT     "DISCONNECT"
#define AT_CMD_PAIRING        "PAIRING"
#define AT_CMD_CONNECT_TWS    "CONNECT_TWS"
#define AT_CMD_CALL_SETUP     "CALL_SETUP"
#define AT_CMD_CALL_HANGUP    "CALL_HANGUP"
#define AT_CMD_MUSIC_ON       "MUSIC_ON"
#define AT_CMD_MUSIC_SUSPEND  "MUSIC_SUSPEND"
#define AT_CMD_SLEEP          "SLEEP_CHECK"
#define AT_CMD_SLEEP_CLEAN    "SLEEP_CLEAN"
#define AT_CMD_WAKEUP         "WAKEUP"
#define AT_CMD_SWITCH_FREQ    "SWITCH_FREQ"
#define AT_CMD_POWER_ON       "POWER_ON"
#define AT_CMD_BT_INIT        "BT_INIT"
#define AT_CMD_SIMULATE_KEY   "SIMULATE_KEY"
#define AT_CMD_NORFLASH       "NORFLASH"
#define AT_CMD_NORFLASH_SUSPEND       "NORFLASH_SUSPEND"
#define AT_CMD_MEMORY         "MEMORY"

#define AT_CMD_RESP_TEST           "Auto test ok."
#define AT_CMD_RESP_POWER_OFF      "System shutdown"
#define AT_CMD_RESP_REBOOT         "System reset."
#define AT_CMD_RESP_PMU_SHUTDOWN   "Pmu shutdown."
#define AT_CMD_RESP_CONNECT        "Connect ok."
#define AT_CMD_RESP_DISCONNECT     "Disconnect ok."
#define AT_CMD_RESP_PAIRING        "Pairing ok."
#define AT_CMD_RESP_CONNECT_TWS    "Connect tws ok."
#define AT_CMD_RESP_CALL_SETUP     "Call setup ok."
#define AT_CMD_RESP_CALL_HANGUP    "Call hangup ok."
#define AT_CMD_RESP_MUSIC_ON       "Music on ok."
#define AT_CMD_RESP_MUSIC_SUSPEND  "Music suspend ok."
#define AT_CMD_RESP_SLEEP          "Sleep ok."
#define AT_CMD_RESP_SLEEP_CLEAN    "Sleep clean ok."
#define AT_CMD_RESP_WAKEUP         "Wakeup ok."
#define AT_CMD_RESP_SWITCH_FREQ    "Switch freq ok."
#define AT_CMD_RESP_POWER_ON       "Power on."
#define AT_CMD_RESP_BT_INIT        "BT Init ok."
#define AT_CMD_RESP_SIMULATE_KEY   "Simulate key ok."
#define AT_CMD_RESP_STANDBY        "<ANY_STRING>"
#define AT_CMD_RESP_NORFLASH       "Norflash ok."
#define AT_CMD_RESP_NORFLASH_SUSPEND       "Norflash suspend ok."
#define AT_CMD_RESP_MEMORY         "Memory ok."
#define AT_CMD_RESP_TIME_OUT       "Time out."
#define AT_CMD_RESP_ERROR          "Error."

#define CMD_OUT_TIME_MS    (30*1000)   // millisecond.

typedef void (*APP_BT_AUTO_TEST_T)(uint32_t, uint32_t);
typedef enum
{
    AT_STATUS_FREE,
    AT_STATUS_BUSY,
}AT_STATUS_T;

typedef struct
{
    AT_CMD_ID_ENUM_T cmd_id;
    const char* cmd;
    APP_BT_AUTO_TEST_T pFunc;
} AT_CMD_FRAME_T;

typedef struct
{
    AT_CMD_ID_ENUM_T cmd_id;
    const char* resp;
    uint32_t         count;
}AT_CMD_RESP;

typedef struct
{
    AT_CMD_ID_ENUM_T cur_cmd_id;
    AT_STATUS_T status;
    uint32_t start_time;
    uint32_t param;
    uint32_t pfunc;
}AT_STAUTS;

#if 1
#define AT_TRACE    TRACE
#else
#define AT_TRACE(str,...)
#endif
void at_test(uint32_t param0,uint32_t param1);
void at_power_off(uint32_t param0,uint32_t param1);
void at_reboot(uint32_t param0,uint32_t param1);
void at_pmu_shutdown(uint32_t param0,uint32_t param1);
void at_connet(uint32_t param0,uint32_t param1);
void at_disconnet(uint32_t param0,uint32_t param1);
void at_pairing(uint32_t param0,uint32_t param1);
void at_connect_tws(uint32_t param0,uint32_t param1);
void at_call_setup(uint32_t param0,uint32_t param1);
void at_call_hangup(uint32_t param0,uint32_t param1);
void at_music_on(uint32_t param0,uint32_t param1);
void at_music_suspend(uint32_t param0,uint32_t param1);
void at_sleep(uint32_t param0,uint32_t param1);
void at_sleep_clean(uint32_t param0,uint32_t param1);
void at_wakeup(uint32_t param0,uint32_t param1);
void at_switch_freq(uint32_t param0,uint32_t param1);
void at_power_on(uint32_t param0,uint32_t param1);
void at_bt_init(uint32_t param0,uint32_t param1);
void at_simulate_key(uint32_t param0,uint32_t param1);
void at_norflash(uint32_t param0,uint32_t param1);
void at_norflash_suspend(uint32_t param0,uint32_t param1);
void at_memory(uint32_t param0,uint32_t param1);
extern int system_shutdown(void);
extern int system_reset(void);
extern void bt_key_send(uint32_t code, uint16_t event);
#if defined(BT_USB_AUDIO_DUAL_MODE_TEST)
extern "C" void test_btusb_switch(void);
extern "C" void test_btusb_switch_to_bt(void);
extern "C" void test_btusb_switch_to_usb(void);

#endif

static const AT_CMD_FRAME_T g_at_cmd_frame[] = {
    {AT_CMD_ID_TEST          ,AT_CMD_TEST           ,at_test              },
    {AT_CMD_ID_POWER_OFF     ,AT_CMD_POWER_OFF      ,at_power_off         },
    {AT_CMD_ID_REBOOT        ,AT_CMD_REBOOT         ,at_reboot            },
    {AT_CMD_ID_PMU_SHUTDOWN  ,AT_CMD_PMU_SHUTDOWN   ,at_pmu_shutdown      },
    {AT_CMD_ID_CONNECT       ,AT_CMD_CONNECT        ,at_connet            },
    {AT_CMD_ID_DISCONNECT    ,AT_CMD_DISCONNECT     ,at_disconnet         },
    {AT_CMD_ID_PAIRING       ,AT_CMD_PAIRING        ,at_pairing           },
    {AT_CMD_ID_CONNECT_TWS   ,AT_CMD_CONNECT_TWS    ,at_connect_tws       },
    {AT_CMD_ID_CALL_SETUP    ,AT_CMD_CALL_SETUP     ,at_call_setup        },
    {AT_CMD_ID_CALL_HANGUP   ,AT_CMD_CALL_HANGUP    ,at_call_hangup       },
    {AT_CMD_ID_MUSIC_ON      ,AT_CMD_MUSIC_ON       ,at_music_on          },
    {AT_CMD_ID_MUSIC_SUSPEND ,AT_CMD_MUSIC_SUSPEND  ,at_music_suspend     },
    {AT_CMD_ID_SLEEP         ,AT_CMD_SLEEP          ,at_sleep             },
    {AT_CMD_ID_SLEEP_CLEAN   ,AT_CMD_SLEEP_CLEAN    ,at_sleep_clean       },
    {AT_CMD_ID_WAKEUP        ,AT_CMD_WAKEUP         ,at_wakeup            },
    {AT_CMD_ID_SWITCH_FREQ   ,AT_CMD_SWITCH_FREQ    ,at_switch_freq       },
    {AT_CMD_ID_POWER_ON      ,AT_CMD_POWER_ON       ,at_power_on          },
    {AT_CMD_ID_BT_INIT       ,AT_CMD_BT_INIT        ,at_bt_init           },
    {AT_CMD_ID_SIMULATE_KEY  ,AT_CMD_SIMULATE_KEY   ,at_simulate_key      },
    {AT_CMD_ID_NORFLASH      ,AT_CMD_NORFLASH       ,at_norflash          },
    {AT_CMD_ID_NORFLASH_SUSPEND      ,AT_CMD_NORFLASH_SUSPEND       ,at_norflash_suspend  },
    {AT_CMD_ID_MEMORY        ,AT_CMD_MEMORY         ,at_memory            },

    {AT_CMD_ID_COUNT         ,(const char*)NULL     ,NULL},

};

static AT_CMD_RESP g_at_cmd_resp[] = {
    {AT_CMD_ID_TEST          ,AT_CMD_RESP_TEST                          ,0 },
    {AT_CMD_ID_POWER_OFF     ,AT_CMD_RESP_POWER_OFF                     ,0 },
    {AT_CMD_ID_REBOOT        ,AT_CMD_RESP_REBOOT                        ,0 },
    {AT_CMD_ID_PMU_SHUTDOWN  ,AT_CMD_RESP_PMU_SHUTDOWN                  ,0 },
    {AT_CMD_ID_CONNECT       ,AT_CMD_RESP_CONNECT                       ,0 },
    {AT_CMD_ID_DISCONNECT    ,AT_CMD_RESP_DISCONNECT                    ,0 },
    {AT_CMD_ID_PAIRING       ,AT_CMD_RESP_PAIRING                       ,0 },
    {AT_CMD_ID_CONNECT_TWS   ,AT_CMD_RESP_CONNECT_TWS                   ,0 },
    {AT_CMD_ID_CALL_SETUP    ,AT_CMD_RESP_CALL_SETUP                    ,0 },
    {AT_CMD_ID_CALL_HANGUP   ,AT_CMD_RESP_CALL_HANGUP                   ,0 },
    {AT_CMD_ID_MUSIC_ON      ,AT_CMD_RESP_MUSIC_ON                      ,0 },
    {AT_CMD_ID_MUSIC_SUSPEND ,AT_CMD_RESP_MUSIC_SUSPEND                 ,0 },
    {AT_CMD_ID_SLEEP         ,AT_CMD_RESP_SLEEP                         ,0 },
    {AT_CMD_ID_SLEEP_CLEAN   ,AT_CMD_RESP_SLEEP_CLEAN                   ,0 },
    {AT_CMD_ID_WAKEUP        ,AT_CMD_RESP_WAKEUP                        ,0 },
    {AT_CMD_ID_SWITCH_FREQ   ,AT_CMD_RESP_SWITCH_FREQ                   ,0 },
    {AT_CMD_ID_POWER_ON      ,AT_CMD_RESP_POWER_ON                      ,0 },
    {AT_CMD_ID_BT_INIT       ,AT_CMD_RESP_BT_INIT                       ,0 },
    {AT_CMD_ID_SIMULATE_KEY  ,AT_CMD_RESP_SIMULATE_KEY                  ,0 },
    {AT_CMD_ID_NORFLASH      ,AT_CMD_RESP_NORFLASH                      ,0 },
    {AT_CMD_ID_NORFLASH_SUSPEND ,AT_CMD_RESP_NORFLASH_SUSPEND           ,0 },
    {AT_CMD_ID_MEMORY        ,AT_CMD_RESP_MEMORY                        ,0 },
    {AT_CMD_ID_COUNT         ,(const char*)NULL                         ,0 },

};

AT_STAUTS g_at_status = {AT_CMD_ID_COUNT,AT_STATUS_FREE,0,0,0};

void _at_trim(uint8_t* buff)
{
    uint8_t* p;
    uint32_t len;
    // int32_t i;

    len = strlen((char*)buff);
    p = buff + (len - 1);

    while(1)
    {
        if(*p == 0x20 || *p == 0x09 || *p == 0x0A || *p == 0x0D)
        {
            *p = 0;
            p--;
        }
        else
        {
            break;
        }
        if(p == buff)
        {
            break;
        }
    }

    p = buff;
    while(1)
    {
       if(*p == 0x20 || *p == 0x09)
       {
           p++;
       }
       else
       {
           break;
       }
    }

    while(*p)
    {
        *buff = *p;
        buff ++;
        p++;
    }
}

/*
static int _at_strtohex(uint8_t* hex, char* str, int len)
{
    int i = 0;
    char* begin;
    char* end;
    //char* tmp;
    char* hex_unit;

    begin = str;
    end = str;
    for(i = 0; i < len; i++)
    {
        hex_unit = begin;
        end = strstr(begin,",");
        if(end)
        {
            *end = 0;
            begin = end + 1;
        }
        else
        {
        }
        hex[i] = (uint8_t)strtol(hex_unit,NULL,16);
    }
    if(i == len)
        return 0;
    else
        return -1;
}
*/
static char* _at_get_cmd_resp(AT_CMD_ID_ENUM_T cmd_id)
{
    uint32_t i;
    uint32_t cmd_count = 0;
    char* resp = NULL;

    cmd_count = sizeof(g_at_cmd_resp)/sizeof(AT_CMD_RESP);

    for(i = 0; i < cmd_count; i++)
    {
        if(AT_CMD_ID_COUNT == g_at_cmd_resp[i].cmd_id)
        {
            AT_TRACE("_debug: undefined at cmd resp, cmd_id = %d.",cmd_id);
            resp = NULL;
            goto _func_end;
        }
        if(cmd_id == g_at_cmd_resp[i].cmd_id)
        {
            if(g_at_cmd_resp[i].count > 0)
            {
                resp = (char*)g_at_cmd_resp[i].resp;
                g_at_cmd_resp[i].count = 0;
                AT_TRACE("_debug:  ount > 0,cmd_id = %d,resp = %s.",cmd_id,resp);
            }
            else
            {
                resp = NULL;
            }
            break;
        }
    }
_func_end:
    return resp;
}

/*
static AT_STATUS_T _at_get_status(void)
{
    if(g_at_status.cur_cmd_id == AT_CMD_ID_COUNT)
        return AT_STATUS_FREE;
    return g_at_status.status;
}
*/

static AT_CMD_ID_ENUM_T _at_get_cur_cmd_id(void)
{
    return g_at_status.cur_cmd_id;
}

static void _at_set_status(AT_CMD_ID_ENUM_T cmd_id,AT_STATUS_T status,uint32_t param,uint32_t pfuc)
{
    g_at_status.status = status;
    if(status == AT_STATUS_BUSY)
    {
        g_at_status.cur_cmd_id = cmd_id;
        g_at_status.start_time = hal_sys_timer_get();
    }
    g_at_status.param = param;
    g_at_status.pfunc = pfuc;
}

static void _at_resp(void)
{
    AT_CMD_ID_ENUM_T cur_cmd_id;
    uint32_t wait_time;
    char* resp;

    cur_cmd_id = _at_get_cur_cmd_id();
    if(cur_cmd_id == AT_CMD_ID_COUNT)
    {
        TRACE_IMM("AUTO_TEST:%s",AT_CMD_RESP_ERROR);
        return;
    }
    resp = _at_get_cmd_resp(cur_cmd_id);
    if(resp)
    {
        //osDelay(64);
        TRACE_IMM("AUTO_TEST: %s",resp);
        //osDelay(64);
        _at_set_status(AT_CMD_ID_COUNT,AT_STATUS_FREE,0,0);
    }
    else
    {
        wait_time = TICKS_TO_MS(hal_sys_timer_get() - g_at_status.start_time);
        if(wait_time < CMD_OUT_TIME_MS)
       {
            //osDelay(4);
            //app_bt_send_request(APP_BT_REQ_AUTO_TEST, (uint32_t)g_at_status.param,cur_cmd_id,
            //g_at_status.pfunc);
            AT_TRACE("_debug: _at_resp,auto_test wait : %d(ms)",wait_time);
       }
       else
       {
            _at_set_status(AT_CMD_ID_COUNT,AT_STATUS_FREE,0,0);
            AT_TRACE("AUTO_TEST:%s,wait time = %d(ms)",AT_CMD_RESP_TIME_OUT,wait_time);
       }
    }
}

#define AT_PARAM_LEN   256
uint8_t at_param_buff[AT_PARAM_LEN];
extern "C" int auto_test_prase(uint8_t *cmd)
{
    int ret = 0;
    uint8_t i;
    uint8_t* cmd_name = NULL;
    uint8_t* cmd_param = NULL;
    uint8_t* p;
    AT_CMD_ID_ENUM_T cmd_id;
    uint32_t param_len;

    if(NULL == cmd)
    {
        ret =  -1;
        goto _func_end;
    }

    AT_TRACE("_debug: cmd = %s",cmd);
    cmd_name = cmd;
    p = (uint8_t*)strstr((char*)cmd,(char*)"=");
    if(p)
    {
        *p = 0;
        cmd_param = (p + 1);
        _at_trim(cmd_param);
    }
    else
    {
        cmd_param = NULL;
    }
    _at_trim(cmd_name);


    for(i = 0; i < sizeof(g_at_cmd_frame)/sizeof(AT_CMD_FRAME_T); i++)
    {
        if(AT_CMD_ID_COUNT == g_at_cmd_frame[i].cmd_id)
        {
            AT_TRACE("_debug: undefined at cmd: %s.",cmd);
            ret = -2;
            goto _func_end;
        }
        if(!strcmp((const char *)cmd, (const char *)g_at_cmd_frame[i].cmd))
        {
            if(NULL == g_at_cmd_frame[i].pFunc)
            {
                ret = -3;
                goto _func_end;
            }
            /*
            if(AT_CMD_ID_REBOOT != g_at_cmd_frame[i].cmd_id)
            {
                if(_at_get_status() != AT_STATUS_FREE)
                {
                    ret = -2;
                    AT_TRACE("_debug: auto_test is busy. cmd = %s,cmd_id = %d,cur_cmd_id = %d",
                         g_at_cmd_frame[i].cmd_id,cmd,g_at_status.cur_cmd_id);
                    goto _func_end;
                }
            }
            */
            cmd_id = g_at_cmd_frame[i].cmd_id;
            // AT_TRACE("_debug: %s,line= %d.",__func__,__LINE__);
            //memset(at_param_buff,0,sizeof(at_param_buff));
            param_len = strlen((char*)cmd_param);
            param_len = param_len < sizeof(at_param_buff) ? param_len :  sizeof(at_param_buff) - 1;
            if(param_len > 0)
            {
                strncpy((char*)at_param_buff,(char*)cmd_param,param_len);
            }
            _at_set_status(cmd_id,AT_STATUS_BUSY,(uint32_t)cmd_param,(uint32_t)g_at_cmd_frame[i].pFunc);
            at_enqueue_cmd((uint32_t)cmd_id,(uint32_t)at_param_buff,(uint32_t)g_at_cmd_frame[i].pFunc);
            //g_at_cmd_frame[i].pFunc((uint32_t)cmd_param,cmd_id);

            break;
        }
    }
_func_end:
    if(ret != 0)
    {
       AT_TRACE("_debug: parse failed,ret = %d.",ret);
    }
    return ret;
}

extern "C" int auto_test_send(char *resp)
{
    uint32_t i;
    uint32_t cmd_count = 0;
    int32_t ret = 0;

    cmd_count = sizeof(g_at_cmd_resp)/sizeof(AT_CMD_RESP);
    // AT_TRACE("_debug: auto_test_send : %s",resp);
    for(i = 0; i < cmd_count; i++)
    {
        if(AT_CMD_ID_COUNT == g_at_cmd_resp[i].cmd_id)
        {
            AT_TRACE("_debug: auto_test_send: undefined at cmd resp, resp = %s.",resp);
            ret = -1;
            goto _func_end;
        }
        if(!strncmp((const char *)resp, (const char *)g_at_cmd_resp[i].resp,strlen((char*)g_at_cmd_resp[i].resp)))
        {
            if( g_at_cmd_resp[i].cmd_id == AT_CMD_ID_POWER_ON ||
               // g_at_cmd_resp[i].cmd_id == AT_CMD_ID_POWER_OFF ||
               // g_at_cmd_resp[i].cmd_id == AT_CMD_ID_REBOOT ||
               g_at_cmd_resp[i].cmd_id == g_at_status.cur_cmd_id)
            {
                TRACE_IMM("AUTO_TEST:%s",resp);
                _at_set_status(AT_CMD_ID_COUNT,AT_STATUS_FREE,0,0);
            }
            else
            {
                g_at_cmd_resp[i].count  = 1;
                AT_TRACE("_debug: auto_test_send: set count to 1, i = %d, resp = %s.",i,resp);
            }
            break;
        }
    }
_func_end:
    if(ret != 0)
    {
        AT_TRACE("_debug: auto_test_send: %s undefine !!!!!",resp);
    }
    return ret;
}

void at_test(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am app_auto_test");

    AUTO_TEST_SEND(AT_CMD_RESP_TEST);
}

void at_power_off(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_power_off");
    pmu_at_skip_shutdown(false);
    system_shutdown();
}

void at_reboot(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_reboot");
    //system_reset();
    app_reset();
}

void at_pmu_shutdown(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_reboot");
    //system_reset();

    AUTO_TEST_SEND(AT_CMD_RESP_PMU_SHUTDOWN);

    // pmu_rtc_enable();
    // pmu_rtc_set(5);
    // pmu_rtc_set_alarm(10);
    pmu_at_skip_shutdown(true);
    pmu_shutdown();
}

void at_connet(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;
    //AT_TRACE("_debug: I am at_connet");
    //app_tws_charger_box_opened();
    _at_resp();
}

void at_disconnet(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_disconnet");
    _at_resp();
}

void at_pairing(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_pairing");
    _at_resp();
}

void at_connect_tws(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_connect_tws");
    _at_resp();
}

void at_call_setup(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_call_setup");
    _at_resp();
}

void at_call_hangup(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_call_hangup");
    _at_resp();
}

void at_music_on(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_music_on");
    _at_resp();
}

void at_music_suspend(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_music_suspend");
    _at_resp();
}

void at_sleep(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_sleep");
    _at_resp();
}

void at_sleep_clean(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;
    unsigned int i;

    //AT_TRACE("_debug: I am at_sleep_clean");

	for(i = 0; i < sizeof(g_at_cmd_resp)/sizeof(AT_CMD_RESP); i++)
	{
		if(AT_CMD_ID_SLEEP_CLEAN == g_at_cmd_resp[i].cmd_id)
		{
			g_at_cmd_resp[i].count = 0;
			break;
		}
	}
	AUTO_TEST_SEND(AT_CMD_RESP_SLEEP_CLEAN);
}

void at_wakeup(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_wakeup");
    _at_resp();
}

void at_switch_freq(uint32_t param0,uint32_t param1)
{
    char* p = (char*)param0;
    char* user_str;
    char* freq_str;
    uint32_t user = 0xffffffff;
    uint32_t freq = 0xffffffff;
    int i;
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_switch_freq");
    user_str = (char*)param0;
    p = strstr(user_str,",");
    if(p)
    {
        *p = 0;
        p ++;
    }
    else
    {
        AT_TRACE("_debug: at_switch_freq: param error 0.");
        goto _func_end;
    }
    _at_trim((uint8_t*)user_str);
    if(*user_str)
    {
        user = atoi(user_str);
    }
    else
    {
        AT_TRACE("_debug: at_switch_freq: param error 1.");
        goto _func_end;
    }

    freq_str = p;
     _at_trim((uint8_t*)freq_str);
    if(*freq_str)
    {
        freq = atoi(freq_str);
    }
    else
    {
        AT_TRACE("_debug: at_switch_freq: param error 2.");
        goto _func_end;
    }
    if(user >= APP_SYSFREQ_USER_QTY || freq >= APP_SYSFREQ_FREQ_QTY)
    {
        AT_TRACE("_debug: at_switch_freq: param error 3.");
        goto _func_end;
    }

    //AT_TRACE("_debug: at_switch_freq: user = %d, freq = %d.",user,freq);
    app_sysfreq_req((APP_SYSFREQ_USER_T)user,(APP_SYSFREQ_FREQ_T)freq);

    for(i = 0; i < 10000; i ++)
    {

    }
    AUTO_TEST_SEND("Switch freq ok.");
_func_end:
    return;

}

void at_power_on(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_power_on");
    AUTO_TEST_SEND(AT_CMD_RESP_POWER_ON);
}

void at_bt_init(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;

    //AT_TRACE("_debug: I am at_bt_init");
    _at_resp();

}

#include "app_key.h"
    typedef struct
    {
        uint32_t key;
        const char*     key_str;
    }AT_KEY_MAP;
    AT_KEY_MAP key_code_map[] =
    {
        {(uint32_t)HAL_KEY_CODE_NONE,"KEY_CODE_NONE"},
        {(uint32_t)HAL_KEY_CODE_PWR ,"KEY_CODE_PWR" },
        {(uint32_t)HAL_KEY_CODE_FN1 ,"KEY_CODE_FN1" },
        {(uint32_t)HAL_KEY_CODE_FN2 ,"KEY_CODE_FN2" },
        {(uint32_t)HAL_KEY_CODE_FN3 ,"KEY_CODE_FN3" },
        {(uint32_t)HAL_KEY_CODE_FN4 ,"KEY_CODE_FN4" },
        {(uint32_t)HAL_KEY_CODE_FN5 ,"KEY_CODE_FN5" },
        {(uint32_t)HAL_KEY_CODE_FN6 ,"KEY_CODE_FN6" },
        {(uint32_t)HAL_KEY_CODE_FN7 ,"KEY_CODE_FN7" },
        {(uint32_t)HAL_KEY_CODE_FN8 ,"KEY_CODE_FN8" },
        {(uint32_t)HAL_KEY_CODE_FN9 ,"KEY_CODE_FN9" },
        {(uint32_t)HAL_KEY_CODE_FN10,"KEY_CODE_FN10"},
        {(uint32_t)HAL_KEY_CODE_FN11,"KEY_CODE_FN11"},
        {(uint32_t)HAL_KEY_CODE_FN12,"KEY_CODE_FN12"},
        {(uint32_t)HAL_KEY_CODE_FN13,"KEY_CODE_FN13"},
        {(uint32_t)HAL_KEY_CODE_FN14,"KEY_CODE_FN14"},
        {(uint32_t)HAL_KEY_CODE_FN15,"KEY_CODE_FN15"}
    };
    AT_KEY_MAP key_event_map[] =
    {
        {(uint32_t)HAL_KEY_EVENT_NONE             ,"KEY_EVENT_NONE"             },
        {(uint32_t)HAL_KEY_EVENT_DOWN             ,"KEY_EVENT_DOWN"             },
        {(uint32_t)HAL_KEY_EVENT_FIRST_DOWN       ,"KEY_EVENT_FIRST_DOWN"       },
        {(uint32_t)HAL_KEY_EVENT_CONTINUED_DOWN   ,"KEY_EVENT_CONTINUED_DOWN"   },
        {(uint32_t)HAL_KEY_EVENT_UP               ,"KEY_EVENT_UP"               },
        {(uint32_t)HAL_KEY_EVENT_LONGPRESS        ,"KEY_EVENT_LONGPRESS"        },
        {(uint32_t)HAL_KEY_EVENT_LONGLONGPRESS    ,"KEY_EVENT_LONGLONGPRESS"    },
        {(uint32_t)HAL_KEY_EVENT_CLICK            ,"KEY_EVENT_CLICK"            },
        {(uint32_t)HAL_KEY_EVENT_DOUBLECLICK      ,"KEY_EVENT_DOUBLECLICK"      },
        {(uint32_t)HAL_KEY_EVENT_TRIPLECLICK      ,"KEY_EVENT_TRIPLECLICK"      },
        {(uint32_t)HAL_KEY_EVENT_ULTRACLICK       ,"KEY_EVENT_ULTRACLICK"       },
        {(uint32_t)HAL_KEY_EVENT_RAMPAGECLICK     ,"KEY_EVENT_RAMPAGECLICK"     },
        {(uint32_t)HAL_KEY_EVENT_REPEAT           ,"KEY_EVENT_REPEAT"           },
        {(uint32_t)HAL_KEY_EVENT_GROUPKEY_DOWN    ,"KEY_EVENT_GROUPKEY_DOWN"    },
        {(uint32_t)HAL_KEY_EVENT_GROUPKEY_REPEAT  ,"KEY_EVENT_GROUPKEY_REPEAT"  },
        {(uint32_t)HAL_KEY_EVENT_INITDOWN         ,"KEY_EVENT_INITDOWN"         },
        {(uint32_t)HAL_KEY_EVENT_INITUP           ,"KEY_EVENT_INITUP"           },
        {(uint32_t)HAL_KEY_EVENT_INITLONGPRESS    ,"KEY_EVENT_INITLONGPRESS"    },
        {(uint32_t)HAL_KEY_EVENT_INITLONGLONGPRESS,"KEY_EVENT_INITLONGLONGPRESS"},
        {(uint32_t)HAL_KEY_EVENT_INITFINISHED     ,"KEY_EVENT_INITFINISHED"     }
    };
void at_simulate_key(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    char* code_str;
    char* event_str;
    uint32_t key_code = 0;
    uint16_t key_event = 0;
    char* p;
    uint32_t i;

    cmd_id = cmd_id;
    AT_TRACE("_debug: I am at_simulate_key");
    p = (char*)param0;
    code_str = p;
    p = strstr(p,",");
    if(p)
    {
        *p = 0;
        p++;
    }
    else
    {
        goto _func_end;
    }

    event_str = p;
    _at_trim((uint8_t*)code_str);
    _at_trim((uint8_t*)event_str);
    AT_TRACE("_debug: code_str = %s, event_str = %s.",code_str,event_str);
    for(i = 0; i < sizeof(key_code_map)/sizeof(AT_KEY_MAP); i ++)
    {
        if(strncmp(code_str,key_code_map[i].key_str,strlen(code_str)) == 0)
        {
            key_code = key_code_map[i].key;
            break;
        }
    }

    if(i == sizeof(key_code_map)/sizeof(AT_KEY_MAP))
    {
        goto _func_end;
    }

    for(i = 0; i < sizeof(key_event_map)/sizeof(AT_KEY_MAP); i ++)
    {
        if(strncmp(event_str,key_event_map[i].key_str,strlen(event_str)) == 0)
        {
            key_event = (key_event_map[i].key & 0xffff);
            break;
        }
    }

    if(i == sizeof(key_event_map)/sizeof(AT_KEY_MAP))
    {
        goto _func_end;
    }
    AT_TRACE("_debug: key_code = %d, key_event = %d.",key_code,key_event);

    if(key_code == HAL_KEY_CODE_FN15 && key_event == HAL_KEY_EVENT_CLICK)
    {
#if defined(BT_USB_AUDIO_DUAL_MODE_TEST)
        test_btusb_switch_to_bt();
        //osDelay(5000);
#endif
    }
    if(key_code == HAL_KEY_CODE_FN14 && key_event == HAL_KEY_EVENT_CLICK)
    {
#if defined(BT_USB_AUDIO_DUAL_MODE_TEST)
        test_btusb_switch_to_usb();
#endif
    }
    AUTO_TEST_SEND(AT_CMD_RESP_SIMULATE_KEY);
    //simul_key_event_process((uint32_t)key_code,(uint16_t)key_event);


_func_end:
    return;
}

enum AT_FLASH_REGION_ID
{
    FLASH_REGION_ID_CODE,
    FLASH_REGION_ID_FREE,
    FLASH_REGION_ID_CRASH_DUMP,
    FLASH_REGION_ID_AUD,
    FLASH_REGION_ID_USERDATA,
    FLASH_REGION_ID_FACTORY,
    FLASH_REGION_ID_NUMBER,
};

typedef struct _flash_region_info
{
    enum AT_FLASH_REGION_ID id;
    uint32_t start_addr;
    uint32_t end_addr;
}AT_FLASH_REGION_INFO;
#define AT_ALIGN(x,a)  (uint32_t)(((x + a - 1)/a) * a)
extern uint32_t __flash_start;
extern uint32_t __flash_end;

extern uint32_t __crash_dump_start;
extern uint32_t __crash_dump_end;

extern uint32_t __custom_parameter_start;
extern uint32_t __custom_parameter_end;

extern uint32_t __aud_start;
extern uint32_t __aud_end;

extern uint32_t __userdata_start;
extern uint32_t __userdata_end;

extern uint32_t __factory_start;
extern uint32_t __factory_end;
extern uint32_t __free_flash;
#define AT_FLASH_SECTOR_SIZE  0x1000
#define AT_FLASH_BLOCK_SIZE  0x10000
AT_FLASH_REGION_INFO region_info[] = {
        {FLASH_REGION_ID_CODE,
         FLASH_C_TO_NC(AT_ALIGN((uint32_t)(&__flash_start),AT_FLASH_SECTOR_SIZE)),
         FLASH_C_TO_NC(AT_ALIGN((uint32_t)(&__flash_end),AT_FLASH_SECTOR_SIZE))},
        {FLASH_REGION_ID_FREE,
         FLASH_C_TO_NC(AT_ALIGN((uint32_t)(&__flash_end + AT_FLASH_SECTOR_SIZE),AT_FLASH_SECTOR_SIZE)),
         FLASH_C_TO_NC(AT_ALIGN((uint32_t)(&__flash_end + AT_FLASH_SECTOR_SIZE) + 10*AT_FLASH_SECTOR_SIZE,AT_FLASH_SECTOR_SIZE))},//AT_ALIGN((uint32_t)(&__crash_dump_start),AT_FLASH_SECTOR_SIZE)},
        {FLASH_REGION_ID_CRASH_DUMP,
         FLASH_C_TO_NC(AT_ALIGN((uint32_t)(&__flash_end + AT_FLASH_SECTOR_SIZE) + 10*AT_FLASH_SECTOR_SIZE,AT_FLASH_SECTOR_SIZE)),//AT_ALIGN((uint32_t)(&__crash_dump_start),AT_FLASH_SECTOR_SIZE),
         FLASH_C_TO_NC(AT_ALIGN((uint32_t)(&__flash_end + AT_FLASH_SECTOR_SIZE) + 20*AT_FLASH_SECTOR_SIZE,AT_FLASH_SECTOR_SIZE))},//AT_ALIGN((uint32_t)(&__crash_dump_end),AT_FLASH_SECTOR_SIZE)},
        {FLASH_REGION_ID_AUD,
         AT_ALIGN((uint32_t)(&__aud_start),AT_FLASH_SECTOR_SIZE),
         AT_ALIGN((uint32_t)(&__aud_end),AT_FLASH_SECTOR_SIZE)},
        {FLASH_REGION_ID_USERDATA,
         AT_ALIGN((uint32_t)(&__userdata_start),AT_FLASH_SECTOR_SIZE),
         AT_ALIGN((uint32_t)(&__userdata_end),AT_FLASH_SECTOR_SIZE)},
        {FLASH_REGION_ID_FACTORY,
         AT_ALIGN((uint32_t)(&__factory_start) + 20*AT_FLASH_SECTOR_SIZE,AT_FLASH_SECTOR_SIZE),//AT_ALIGN((uint32_t)(&__factory_start),AT_FLASH_SECTOR_SIZE),
         AT_ALIGN((uint32_t)(&__flash_end) + 30*AT_FLASH_SECTOR_SIZE,AT_FLASH_SECTOR_SIZE)},//AT_ALIGN((uint32_t)(&__factory_end),AT_FLASH_SECTOR_SIZE)},
    };

uint8_t at_buff_r[AT_FLASH_SECTOR_SIZE];
uint8_t at_buff_r1[AT_FLASH_SECTOR_SIZE];
uint8_t at_buff_w[AT_FLASH_SECTOR_SIZE];
uint8_t at_buff_b[AT_FLASH_SECTOR_SIZE];

int32_t _at_norflash_test(uint32_t addr,uint32_t size,uint8_t is_restore)
{
    uint32_t start_addr;
    uint32_t i,j;
    uint8_t value = 0;
    int32_t ret = 0;
    enum HAL_NORFLASH_RET_T result = HAL_NORFLASH_OK;

    for(i = 0; i < size/AT_FLASH_SECTOR_SIZE; i++)
    {
        // erase checking.
        start_addr = addr + i * AT_FLASH_SECTOR_SIZE;
        if(is_restore != 0)
        {
            result = hal_norflash_read(HAL_NORFLASH_ID_0,start_addr,at_buff_b,AT_FLASH_SECTOR_SIZE);
            if(result != HAL_NORFLASH_OK)
            {
                ret = -2;
                goto _func_end;
            }
        }
        result = hal_norflash_erase(HAL_NORFLASH_ID_0,start_addr,AT_FLASH_SECTOR_SIZE);
        if(result != HAL_NORFLASH_OK)
        {
            ret = -1;
            goto _func_end;
        }
        result = hal_norflash_read(HAL_NORFLASH_ID_0,start_addr,at_buff_r,AT_FLASH_SECTOR_SIZE);
        if(result != HAL_NORFLASH_OK)
        {
            ret = -2;
            goto _func_end;
        }
        for(j = 0; j < AT_FLASH_SECTOR_SIZE; j++)
        {
            if(at_buff_r[j] != 0xff)
            {
                ret = -3;
                goto _func_end;
            }
        }

        // write/read checking
        for(j = 0; j < AT_FLASH_SECTOR_SIZE; j++)
        {
            at_buff_w[j] = value ++;
        }
        result = hal_norflash_write(HAL_NORFLASH_ID_0,start_addr,at_buff_w,AT_FLASH_SECTOR_SIZE);
        if(result != HAL_NORFLASH_OK)
        {
            ret = -4;
            goto _func_end;
        }

        result = hal_norflash_read(HAL_NORFLASH_ID_0,start_addr,at_buff_r,AT_FLASH_SECTOR_SIZE);
        if(result != HAL_NORFLASH_OK)
        {
            ret = -5;
            goto _func_end;
        }
        for(j = 0; j < AT_FLASH_SECTOR_SIZE; j++)
        {
            if(at_buff_r[j] != at_buff_w[j])
            {
                ret = -6;
                goto _func_end;
            }
        }
        if(is_restore != 0)
        {
            result = hal_norflash_erase(HAL_NORFLASH_ID_0,start_addr,AT_FLASH_SECTOR_SIZE);
            if(result != HAL_NORFLASH_OK)
            {
                ret = -7;
                break;
            }
            result = hal_norflash_write(HAL_NORFLASH_ID_0,start_addr,at_buff_b,AT_FLASH_SECTOR_SIZE);
            if(result != HAL_NORFLASH_OK)
            {
                ret = -8;
                break;
            }
            result = hal_norflash_read(HAL_NORFLASH_ID_0,start_addr,at_buff_r,AT_FLASH_SECTOR_SIZE);
            if(result != HAL_NORFLASH_OK)
            {
                ret = -9;
                goto _func_end;
            }
            for(j = 0; j < AT_FLASH_SECTOR_SIZE; j++)
            {
                if(at_buff_r[j] != at_buff_b[j])
                {
                    ret = -10;
                    goto _func_end;
                }
            }

        }

    }

_func_end:

    AT_TRACE("_debug: flash checking end. ret = %d.",ret);
    return ret;

}


AT_FLASH_REGION_INFO* _at_norflash_get_region_info(enum AT_FLASH_REGION_ID id)
{
    uint32_t i;

    for(i = 0; i < sizeof(region_info)/sizeof(AT_FLASH_REGION_INFO); i++)
    {
        if(region_info[i].id == id)
        {
            return &region_info[i];
        }
    }
    return NULL;
}

void at_norflash(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    // uint32_t start_addr = 0;
    // uint32_t end_addr = 0;
    // uint32_t size = 0;
    uint8_t check_code = 0;
    uint8_t check_free = 1;
    uint8_t check_crash_dump = 1;
    uint8_t check_aud = 1;
    uint8_t check_userdata = 1;
    uint8_t check_factory = 1;
    AT_FLASH_REGION_INFO* region_info;
    int32_t result = 0;
    uint8_t error_code = 0;
    cmd_id = cmd_id;
    uint32_t lock_pri;

    error_code = error_code;
    AT_TRACE("_debug:at_norflash begin.");
    if(check_code)
    {
        AT_TRACE("_debug:at_norflash check code region.");
        region_info = _at_norflash_get_region_info(FLASH_REGION_ID_CODE);
        lock_pri = int_lock();
        result = _at_norflash_test(region_info->start_addr,
                                   (region_info->end_addr - region_info->start_addr),
                                   1);
        int_unlock(lock_pri);
        if(result != 0)
        {
            error_code = 1;
            goto _func_fail;
        }
    }
    if(check_free)
    {
        AT_TRACE("_debug:at_norflash check free region.");
        region_info = _at_norflash_get_region_info(FLASH_REGION_ID_FREE);
        lock_pri = int_lock();
        result = _at_norflash_test(region_info->start_addr,
                                   (region_info->end_addr - region_info->start_addr),
                                   1);
        int_unlock(lock_pri);
        if(result != 0)
        {
            error_code = 2;
            goto _func_fail;
        }
    }
    if(check_crash_dump)
    {
        AT_TRACE("_debug:at_norflash check crash_dump region.");
        region_info = _at_norflash_get_region_info(FLASH_REGION_ID_CRASH_DUMP);
        lock_pri = int_lock();
        result = _at_norflash_test(region_info->start_addr,
                                   (region_info->end_addr - region_info->start_addr),
                                   1);
        int_unlock(lock_pri);
        if(result != 0)
        {
            error_code = 3;
            goto _func_fail;
        }
    }
    if(check_aud)
    {
        AT_TRACE("_debug:at_norflash check aud region.");
        region_info = _at_norflash_get_region_info(FLASH_REGION_ID_AUD);
        lock_pri = int_lock();
        result = _at_norflash_test(region_info->start_addr,
                                   (region_info->end_addr - region_info->start_addr),
                                   1);
        int_unlock(lock_pri);
        if(result != 0)
        {
            error_code = 4;
            goto _func_fail;
        }
    }
    if(check_userdata)
    {
        AT_TRACE("_debug:at_norflash check userdata region.");
        region_info = _at_norflash_get_region_info(FLASH_REGION_ID_USERDATA);
        lock_pri = int_lock();
        result = _at_norflash_test(region_info->start_addr,
                                   (region_info->end_addr - region_info->start_addr),
                                   1);
        int_unlock(lock_pri);
        if(result != 0)
        {
            error_code = 5;
            goto _func_fail;
        }
    }
    if(check_factory)
    {
        AT_TRACE("_debug:at_norflash check factory region.");
        region_info = _at_norflash_get_region_info(FLASH_REGION_ID_FACTORY);
        lock_pri = int_lock();
        result = _at_norflash_test(region_info->start_addr,
                                   (region_info->end_addr - region_info->start_addr),
                                   1);
        int_unlock(lock_pri);
        if(result != 0)
        {
            error_code = 6;
            goto _func_fail;
        }
    }

    //AT_TRACE("_debug: I am at_norflash");
    error_code = 0;
    AUTO_TEST_SEND(AT_CMD_RESP_NORFLASH);
_func_fail:
    AT_TRACE("_debug:Flash testing fail.error_code = %d.",error_code);

}

enum NORFLASH_ASYNC_CMD
{
    NORFLASH_ASYNC_CMD_WRITE,
    NORFLASH_ASYNC_CMD_ERASE,
    NORFLASH_ASYNC_CMD_FLUSH,
};

// regin[0]: start = 0x3c000000, end = 0x3c0cf000. -- code

// regin[1]: start = 0x3c0cf000, end = 0x3c3ed000. -- __crash_dump

// regin[2]: start = 0x3c3ed000, end = 0x3c3fd000. -- __custom_parameter

// regin[3]: start = 0x3c3fe000, end = 0x3c3fe000. -- __aud

// regin[4]: start = 0x3c3fe000, end = 0x3c3ff000. -- __userdata

// regin[5]: start = 0x3c3ff000, end = 0x3c400000. -- __factory

enum NORFLASH_ASYNC_STATE
{
    NORFLASH_ASYNC_STATE_INIT,
    NORFLASH_ASYNC_STATE_ERASE,
    NORFLASH_ASYNC_STATE_WRITTING,
    NORFLASH_ASYNC_STATE_WAIT,
    NORFLASH_ASYNC_STATE_END,
    NORFLASH_ASYNC_STATE_FAILED,
};

typedef struct
{
    enum NORFLASH_ASYNC_STATE state;
    AT_FLASH_REGION_INFO *region;
    enum NORFLASH_API_MODULE_ID_T mod_id;
    uint32_t buff_len;
    uint32_t e_addr;
    uint32_t w_offs;
    uint32_t len_index;
    uint32_t succ_count;
    uint32_t fail_count;
    uint32_t tried_count;
}ASYNC_SCENE;

uint32_t g_at_len[] = {1,255,256,512,1024,2048};
ASYNC_SCENE at_async_scene[3] =
{
    {NORFLASH_ASYNC_STATE_INIT,&region_info[FLASH_REGION_ID_FREE],NORFLASH_API_MODULE_ID_FREE,AT_FLASH_SECTOR_SIZE*2,0,0,0,0,0,0},
    {NORFLASH_ASYNC_STATE_INIT,&region_info[FLASH_REGION_ID_CRASH_DUMP],NORFLASH_API_MODULE_ID_CRASH_DUMP,AT_FLASH_SECTOR_SIZE*2,0,0,0,0,0,0},
    {NORFLASH_ASYNC_STATE_INIT,&region_info[FLASH_REGION_ID_FACTORY],NORFLASH_API_MODULE_ID_FACTORY,AT_FLASH_SECTOR_SIZE*1,0,0,0,0,0,0}
};
bool g_at_test_break;
//uint8_t w_buff[AT_FLASH_SECTOR_SIZE];
//uint8_t r_buff[AT_FLASH_SECTOR_SIZE];
void at_w_buff_set(uint8_t* buff,uint32_t len)
{
    uint32_t i;
    uint8_t val;

    val = 0;
    for(i= 0; i < len; i++)
    {
        buff[i] = val;
        val ++;
    }
}

uint32_t at_get_scene_index(uint32_t addr)
{
    uint32_t i;

    for(i = 0; i < 3; i++)
    {
        if(addr >= at_async_scene[i].region->start_addr
            && addr < at_async_scene[i].region->end_addr)
        {
            return i;
        }
    }
    return i;
}
void at_user_func(uint32_t param0,uint32_t param1);
void at_opera_result_notify(void* param)
{
    NORFLASH_API_OPERA_RESULT *opera_result;
    enum NORFLASH_API_RET_T result;
    uint32_t offs;
    uint32_t index;
    uint32_t i;
    //enum THREAD_USER_ID user_id;
    //uint32_t suspend = 1;

    opera_result = (NORFLASH_API_OPERA_RESULT*)param;

    AT_TRACE("%s:type = %d, addr = 0x%x,len = 0x%x,remain_num = %d,result = %d.",
                __func__,
                opera_result->type,
                opera_result->addr,
                opera_result->len,
                opera_result->remain_num,
                opera_result->result);
    // check.
    index = at_get_scene_index(opera_result->addr);
    ASSERT(index < 3, "%s:at_get_scene_index faile!,addr = 0x%x.",__func__,opera_result->addr);
    offs = (opera_result->addr&0xffffff) - ((opera_result->addr&0xffffff)/4096)*4096;

    if(opera_result->remain_num == 0)
    {
        result = norflash_sync_read(at_async_scene[index].mod_id,opera_result->addr,at_buff_r,opera_result->len);
        if(result != NORFLASH_API_OK)
        {
            ASSERT(0, "%s: hal_norflash_read failed, result = %d",__func__,result);
        }
        if(opera_result->type == NORFLASH_API_WRITTING)
        {
            if(memcmp(at_buff_r,at_buff_w + offs,opera_result->len) == 0)
            {
                AT_TRACE("%s: writting read back check ok.addr = 0x%x,len = 0x%x, offs = 0x%x.",
                    __func__,opera_result->addr,opera_result->len,offs);
                at_async_scene[index].succ_count ++;
            }
            else
            {
                 uint32_t remain_len;
                 AT_TRACE("%s: writting read back check failed.addr = 0x%x,len = 0x%x,offs = 0x%x.",
                    __func__,opera_result->addr,opera_result->len,offs);
                 for(i = 0; i < opera_result->len/32; i++)
                 {
                    DUMP8("0x%02x, ",at_buff_r + i*32,32);

                    DUMP8("0x%02x, ",at_buff_w + offs + i*32,32);
                 }
                 remain_len = opera_result->len - (opera_result->len/32)*32;
                 if(remain_len > 0)
                 {
                    DUMP8("0x%02x, ",at_buff_r + i*32,remain_len);

                    DUMP8("0x%02x, ",at_buff_w + offs + i*32,remain_len);
                 }
                 at_async_scene[index].fail_count ++;
            }
       }
       else
       {
            for(i = 0; i < AT_FLASH_SECTOR_SIZE; i++)
            {
                if(at_buff_r[i] != 0xff)
                {
                    break;
                }
            }
            if(i == AT_FLASH_SECTOR_SIZE)
            {
                at_async_scene[index].succ_count ++;
            }
            else
            {
                AT_TRACE("%s: erasing operation read back check failed.",__func__);
                at_async_scene[index].fail_count ++;
            }
       }
   }

   //user_id = (enum THREAD_USER_ID)index;
   //suspend = 1;
   //at_thread_user_enqueue_cmd(user_id,0,user_id,suspend,(uint32_t)at_user_func);

}

void at_user_func(uint32_t param0,uint32_t param1)
{
    enum NORFLASH_API_RET_T result;
    ASYNC_SCENE *pscane;
    enum THREAD_USER_ID user_id;
    uint32_t suspend = 1;

    user_id = (enum THREAD_USER_ID)param0;
    suspend = param1;
    pscane = &at_async_scene[user_id];
    pscane->state = NORFLASH_ASYNC_STATE_INIT;

    suspend = suspend;
    AT_TRACE("%s,%d:user_id = %d,suspend = %d.",__func__,__LINE__,user_id,suspend);
    while(1)
    {
        if(g_at_test_break || pscane->fail_count > 0)
        {
            pscane->state = NORFLASH_ASYNC_STATE_END;
            break;
        }

        if(pscane->state == NORFLASH_ASYNC_STATE_INIT)
        {
            AT_TRACE("%s,%d,NORFLASH_ASYNC_STATE_INIT",__func__,__LINE__);
            pscane->e_addr = pscane->region->start_addr;
            pscane->w_offs = 0;
            pscane->len_index = 0;
            pscane->succ_count = 0;
            pscane->fail_count = 0;
            pscane->tried_count = 0;
            pscane->state = NORFLASH_ASYNC_STATE_ERASE;
        }
        else if(pscane->state == NORFLASH_ASYNC_STATE_ERASE)
        {
            if(pscane->e_addr >= pscane->region->end_addr)
            {
                AT_TRACE("%s,%d: NORFLASH_ASYNC_STATE_ERASE, has arrived to end_addr.addr = 0x%x",
                        __func__,
                        __LINE__,
                        pscane->e_addr);
                pscane->state = NORFLASH_ASYNC_STATE_WAIT;
                continue;
            }
            result = norflash_api_erase(pscane->mod_id,
                        pscane->e_addr,
                        AT_FLASH_SECTOR_SIZE,
                        true);
            if(result == NORFLASH_API_OK)
            {
                AT_TRACE("%s,%d: NORFLASH_ASYNC_STATE_ERASE,erase ok!addr = 0x%x",
                        __func__,
                        __LINE__,
                        pscane->e_addr);
                pscane->w_offs = 0;
                pscane->len_index = 0;
                pscane->state = NORFLASH_ASYNC_STATE_WRITTING;
                pscane->tried_count = 0;
            }
            else if(result == NORFLASH_API_BUFFER_FULL)
            {
                if(pscane->tried_count == 0)
                {
                    AT_TRACE("%s,%d: NORFLASH_ASYNC_STATE_ERASE,too more!addr = 0x%x",
                            __func__,
                            __LINE__,
                            pscane->e_addr);
                }
                pscane->tried_count++;
                norflash_api_flush();
                //osDelay(100);
            }
            else
            {
                AT_TRACE("%s,%d: NORFLASH_ASYNC_STATE_ERASE,erase failed!addr = 0x%x,result = %d.",
                        __func__,
                        __LINE__,
                        pscane->e_addr,result);
            }
        }
        else if(pscane->state == NORFLASH_ASYNC_STATE_WRITTING)
        {
            if(pscane->len_index >= sizeof(g_at_len)/sizeof(g_at_len[0]))
            {
                AT_TRACE("%s,%d:NORFLASH_ASYNC_STATE_WRITTING,has arrived to sector end. w_offs = 0x%x",
                        __func__,
                        __LINE__,
                        pscane->w_offs);
                pscane->e_addr += AT_FLASH_SECTOR_SIZE;
                pscane->state = NORFLASH_ASYNC_STATE_ERASE;
                continue;
            }

            if(norflash_api_get_used_buffer_count(pscane->mod_id,NORFLASH_API_ALL) > 0)
            {
                norflash_api_flush();
                continue;
            }

            result = norflash_api_write(pscane->mod_id,
                            pscane->e_addr + pscane->w_offs,
                            at_buff_w + pscane->w_offs,
                            g_at_len[pscane->len_index],
                            true);
            if(result == NORFLASH_API_OK)
            {
                AT_TRACE("%s,%d NORFLASH_ASYNC_STATE_WRITTING,norflash_api_write ok!addr = 0x%x",
                        __func__,
                        __LINE__,
                        pscane->e_addr + pscane->w_offs);
                pscane->w_offs += g_at_len[pscane->len_index];
                pscane->len_index ++;
                pscane->tried_count = 0;
            }
            else if(result == NORFLASH_API_BUFFER_FULL)
            {

                if(pscane->tried_count == 0)
                {
                     AT_TRACE("%s,%d: NORFLASH_ASYNC_STATE_WRITTING, too more!addr = 0x%x",
                            __func__,
                            __LINE__,
                            pscane->e_addr + pscane->w_offs);
                }
                pscane->tried_count++;
                norflash_api_flush();
                //osDelay(100);
            }
            else
            {
                AT_TRACE("%s,%d: NORFLASH_ASYNC_STATE_WRITTING writting failed!addr = 0x%x,result = %d.",
                        __func__,
                        __LINE__,
                        pscane->e_addr + pscane->w_offs,
                        result);
                goto _func_fail;
            }
        }
        else if(pscane->state == NORFLASH_ASYNC_STATE_WAIT)
        {
            //AT_TRACE("%s,%d,NORFLASH_ASYNC_STATE_WAIT",__func__,__LINE__);
            if(norflash_api_buffer_is_free(pscane->mod_id))
            {
                AT_TRACE("%s,%d: NORFLASH_ASYNC_STATE_WAIT,norflash_api_buffer_is_free is true.",
                            __func__,
                            __LINE__);
                pscane->state = NORFLASH_ASYNC_STATE_END;
                pscane->tried_count = 0;
            }
            else
            {
                if(pscane->tried_count == 0)
                {
                     AT_TRACE("%s,%d: NORFLASH_ASYNC_STATE_WAIT,norflash_api_buffer_is_free is false ",
                            __func__,
                            __LINE__);
                }
                pscane->tried_count++;
                norflash_api_flush();
                //osDelay(100);
            }
        }
        else if(pscane->state == NORFLASH_ASYNC_STATE_END)
        {
            AT_TRACE("%s,%d,NORFLASH_ASYNC_STATE_END",__func__,__LINE__);
            break;
        }
    }
    AT_TRACE("%s,%d,user_id = %d",__func__,__LINE__,user_id);
    return;
_func_fail:
    AT_TRACE("%s: norflash_api failed! state = %d.", __func__, pscane->state);

}

void at_norflash_async_init(enum THREAD_USER_ID user_id)
{
    ASYNC_SCENE *pscene;
    enum NORFLASH_API_RET_T result;
    uint32_t sector_size = 0;
    uint32_t block_size = 0;
    uint32_t page_size = 0;

    ASSERT(user_id < 3,"%s:user_id == %d.",__func__,user_id);
    AT_TRACE("%s,%d: user_id = %d.",__func__,__LINE__,user_id);
    hal_norflash_get_size(HAL_NORFLASH_ID_0,
           NULL,
           &block_size,
           &sector_size,
           &page_size);
    pscene = &at_async_scene[user_id];
    result = norflash_api_register(pscene->mod_id,
                    HAL_NORFLASH_ID_0,
                    pscene->region->start_addr,
                    pscene->region->end_addr - pscene->region->start_addr,
                    block_size,
                    sector_size,
                    page_size,
                    pscene->buff_len,
                    at_opera_result_notify);
    ASSERT(result == NORFLASH_API_OK, "%s:user_id == %d,result = %d.",__func__,user_id,(int)result);

}

void at_norflash_suspend(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    uint32_t i;
    uint32_t start_time;
    static bool is_fst = true;

    param0 = param0;
    cmd_id = cmd_id;
    AT_TRACE("I am at_norflash_suspend");
    g_at_test_break = false;
    for(i = 0; i < sizeof(region_info)/sizeof(AT_FLASH_REGION_INFO); i++)
    {
        AT_TRACE("regin[%d]: start = 0x%x, end = 0x%x.",region_info[i].id,region_info[i].start_addr,region_info[i].end_addr);

    }
    if(is_fst)
    {
        at_w_buff_set(at_buff_w,AT_FLASH_SECTOR_SIZE);

        at_norflash_async_init(THREAD_USER0);
        at_norflash_async_init(THREAD_USER1);
        //  at_norflash_async_init(THREAD_USER2);
        is_fst = false;
    }
    if(!at_thread_user_is_inited(THREAD_USER0))
    {
        at_thread_user_init(THREAD_USER0);
    }

    if(!at_thread_user_is_inited(THREAD_USER1))
    {
        at_thread_user_init(THREAD_USER1);
    }

   // if(!at_thread_user_is_inited(THREAD_USER2))
    {
   //     at_thread_user_init(THREAD_USER2);
    }
    // param0: user_id
    // praram1: suspend
    osDelay(100);
    at_thread_user_enqueue_cmd(THREAD_USER0,0,THREAD_USER0,1,(uint32_t)at_user_func);
    //osDelay(100);
    at_thread_user_enqueue_cmd(THREAD_USER1,0,THREAD_USER1,1,(uint32_t)at_user_func);
    //osDelay(100);
  //  at_thread_user_enqueue_cmd(THREAD_USER2,0,THREAD_USER2,1,(uint32_t)at_user_func);
    start_time = hal_sys_timer_get();
    while(1)
    {
        if(TICKS_TO_MS(hal_sys_timer_get() - start_time) >= 1000*60) //  timeout 60second
        {
            AT_TRACE("%s: wait out time!");
            break;
        }
        if(at_async_scene[0].state == NORFLASH_ASYNC_STATE_END
           && at_async_scene[1].state == NORFLASH_ASYNC_STATE_END
           )
        {
            if(at_async_scene[0].fail_count == 0
               && at_async_scene[1].fail_count == 0
               )
            {
                AUTO_TEST_SEND(AT_CMD_RESP_NORFLASH_SUSPEND);
                break;
            }
            else
            {
                break;
            }
        }
        else if(at_async_scene[0].fail_count > 0
           || at_async_scene[1].fail_count > 0
           )
        {
            g_at_test_break = true;
        }
        else
        {
            AT_TRACE("%s: is runnig.time = %d(ms).",__func__,TICKS_TO_MS(hal_sys_timer_get() - start_time));
            osDelay(500);
        }

    }

//_func_fail:
//    AT_TRACE("_debug:Flash testing fail.error_code = %d.",error_code);

}

int32_t _at_memory_test(uint8_t* buff,uint32_t size)
{
    uint8_t value;
    uint32_t i;
    int32_t ret = 0;

    value = 0;
    for(i = 0; i < size; i++)
    {
        buff[i] = value++;
    }
    value = 0;
    for(i = 0; i < size; i++)
    {
        if(buff[i] != value++)
        {
            ret = 2;
            goto _func_end;
        }
    }
_func_end:
    return ret;
}

void at_memory(uint32_t param0,uint32_t param1)
{
    AT_CMD_ID_ENUM_T cmd_id = (AT_CMD_ID_ENUM_T)param1;
    cmd_id = cmd_id;
    uint8_t *buff;
    uint8_t error_code = 1;
    int32_t result;

    error_code = error_code;
    //AT_TRACE("_debug: I am at_memory");
    buff = (uint8_t*)at_buff_r;
    result = _at_memory_test(buff,AT_FLASH_SECTOR_SIZE);
    if(result != 0)
    {
        error_code = 2;
        goto _func_fail;
    }

    buff = (uint8_t*)at_buff_w;
    result = _at_memory_test(buff,AT_FLASH_SECTOR_SIZE);
    if(result != 0)
    {
        error_code = 2;
        goto _func_fail;
    }

    buff = (uint8_t*)at_buff_b;
    result = _at_memory_test(buff,AT_FLASH_SECTOR_SIZE);
    if(result != 0)
    {
        error_code = 3;
        goto _func_fail;
    }
    error_code = 0;
    AUTO_TEST_SEND(AT_CMD_RESP_MEMORY);
_func_fail:
    AT_TRACE("_debug:Memory testing fail.error_code = %d.",error_code);

}

int32_t at_Init(void)
{
    int32_t ret;

    AT_TRACE("at_Init.");
    ret = at_os_init();
    return ret;
}
#endif// (_AUTO_TEST_)


