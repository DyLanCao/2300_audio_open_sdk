/*
 *  Copyright (c)  All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*
 * This header file includes the VAD API calls. Specific function calls are given below.
 */

#ifndef USER_VAD_H_  // NOLINT
#define USER_VAD_H_

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

#define VAD_MODE 3

void wl_vad_init(int mode);

int wl_vad_process_frame(short *buffer, int len);

#ifdef __cplusplus
}
#endif

#endif  // 