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
#ifndef __MPU_H__
#define __MPU_H__

#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum MPU_ID_T {
    MPU_ID_NULL_POINTER = 0,
    MPU_ID_1,
    MPU_ID_2,
    MPU_ID_3,
    MPU_ID_4,
    MPU_ID_5,
    MPU_ID_6,
    MPU_ID_7,

    MPU_ID_QTY,
};

enum MPU_ATTR_T {
    MPU_ATTR_READ_WRITE_EXEC = 0,
    MPU_ATTR_READ_EXEC,
    MPU_ATTR_EXEC,
    MPU_ATTR_READ_WRITE,
    MPU_ATTR_READ,
    MPU_ATTR_NO_ACCESS,

    MPU_ATTR_QTY,
};

int mpu_open(void);

int mpu_close(void);

// VALID LENGTH: 32, 64, 128, 256, 512, 1K, 2K, ..., 4G
// ADDR must be aligned to len
int mpu_set(enum MPU_ID_T id, uint32_t addr, uint32_t len, enum MPU_ATTR_T attr);

int mpu_clear(enum MPU_ID_T id);

int mpu_null_check_enable(void);

#ifdef __cplusplus
}
#endif

#endif

