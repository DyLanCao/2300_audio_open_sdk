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
#ifndef _CODEC_SCALABLE_H
#define _CODEC_SCALABLE_H

#define BTIF_A2DP_SCALABLE_OCTET_NUMBER                (7)
#define BTIF_A2DP_SCALABLE_VENDOR_ID                   0x00000075
#define BTIF_A2DP_SCALABLE_CODEC_ID                    0x0103

//To indicate Sampling Rate.
#define BTIF_A2DP_SCALABLE_SR_96000                    0x80
#define BTIF_A2DP_SCALABLE_SR_32000                    0x40
#define BTIF_A2DP_SCALABLE_SR_44100                    0x20
#define BTIF_A2DP_SCALABLE_SR_48000                    0x10
#define BTIF_A2DP_SCALABLE_SR_DATA(X)                  (X & (BTIF_A2DP_SCALABLE_SR_96000 | BTIF_A2DP_SCALABLE_SR_32000 | BTIF_A2DP_SCALABLE_SR_44100 | BTIF_A2DP_SCALABLE_SR_48000))

//To indicate bits per sample.
#define BTIF_A2DP_SCALABLE_FMT_24                      0x08
#define BTIF_A2DP_SCALABLE_FMT_16                      0x00
#define BTIF_A2DP_SCALABLE_FMT_DATA(X)                 (X & (BTIF_A2DP_SCALABLE_FMT_24 | BTIF_A2DP_SCALABLE_FMT_16))

#endif