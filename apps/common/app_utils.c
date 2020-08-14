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
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_wdt.h"
#include "hal_sleep.h"
#include "pmu.h"
#include "analog.h"
#include "app_thread.h"
#include "app_utils.h"

static uint32_t app_wdt_ping_time = 0;
static uint32_t app_wdt_ping_time_thr = 0;

int app_sysfreq_req(enum APP_SYSFREQ_USER_T user, enum APP_SYSFREQ_FREQ_T freq)
{
    int ret;

    ret = hal_sysfreq_req((enum HAL_SYSFREQ_USER_T)user, (enum HAL_CMU_FREQ_T)freq);

    return ret;
}

static int app_wdt_kick(void)
{
    hal_wdt_ping(HAL_WDT_ID_0);
    pmu_wdt_feed();
    return 0;
}

static int app_wdt_handle_process(APP_MESSAGE_BODY *msg_body)
{
    app_wdt_kick();
    app_wdt_thread_check_handle();

    return 0;
}

static void app_wdt_irq_handle(enum HAL_WDT_ID_T id, uint32_t status)
{
    TRACE("%s id:%d status:%d",__func__, id, status);
    analog_aud_codec_mute();
    pmu_wdt_stop();
    pmu_wdt_config(5, 5);
    pmu_wdt_start();    
    TRACE_IMM(" ");
    hal_sys_timer_delay(MS_TO_TICKS(50));
    ASSERT(0, "%s id:%d status:%d",__func__, id, status);
}

void app_wdt_ping(void)
{
    APP_MESSAGE_BLOCK msg;
    uint32_t time = hal_sys_timer_get();
    if  ((time - app_wdt_ping_time)>app_wdt_ping_time_thr){
        app_wdt_ping_time = time;
        msg.mod_id = APP_MODUAL_WATCHDOG;
        app_mailbox_put(&msg);
    }
}

int app_wdt_open(int seconds)
{
    uint32_t lock = int_lock();

    hal_wdt_set_irq_callback(HAL_WDT_ID_0, app_wdt_irq_handle);
    hal_wdt_set_timeout(HAL_WDT_ID_0, seconds);
    hal_wdt_start(HAL_WDT_ID_0);    
    pmu_wdt_config(seconds * 1000, seconds * 1000);
    pmu_wdt_start();
    app_wdt_thread_check_open();
    app_wdt_ping_time_thr = MS_TO_TICKS(seconds * 1000 / 5);
    int_unlock(lock);
 
    hal_sleep_set_deep_sleep_hook(HAL_DEEP_SLEEP_HOOK_USER_WDT, app_wdt_kick);
    app_set_threadhandle(APP_MODUAL_WATCHDOG, app_wdt_handle_process);

    return 0;
}

int app_wdt_reopen(int seconds)
{
    uint32_t lock = int_lock();
    hal_wdt_stop(HAL_WDT_ID_0);
    hal_wdt_set_timeout(HAL_WDT_ID_0, seconds);
    hal_wdt_start(HAL_WDT_ID_0);    
    pmu_wdt_config(seconds * 1000, seconds * 1000);
    pmu_wdt_start();
    app_wdt_ping_time_thr = MS_TO_TICKS(seconds * 1000 / 5);
    int_unlock(lock);

    return 0;
}

int app_wdt_close(void)
{
    uint32_t lock = int_lock();
    hal_wdt_stop(HAL_WDT_ID_0);
    pmu_wdt_stop();
    int_unlock(lock);

    return 0;
}

typedef struct {
    uint32_t kick_time;
    uint32_t timeout;
    bool enable;
}APP_WDT_THREAD_CHECK;

APP_WDT_THREAD_CHECK app_wdt_thread_check[APP_WDT_THREAD_CHECK_USER_QTY];

uint32_t __inline__ app_wdt_thread_tickdiff_calc(uint32_t curr_ticks, uint32_t prev_ticks)
{
    if(curr_ticks < prev_ticks)
        return ((0xffffffff  - prev_ticks + 1) + curr_ticks);
    else
        return (curr_ticks - prev_ticks);
}

int app_wdt_thread_check_open(void)
{
    uint8_t i=0;

    for (i=0; i<APP_WDT_THREAD_CHECK_USER_QTY; i++){
        app_wdt_thread_check[i].kick_time = 0;
        app_wdt_thread_check[i].timeout = 0;
        app_wdt_thread_check[i].enable = false;
    }
    
    return 0;
}

int app_wdt_thread_check_enable(enum APP_WDT_THREAD_CHECK_USER_T user, int seconds)
{
    ASSERT(user < APP_WDT_THREAD_CHECK_USER_QTY, "%s user:%d seconds:%d",__func__, user, seconds);

    app_wdt_thread_check[user].kick_time = hal_sys_timer_get();    
    app_wdt_thread_check[user].timeout = MS_TO_TICKS(1000*seconds);
    app_wdt_thread_check[user].enable = true;

    return 0;
}

int app_wdt_thread_check_disable(enum APP_WDT_THREAD_CHECK_USER_T user)
{
    app_wdt_thread_check[user].kick_time = 0;
    app_wdt_thread_check[user].enable = false;

    return 0;
}

int app_wdt_thread_check_ping(enum APP_WDT_THREAD_CHECK_USER_T user)
{
    if (app_wdt_thread_check[user].enable){
        app_wdt_thread_check[user].kick_time = hal_sys_timer_get();
    }
    return 0;
}

int app_wdt_thread_check_handle(void)
{
    uint32_t curtime = hal_sys_timer_get();
    uint32_t diff, i;

    for (i=0; i<APP_WDT_THREAD_CHECK_USER_QTY; i++){
        if (app_wdt_thread_check[i].enable){
            diff = app_wdt_thread_tickdiff_calc(curtime, app_wdt_thread_check[i].kick_time);
            if (diff >= app_wdt_thread_check[i].timeout){
                ASSERT(0, "%s user:%d",__func__, i, diff);
            }
        }
    }

    return 0;
}
