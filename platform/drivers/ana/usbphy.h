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
#ifndef __USBPHY_H__
#define __USBPHY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "hal_analogif.h"
#include "hal_cmu.h"
#include "plat_addr_map.h"
#include CHIP_SPECIFIC_HDR(usbphy)

#ifdef ISPI_USBPHY_REG
#define usbphy_read(reg,val)            hal_analogif_reg_read(ISPI_USBPHY_REG(reg),val)
#define usbphy_write(reg,val)           hal_analogif_reg_write(ISPI_USBPHY_REG(reg),val)
#else
int usbphy_read(unsigned short reg, unsigned short *val);

int usbphy_write(unsigned short reg, unsigned short val);
#endif

void usbphy_open(void);

void usbphy_sleep(void);

void usbphy_wakeup(void);

#ifdef __cplusplus
}
#endif

#endif

