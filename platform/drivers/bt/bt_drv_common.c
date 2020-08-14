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
#include "plat_types.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "hal_chipid.h"
#include "bt_drv.h"
#include "bt_drv_interface.h"
#include "string.h"

#define SLOT_SIZE           625 
#define MAX_SLOT_CLOCK      ((1L<<27) - 1)
#define CLK_SUB(clock_a, clock_b)     ((uint32_t)(((clock_a) - (clock_b)) & MAX_SLOT_CLOCK))
#define CLK_ADD_2(clock_a, clock_b)     ((uint32_t)(((clock_a) + (clock_b)) & MAX_SLOT_CLOCK))

///only used for bt chip write patch data for speed up 
void btdrv_memory_copy(uint32_t *dest,const uint32_t *src,uint16_t length)
{
   // memcpy(dest,src,length);
   uint16_t i;
   for(i=0;i<length/4;i++)
   {
       *dest++ = *src++;
   }
}


extern uint8_t sleep_param[];
void btdrv_set_lpo_times(void)
{
    *(uint32_t *)0xd0330044 = (uint32_t)(sleep_param[4] - 1);
}

int btdrv_slave2master_clkcnt_convert(uint32_t local_clk, uint16_t local_cnt, 
                                               int32_t clk_offset, uint16_t bit_offset,
                                               uint32_t *master_clk, uint16_t *master_cnt)
{
    // Adjust bit offset and clock offset if needed
    uint32_t new_clk;
    int16_t new_cnt;

    new_clk = CLK_ADD_2(local_clk, clk_offset);
    new_cnt = (int16_t)local_cnt + bit_offset;
    
    if (new_cnt > SLOT_SIZE){
        new_cnt -= SLOT_SIZE;
        new_clk = CLK_SUB(new_clk, 1);
    }
    
    *master_clk = new_clk;
    *master_cnt = new_cnt;

    return 0;
}

int btdrv_clkcnt_diff(int32_t clk1, int16_t cnt1, 
                         int32_t clk2, int16_t cnt2,
                         int32_t *diff_clk, uint16_t *diff_bit)
{
    int32_t new_clk;
    int16_t new_cnt;
    int diff_us;
    
    new_clk = (int32_t)clk1 - (int32_t)clk2;   
    new_cnt = cnt2 - cnt1;
    if (new_cnt < 0){
        new_cnt += SLOT_SIZE;
        new_clk--;
    }
    *diff_clk = new_clk;
    *diff_bit = new_cnt;

    diff_us = new_clk * SLOT_SIZE + new_cnt;

    return diff_us;
}


