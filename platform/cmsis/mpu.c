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
#include "mpu.h"
#include "cmsis.h"

int mpu_open(void)
{
    int i;

    if (MPU->TYPE == 0) {
        return 1;
    }

    ARM_MPU_Disable();

    for (i = 0; i < MPU_ID_QTY; i++) {
        ARM_MPU_ClrRegion(i);
    }

    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);

    return 0;
}

int mpu_close(void)
{
    ARM_MPU_Disable();

    return 0;
}

int mpu_set(enum MPU_ID_T id, uint32_t addr, uint32_t len, enum MPU_ATTR_T attr)
{
    int ret;
    uint32_t rbar;
    uint32_t rasr;
    uint8_t xn;
    uint8_t ap;
    uint8_t size;

    if ((MPU->CTRL & MPU_CTRL_ENABLE_Msk) == 0) {
        ret = mpu_open();
        if (ret) {
            return ret;
        }
    }

    if (id >= MPU_ID_QTY) {
        return 2;
    }
    if (len < 32 || (len & (len - 1))) {
        return 3;
    }
    if (addr & (len - 1)) {
        return 4;
    }
    if (attr >= MPU_ATTR_QTY) {
        return 5;
    }

    if (attr == MPU_ATTR_READ_WRITE_EXEC || attr == MPU_ATTR_READ_EXEC ||
            attr == MPU_ATTR_EXEC) {
        xn = 0;
    } else {
        xn = 1;
    }

    ap = ARM_MPU_AP_NONE;
    if (attr == MPU_ATTR_READ_WRITE_EXEC || attr == MPU_ATTR_READ_WRITE) {
        ap = ARM_MPU_AP_FULL;
    } else if (attr == MPU_ATTR_READ_EXEC || attr == MPU_ATTR_READ) {
        ap = ARM_MPU_AP_RO;
    }

    size = __CLZ(__RBIT(len)) - 1;

    rbar = ARM_MPU_RBAR(id, addr);
    rasr = ARM_MPU_RASR(xn, ap, 0, 1, 1, 1, 0, size);

    ARM_MPU_SetRegion(rbar, rasr);

    return 0;
}

int mpu_clear(enum MPU_ID_T id)
{
    if (id >= MPU_ID_QTY) {
        return 2;
    }

    ARM_MPU_ClrRegion(id);

    return 0;
}

int mpu_null_check_enable(void)
{
    int ret;

    ret = mpu_set(MPU_ID_NULL_POINTER, 0, 32, MPU_ATTR_NO_ACCESS);
    return ret;
}
