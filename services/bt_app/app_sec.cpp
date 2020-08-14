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
#include "apps.h"
#include "hal_trace.h"
#include "bt_if.h"

void pair_handler_func(enum pair_event evt, int err_code)
{
    switch(evt) {
    case PAIR_EVENT_NUMERIC_REQ:
        break;
    case PAIR_EVENT_COMPLETE:
#if defined(_AUTO_TEST_)
        AUTO_TEST_SEND("Pairing ok.");
#endif

#ifndef FPGA
#ifdef MEDIA_PLAYER_SUPPORT
        if (err_code == 0) {
            app_voice_report(APP_STATUS_INDICATION_PAIRSUCCEED,0);
        } else {
            app_voice_report(APP_STATUS_INDICATION_PAIRFAIL,0);
        }
#endif
#endif
        break;
    default:
        break;
    }
}

void auth_handler_func(void)
{
    /*currently do nothing*/
    return;
}
