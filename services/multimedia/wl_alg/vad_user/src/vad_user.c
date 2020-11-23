/*
 *  Copyright (c) 2020 The wl project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifdef WL_VAD

/*
 *  Copyright (c) 
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */



#include "webrtc_vad.h"
#include "hal_trace.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>  // size_t
#include <stdlib.h>
#include <string.h>


#define VAD_MIN_THD 100
#define VAD_MAX_THD 200

static pVadInst phandle = NULL;



void wl_vad_init(int mode)
{
	int flag;

	flag = wl_WebRtcVad_Create(&phandle);

	TRACE("vad flag:%d ",flag);

	flag = wl_WebRtcVad_Init(phandle);
	TRACE("vad aa flag:%d ",flag);

	flag = wl_WebRtcVad_set_mode(phandle, mode);

	TRACE("vad init success :flag:%d, mode is:%d ",flag, mode);
}

bool vad_step_count_process(int vad_cnt)
{
	static uint32_t tmp_count = 0, last_count = 0;

	bool vad_ret = 0;

	if(vad_cnt)
	{
		tmp_count++;
	}
	else
	{
		tmp_count=0;
	}

	if(tmp_count < last_count)
	{
		TRACE("tmp_count:%d last_count:%d ",tmp_count,last_count);

		if((last_count > VAD_MIN_THD) && (last_count < VAD_MAX_THD))
		{
			vad_ret = 1;
		}
	}
	
	last_count = tmp_count;

	return vad_ret;
}

int wl_vad_process_frame(short *buffer, int len)
{
    int vad_state = 0;
	bool vad_value = OFF;

    vad_state = wl_WebRtcVad_Process(phandle, 16000, buffer, len);

	vad_value =vad_step_count_process(vad_state);

    if(vad_value)
    {
	    TRACE("man voice detected:%d  ",vad_value);
    }
	
	return vad_state;
}
void vad_deinit()
{
    ;
}


#endif
