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
#ifndef __ANALOG_BEST2300P_H__
#define __ANALOG_BEST2300P_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ANA_MIC_CH_NUM                  5

uint32_t analog_aud_get_max_dre_gain(void);

int analog_debug_config_vad_mic(bool enable);

void analog_aud_codec_vad_open(void);
void analog_aud_codec_vad_close(void);
void analog_aud_codec_vad_enable(bool enable);

#ifdef __cplusplus
}
#endif

#endif

