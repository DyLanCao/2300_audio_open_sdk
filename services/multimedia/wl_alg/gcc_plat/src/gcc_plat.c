/*
 *  Copyright (c) 2020 The wl project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifdef GCC_PLAT

#include "hal_trace.h"
#include "plat_types.h"

void gcc_plat_init(void)
{
    TRACE("gcc_plat_init success");
}



void gcc_plat_process(short *left_input,short *right_input, uint32_t frame_len)
{
    uint32_t left_amount = 0, right_amount = 0,left_avg = 0,right_avg = 0,compare_value = 0;
    for(int icnt = 0; icnt < frame_len; icnt++)
    {
            left_amount += ABS(left_input[icnt]);
            right_amount += ABS(right_input[icnt]);
    }

    if(frame_len == 0 || right_amount == 0)
    {
        TRACE("frame_len:%d right_amount:%d  exit ",frame_len,right_amount);
        return;
    }

    left_avg = left_amount/frame_len;
    right_avg = right_amount/frame_len;

    compare_value = left_avg*1000/right_avg;

    TRACE("left_avg:%d right_avg:%d compare_value:%d ",left_avg,right_avg,compare_value);

}

void gcc_plat_exit(void)
{

    TRACE("gcc_plat_exit success");

}

#endif
