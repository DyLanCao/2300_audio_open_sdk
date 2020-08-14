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
#ifndef __SPEECH_SSAT_H__
#define __SPEECH_SSAT_H__


#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && defined(__arm__)
#include "cmsis.h"
static inline short speech_ssat_int16(int in)
{
    short out;

    out = __SSAT(in,16);

    return out;
}
#else
static inline short speech_ssat_int16(int in)
{
    short out;

    if (in>32767)
    {
        in = 32767;
    }
    else if (in<-32768)
    {
        in = -32768;
    }
    out = (int)in;
    return out;
}
#endif

#ifdef __cplusplus
}
#endif

#endif