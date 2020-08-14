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
#ifndef __REG_BTCMU_BEST2000_H__
#define __REG_BTCMU_BEST2000_H__

#include "plat_types.h"

struct BTCMU_T {
    __IO uint32_t CLK_ENABLE;           // 0x00
    __IO uint32_t CLK_DISABLE;          // 0x04
    __IO uint32_t CLK_MODE;             // 0x08
    __IO uint32_t DIV_TIMER;            // 0x0C
    __IO uint32_t RESET_SET;            // 0x10
    __IO uint32_t RESET_CLR;            // 0x14
    __IO uint32_t DIV_WDT;              // 0x18
    __IO uint32_t RESET_PULSE;          // 0x1C
         uint32_t RESERVED[0x30 / 4];   // 0x20
    __IO uint32_t ISIRQ_SET;            // 0x50
    __IO uint32_t ISIRQ_CLR;            // 0x54
};

#define BT_HCLK_A2A_CODEC               (1 << 17)
#define BT_HRST_A2A_CODEC               (1 << 17)

#endif

