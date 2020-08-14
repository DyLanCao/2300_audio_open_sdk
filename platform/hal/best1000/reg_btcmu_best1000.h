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
#ifndef __REG_BTCMU_BEST1000_H__
#define __REG_BTCMU_BEST1000_H__

#include "plat_types.h"

struct BTCMU_T {
         uint32_t RESERVED_00[0x50 / 4];
    __IO uint32_t ISIRQ_SET;        // 0x50
    __IO uint32_t ISIRQ_CLR;        // 0x54
         uint32_t RESERVED_58[0x98 / 4];
    __IO uint32_t MISC;             // 0xF0
};

// MISC
#define BTCMU_DISABLE_FORCE_PU_OSC_DOWN     (1 << 31)

#endif

