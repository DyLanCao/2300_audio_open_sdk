/*
 *  Copyright (c) 2020 The wl project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GCC_PLAT__
#define __GCC_PLAT__

#include "plat_types.h"

void gcc_plat_init(void);

void gcc_plat_process(short *left_input,short *right_input, uint32_t frame_len);

void gcc_plat_exit(void);

#endif
#ifdef __cplusplus
}
#endif

