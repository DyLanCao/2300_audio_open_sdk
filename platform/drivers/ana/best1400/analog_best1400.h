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
#ifndef __ANALOG_BEST1400_H__
#define __ANALOG_BEST1400_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ISPI_ANA_REG(reg)                   (((reg) & 0xFFF) | 0x1000)

#define MAX_ANA_MIC_CH_NUM                  1

uint32_t analog_aud_get_max_dre_gain(void);

#ifdef __cplusplus
}
#endif

#endif

