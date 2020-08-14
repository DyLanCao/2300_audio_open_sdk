/*
 *  Copyright 2006 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_BASE_CHECKS_H_
#define WEBRTC_BASE_CHECKS_H_

#include <stdio.h>
#include "typedefs.h"

// If you for some reson need to know if DCHECKs are on, test the value of
// RTC_DCHECK_IS_ON. (Test its value, not if it's defined; it'll always be
// defined, to either a true or a false value.)
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
#define RTC_DCHECK_IS_ON 1
#else
#define RTC_DCHECK_IS_ON 0
#endif

// C version. Lacks many features compared to the C++ version, but usage
// guidelines are the same.

#define RTC_CHECK(condition)                                                         \
  do {                                                                               \
    if (!(condition)) {                                                              \
      fprintf(stderr, "%s:%d CHECK failed: %s\n", __FILE__, __LINE__, #condition);	 \
    }                                                                                \
  } while (0)

#define RTC_CHECK_EQ(a, b) RTC_CHECK((a) == (b))
#define RTC_CHECK_NE(a, b) RTC_CHECK((a) != (b))
#define RTC_CHECK_LE(a, b) RTC_CHECK((a) <= (b))
#define RTC_CHECK_LT(a, b) RTC_CHECK((a) < (b))
#define RTC_CHECK_GE(a, b) RTC_CHECK((a) >= (b))
#define RTC_CHECK_GT(a, b) RTC_CHECK((a) > (b))

#define RTC_DCHECK(condition)                                                          \
  do {                                                                                 \
    if (RTC_DCHECK_IS_ON && !(condition)) {                                            \
      fprintf(stderr, "%s:%d DCHECK failed: %s\n", __FILE__, __LINE__, #condition);  \
    }                                                                                  \
  } while (0)

#define RTC_DCHECK_EQ(a, b) RTC_DCHECK((a) == (b))
#define RTC_DCHECK_NE(a, b) RTC_DCHECK((a) != (b))
#define RTC_DCHECK_LE(a, b) RTC_DCHECK((a) <= (b))
#define RTC_DCHECK_LT(a, b) RTC_DCHECK((a) < (b))
#define RTC_DCHECK_GE(a, b) RTC_DCHECK((a) >= (b))
#define RTC_DCHECK_GT(a, b) RTC_DCHECK((a) > (b))

#endif  // WEBRTC_BASE_CHECKS_H_
