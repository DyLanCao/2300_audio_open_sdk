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
#ifndef __AT_THREAD_USER_H__
#define __AT_THREAD_USER_H__

#ifdef __cplusplus
extern "C" {
#endif

enum THREAD_USER_ID
{
    THREAD_USER0,
    THREAD_USER1,
    THREAD_USER2,
    THREAD_USER3,
    THREAD_USER4,
    THREAD_USER_COUNT,
};

typedef struct {
	uint32_t id;
	uint32_t ptr;
	uint32_t param0;
	uint32_t param1;
} AT_USER_MESSAGE;

#define AT_USER_MESSAGE_ID_CMD    0

bool at_thread_user_is_inited(enum THREAD_USER_ID user_id);
int at_thread_user_init(enum THREAD_USER_ID user_id);
int at_thread_user_enqueue_cmd(enum THREAD_USER_ID user_id,
        uint32_t cmd_id,
        uint32_t param0,
        uint32_t param1,
        uint32_t pfunc
        );
#ifdef __cplusplus
	}
#endif

#endif // __AT_THREAD_USER_H__

