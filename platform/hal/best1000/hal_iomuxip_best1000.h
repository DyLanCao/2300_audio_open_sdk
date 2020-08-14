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
#ifndef __HAL_IOMUXIP_H__
#define __HAL_IOMUXIP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "plat_addr_map.h"
#include "reg_iomuxip_best1000.h"

#define IOMUXIP_REG_BASE IOMUX_BASE

#define iomuxip_read32(b,a) \
     (*(volatile uint32_t *)(b+a))

#define iomuxip_write32(v,b,a) \
     ((*(volatile uint32_t *)(b+a)) = v)

#ifdef __cplusplus
}
#endif

#endif /* __HAL_IOMUXIP_H__ */
