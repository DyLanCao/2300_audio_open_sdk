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

#ifndef __TWS_ROLE_SWITCH__H__
#define __TWS_ROLE_SWITCH__H__

#include "bluetooth.h"

enum TWS_DATA_STRUCTURE
{
    BT_ME = 0,
    BT_HCI,
    BT_L2CAP,
    BT_RFC,
    HF_CONTEXT,
    CMGR_CONTEXT,
    AVRCP_CONTEXT,
    DEV_TABLE,
    APP_BT_DEVICE,
    AVDEV_CONTEXT,
    SLAVE_SAVE_DATA_OK,
};

enum PSM_CONTEXT_TYPE
{
     PSM_CONTEXT_HF_RFC     = 0x01, //HFP
     PSM_CONTEXT_AVDTP   = 0x02,  //A2DP
     PSM_CONTEXT_AVCTP   = 0x04,  //AVRCP
     
     PSM_CONTEXT_INVALID = 0x80,
};

enum PROFILE_CONTEXT_FLAG
{
    BT_COMMON_FLAG = 0x01,
    BT_HFP_FLAG = 0x02,
    BT_A2DP_FLAG = 0x04,
    BT_A2DP_CONTINUE_FLAG = 0x08,
    BT_AVRCP_FLAG = 0x10,
    
    DATA_COMPLETE_FLAG = 0x80,
};


enum TWS_CHANNEL_TYPE
{
     HF_RF_CHANNEL = 0,
     UNKNOWN_CHANNEL,
};

#ifdef __cplusplus
extern "C" {
#endif                          /*  */


#ifdef __cplusplus
}
#endif                          /*  */
#endif                          /* __ME_H */
