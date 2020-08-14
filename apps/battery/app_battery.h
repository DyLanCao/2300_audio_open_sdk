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
#ifndef __APP_BATTERY_H__
#define __APP_BATTERY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define APP_BATTERY_SET_MESSAGE(appevt, status, volt) (appevt = (((uint32_t)status&0xffff)<<16)|(volt&0xffff))
#define APP_BATTERY_GET_STATUS(appevt, status) (status = (appevt>>16)&0xffff)
#define APP_BATTERY_GET_VOLT(appevt, volt) (volt = appevt&0xffff)

#define APP_BATTERY_LEVEL_MIN (0)

#ifdef __INTERCONNECTION__
#define APP_BATTERY_LEVEL_MAX (100)
#define APP_BATTERY_DEFAULT_INFO 0xE4

typedef struct
{
    uint8_t chargingStatus : 1;
    uint8_t batteryLevel : 7;
}APP_BATTERY_INFO_T;

uint8_t* app_battery_get_mobile_support_self_defined_command_p(void);
#else // #ifdef __INTERCONNECTION__
#define APP_BATTERY_LEVEL_MAX (9)
#endif // #ifdef __INTERCONNECTION__

#define APP_BATTERY_LEVEL_NUM (APP_BATTERY_LEVEL_MAX-APP_BATTERY_LEVEL_MIN+1)

enum APP_BATTERY_STATUS_T {
    APP_BATTERY_STATUS_NORMAL,
    APP_BATTERY_STATUS_CHARGING,
    APP_BATTERY_STATUS_OVERVOLT,
    APP_BATTERY_STATUS_UNDERVOLT,
    APP_BATTERY_STATUS_PDVOLT,
    APP_BATTERY_STATUS_INVALID,

    APP_BATTERY_STATUS_QTY
};

typedef uint16_t APP_BATTERY_MV_T;

typedef void (*APP_BATTERY_CB_T)(APP_BATTERY_MV_T currvolt, uint8_t currlevel,
    enum APP_BATTERY_STATUS_T status);


int app_battery_get_info(APP_BATTERY_MV_T *currvolt, uint8_t *currlevel, enum APP_BATTERY_STATUS_T *status);

/**
 * Battery App owns its own context where it will periodically update the
 * battery level and state. Makes sense for it to act as a Model and publish
 * battery change events to any interested users.
 *
 * Note: Callback will execute out of BES "App Thread" context.
 *
 * TODO: ideally implemented as proper observer pattern with support for multiple
 *       listeners
 *
 * Returns
 *  - 0: On success
 *  - 1: If registration failed
 */
int app_battery_register(APP_BATTERY_CB_T user_cb);

int app_battery_open(void);

int app_battery_start(void);

int app_battery_stop(void);

int app_battery_close(void);

int app_battery_charger_indication_open(void);

int8_t app_battery_current_level(void);

int ntc_capture_open(void);

int ntc_capture_start(void);

#ifdef __cplusplus
}
#endif


#endif
