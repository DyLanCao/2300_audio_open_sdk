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
#include "cp_accel.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_cmu.h"
#include "hal_location.h"
#include "hal_mcu2cp.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "stdarg.h"
#include "system_cp.h"

#ifdef CP_ACCEL_DEBUG
#define CP_ACCEL_TRACE(s, ...)              TRACE(s, ##__VA_ARGS__)
#else
#define CP_ACCEL_TRACE(s, ...)
#endif

static bool ram_inited;
static bool cp_accel_inited;
static CP_ACCEL_EVT_HDLR mcu_evt_hdlr;

static CP_ACCEL_CP_MAIN cp_work_main;
static CP_ACCEL_EVT_HDLR cp_evt_hdlr;

static CP_BSS_LOC volatile uint8_t cp_irq_cnt;

static CP_TEXT_SRAM_LOC unsigned int mcu2cp_msg_arrived(const unsigned char *data, unsigned int len)
{
    cp_irq_cnt = 1;

    if (cp_evt_hdlr) {
        cp_evt_hdlr((uint32_t)data);
    }

    return 0;
}

static CP_TEXT_SRAM_LOC NOINLINE void accel_loop(void)
{
    uint32_t lock;
    uint32_t cnt;

    while (1) {
        lock = int_lock();
        cnt = cp_irq_cnt;
        cp_irq_cnt = 0;
        if (cnt == 0) {
            __WFI();
        }
        int_unlock(lock);

        while (cnt > 0) {
            if (cp_work_main) {
                cp_work_main();
            }
            cnt = 0;
        }
    }
}

static void accel_main(void)
{
    system_cp_init(!ram_inited);

    ram_inited = true;

    hal_mcu2cp_open_cp(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, mcu2cp_msg_arrived, NULL, false);

    hal_mcu2cp_start_recv_cp(HAL_MCU2CP_ID_0);

    cp_accel_inited = true;

    accel_loop();
}

static SRAM_TEXT_LOC unsigned int cp2mcu_msg_arrived(const unsigned char *data, unsigned int len)
{
    if (mcu_evt_hdlr) {
        mcu_evt_hdlr((uint32_t)data);
    }

    return 0;
}

int cp_accel_open(CP_ACCEL_CP_MAIN cp_main, CP_ACCEL_EVT_HDLR cp_hdlr, CP_ACCEL_EVT_HDLR mcu_hdlr)
{
    cp_work_main = cp_main;
    cp_evt_hdlr = cp_hdlr;
    mcu_evt_hdlr = mcu_hdlr;

    hal_cmu_cp_enable(RAMCP_BASE + RAMCP_SIZE, (uint32_t)accel_main);

    hal_mcu2cp_open_mcu(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, cp2mcu_msg_arrived, NULL, false);

    hal_mcu2cp_start_recv_mcu(HAL_MCU2CP_ID_0);

    return 0;
}

int cp_accel_close(void)
{
    cp_accel_inited = false;

    hal_mcu2cp_close_mcu(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0);

    hal_cmu_cp_disable();

    system_cp_term();

    hal_mcu2cp_close_cp(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0);

    return 0;
}

int SRAM_TEXT_LOC cp_accel_init_done(void)
{
    return cp_accel_inited;
}

int SRAM_TEXT_LOC cp_accel_send_event_mcu2cp(uint32_t event)
{
    return hal_mcu2cp_send_mcu(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, (unsigned char *)event, 0);
}

int CP_TEXT_SRAM_LOC cp_accel_send_event_cp2mcu(uint32_t event)
{
    return hal_mcu2cp_send_cp(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, (unsigned char *)event, 0);
}

