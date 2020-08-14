/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "noise_suppression_x.h"

#include <stdlib.h>



#include "real_fft.h"
#include "wl_nsx_core.h"
#include "nsx_defines.h"
#include "hal_trace.h"
#include "plat_types.h"

int wl_WebRtcNsx_Create(NsxHandle** nsxInst, uint8_t* nsx_heap) {
    #if 0
  NsxInst_t* self = malloc(sizeof(NsxInst_t));
    ADEBUG("nsx srt:%d \n",sizeof(NsxInst_t));
  #else
    NsxInst_t* self = NULL;

    self = (NsxInst_t*)nsx_heap;

  #endif
  *nsxInst = (NsxHandle*)self;

  if (self != NULL) {
    wl_WebRtcSpl_Init();
    self->real_fft = NULL;
    self->initFlag = 0;
    return 0;
  } else {
    return -1;
  }

}

int wl_WebRtcNsx_Free(NsxHandle* nsxInst) {
  wl_WebRtcSpl_FreeRealFFT(((NsxInst_t*)nsxInst)->real_fft);
  return 0;
}

int wl_WebRtcNsx_Init(NsxHandle* nsxInst, uint32_t fs) {
  return wl_WebRtcNsx_InitCore((NsxInst_t*)nsxInst, fs);
}

int wl_WebRtcNsx_set_policy(NsxHandle* nsxInst, int mode) {
  return wl_WebRtcNsx_set_policy_core((NsxInst_t*)nsxInst, mode);
}

int wl_WebRtcNsx_Process(NsxHandle* nsxInst, short* speechFrame,
                      short* speechFrameHB, short* outFrame,
                      short* outFrameHB) {
  return wl_WebRtcNsx_ProcessCore(
      (NsxInst_t*)nsxInst, speechFrame, speechFrameHB, outFrame, outFrameHB);
}
