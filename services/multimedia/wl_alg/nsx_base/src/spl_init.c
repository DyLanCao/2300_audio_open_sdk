/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/* The global function contained in this file initializes SPL function
 * pointers, currently only for ARM platforms.
 *
 * Some code came from common/rtcd.c in the WebM project.
 */

#include "real_fft.h"
#include "signal_processing_library.h"
//#include "cpu_features_wrapper.h"

/* Declare function pointers. */
MaxAbsValueW16 wl_WebRtcSpl_MaxAbsValueW16;
MaxAbsValueW32 wl_WebRtcSpl_MaxAbsValueW32;
MaxValueW16 wl_WebRtcSpl_MaxValueW16;
MaxValueW32 wl_WebRtcSpl_MaxValueW32;
MinValueW16 wl_WebRtcSpl_MinValueW16;
MinValueW32 wl_WebRtcSpl_MinValueW32;
CrossCorrelation wl_WebRtcSpl_CrossCorrelation;
DownsampleFast wl_WebRtcSpl_DownsampleFast;
ScaleAndAddVectorsWithRound wl_WebRtcSpl_ScaleAndAddVectorsWithRound;
CreateRealFFT wl_WebRtcSpl_CreateRealFFT;
FreeRealFFT wl_WebRtcSpl_FreeRealFFT;
RealForwardFFT wl_WebRtcSpl_RealForwardFFT;
RealInverseFFT wl_WebRtcSpl_RealInverseFFT;

#if (defined(WEBRTC_DETECT_ARM_NEON) || !defined(WEBRTC_ARCH_ARM_NEON)) && \
     !defined(MIPS32_LE)
/* Initialize function pointers to the generic C version. */
static void InitPointersToC() {

  wl_WebRtcSpl_MaxAbsValueW16 = wl_WebRtcSpl_MaxAbsValueW16C;
  wl_WebRtcSpl_MaxAbsValueW32 = wl_WebRtcSpl_MaxAbsValueW32C;
  wl_WebRtcSpl_MaxValueW16 = wl_WebRtcSpl_MaxValueW16C;
  wl_WebRtcSpl_MaxValueW32 = wl_WebRtcSpl_MaxValueW32C;
  wl_WebRtcSpl_MinValueW16 = wl_WebRtcSpl_MinValueW16C;
  wl_WebRtcSpl_MinValueW32 = wl_WebRtcSpl_MinValueW32C;
  wl_WebRtcSpl_CrossCorrelation = wl_WebRtcSpl_CrossCorrelationC;
  wl_WebRtcSpl_DownsampleFast = wl_WebRtcSpl_DownsampleFastC;
  wl_WebRtcSpl_ScaleAndAddVectorsWithRound =wl_WebRtcSpl_ScaleAndAddVectorsWithRoundC;
  wl_WebRtcSpl_CreateRealFFT = WebRtcSpl_CreateRealFFTC;
  wl_WebRtcSpl_FreeRealFFT = WebRtcSpl_FreeRealFFTC;
  wl_WebRtcSpl_RealForwardFFT = WebRtcSpl_RealForwardFFTC;
  wl_WebRtcSpl_RealInverseFFT = WebRtcSpl_RealInverseFFTC;
}
#endif

#if defined(WEBRTC_DETECT_ARM_NEON) || defined(WEBRTC_ARCH_ARM_NEON)
/* Initialize function pointers to the Neon version. */
static void InitPointersToNeon() {
  wl_WebRtcSpl_MaxAbsValueW16 = WebRtcSpl_MaxAbsValueW16Neon;
  wl_WebRtcSpl_MaxAbsValueW32 = WebRtcSpl_MaxAbsValueW32Neon;
  wl_WebRtcSpl_MaxValueW16 = WebRtcSpl_MaxValueW16Neon;
  wl_WebRtcSpl_MaxValueW32 = WebRtcSpl_MaxValueW32Neon;
  wl_WebRtcSpl_MinValueW16 = WebRtcSpl_MinValueW16Neon;
  wl_WebRtcSpl_MinValueW32 = WebRtcSpl_MinValueW32Neon;
  wl_WebRtcSpl_CrossCorrelation = WebRtcSpl_CrossCorrelationNeon;
  wl_WebRtcSpl_DownsampleFast = WebRtcSpl_DownsampleFastNeon;
  wl_WebRtcSpl_ScaleAndAddVectorsWithRound =
      WebRtcSpl_ScaleAndAddVectorsWithRoundNeon;
  wl_WebRtcSpl_CreateRealFFT = WebRtcSpl_CreateRealFFTNeon;
  wl_WebRtcSpl_FreeRealFFT = WebRtcSpl_FreeRealFFTNeon;
  wl_WebRtcSpl_RealForwardFFT = WebRtcSpl_RealForwardFFTNeon;
  wl_WebRtcSpl_RealInverseFFT = WebRtcSpl_RealInverseFFTNeon;
}
#endif

#if defined(MIPS32_LE)
/* Initialize function pointers to the MIPS version. */
static void InitPointersToMIPS() {
  wl_WebRtcSpl_MaxAbsValueW16 = WebRtcSpl_MaxAbsValueW16_mips;
  wl_WebRtcSpl_MaxValueW16 = WebRtcSpl_MaxValueW16_mips;
  wl_WebRtcSpl_MaxValueW32 = WebRtcSpl_MaxValueW32_mips;
  wl_WebRtcSpl_MinValueW16 = WebRtcSpl_MinValueW16_mips;
  wl_WebRtcSpl_MinValueW32 = WebRtcSpl_MinValueW32_mips;
  wl_WebRtcSpl_CrossCorrelation = WebRtcSpl_CrossCorrelation_mips;
  wl_WebRtcSpl_DownsampleFast = WebRtcSpl_DownsampleFast_mips;
  wl_WebRtcSpl_CreateRealFFT = WebRtcSpl_CreateRealFFTC;
  wl_WebRtcSpl_FreeRealFFT = WebRtcSpl_FreeRealFFTC;
  wl_WebRtcSpl_RealForwardFFT = WebRtcSpl_RealForwardFFTC;
  wl_WebRtcSpl_RealInverseFFT = WebRtcSpl_RealInverseFFTC;
#if defined(MIPS_DSP_R1_LE)
  wl_WebRtcSpl_MaxAbsValueW32 = WebRtcSpl_MaxAbsValueW32_mips;
  wl_WebRtcSpl_ScaleAndAddVectorsWithRound =
      WebRtcSpl_ScaleAndAddVectorsWithRound_mips;
#else
  wl_WebRtcSpl_MaxAbsValueW32 = wl_WebRtcSpl_MaxAbsValueW32C;
  wl_WebRtcSpl_ScaleAndAddVectorsWithRound = wl_WebRtcSpl_ScaleAndAddVectorsWithRoundC;
#endif
}
#endif

static void InitFunctionPointers(void) {
#if defined(WEBRTC_DETECT_ARM_NEON)
  if ((WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON) != 0) {
    InitPointersToNeon();
  } else {
    InitPointersToC();
  }
#elif defined(WEBRTC_ARCH_ARM_NEON)
  InitPointersToNeon();
#elif defined(MIPS32_LE)
  InitPointersToMIPS();
#else
  InitPointersToC();
#endif  /* WEBRTC_DETECT_ARM_NEON */
}

#if defined(WEBRTC_POSIX)
#include <pthread.h>

static void once(void (*func)(void)) {
  static pthread_once_t lock = PTHREAD_ONCE_INIT;
  pthread_once(&lock, func);
}

#elif defined(_WIN32)
#include <windows.h>

static void once(void (*func)(void)) {
  /* Didn't use InitializeCriticalSection() since there's no race-free context
   * in which to execute it.
   *
   * TODO(kma): Change to different implementation (e.g.
   * InterlockedCompareExchangePointer) to avoid issues similar to
   * http://code.google.com/p/webm/issues/detail?id=467.
   */
  static CRITICAL_SECTION lock = {(void *)((size_t)-1), -1, 0, 0, 0, 0};
  static int done = 0;

  EnterCriticalSection(&lock);
  if (!done) {
    func();
    done = 1;
  }
  LeaveCriticalSection(&lock);
}

/* There's no fallback version as an #else block here to ensure thread safety.
 * In case of neither pthread for WEBRTC_POSIX nor _WIN32 is present, build
 * system should pick it up.
 */
#endif  /* WEBRTC_POSIX */

void wl_WebRtcSpl_Init() {
  //once(InitFunctionPointers);
  InitFunctionPointers();
}
