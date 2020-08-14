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
#ifndef __BT_STATUS_CONV_H__
#define __BT_STATUS_CONV_H__
#include "bluetooth.h"
#include "bttypes.h"

extern const bt_status_t status_table[];
static inline bt_status_t bt_convert_status(BtStatus stack_status)
{
    if (stack_status <= BT_STATUS_LAST_CODE)
        return status_table[stack_status];
    else
        return BT_STS_LAST_CODE;
}

#define BT_STATUS(stack_status) bt_convert_status(stack_status)

#endif/*__BT_STATUS_CONV_H__*/
