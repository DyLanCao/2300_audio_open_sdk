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
#ifndef __SPEECH_MEMORY_H__
#define __SPEECH_MEMORY_H__

#include "med_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define speech_heap_init(a, b)          med_heap_init(a, b)
#define speech_heap_add_block(a, b)     med_heap_add_block(a, b)
#define speech_malloc(a)                med_malloc(a)
#define speech_realloc(a, b)            med_realloc(a, b)
#define speech_calloc(a, b)             med_calloc(a, b)
#define speech_free(a)                  med_free(a)
#define speech_memory_info(a, b, c)     med_memory_info(a, b, c)

#ifdef __cplusplus
}
#endif

#endif