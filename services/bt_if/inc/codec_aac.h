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
#ifndef _CODEC_AAC_H
#define _CODEC_AAC_H

#define BTIF_A2DP_AAC_OCTET0_MPEG2_AAC_LC              0x80
#define BTIF_A2DP_AAC_OCTET0_MPEG4_AAC_LC              0x40
#define BTIF_A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100  0x01
#define BTIF_A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000  0x80
#define BTIF_A2DP_AAC_OCTET2_CHANNELS_1                0x08
#define BTIF_A2DP_AAC_OCTET2_CHANNELS_2                0x04
#define BTIF_A2DP_AAC_OCTET3_VBR_SUPPORTED             0x80

#define BTIF_A2DP_AAC_OCTET_NUMBER                     (6)

#endif
