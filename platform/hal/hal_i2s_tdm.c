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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hal_i2s.h"
#include "hal_trace.h"
#include "hal_dma.h"
#include "hal_i2s_tdm.h"

static struct HAL_DMA_DESC_T i2s_rx_dma_desc[I2S_TDM_FRAME_NUM_MAX];
static struct HAL_DMA_DESC_T i2s_tx_dma_desc[I2S_TDM_FRAME_NUM_MAX];
static uint16_t I2S_TDM_BUF_ALIGN tdm_i2s_tx_buf[I2S_TDM_TX_FRAME_NUM][I2S_TDM_TX_FRAME_SIZE];
static uint32_t tx_frame_num = I2S_TDM_TX_FRAME_NUM;

static void i2s_tx_handler(uint8_t chan, uint32_t remains, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    /*
    static int cnt = 0;

    cnt++;
    if (cnt % 600 == 0) {
        TRACE("I2S-TX: remains=%ld, error=%ld, cnt=%d,tx_buff_len = %d.", remains, error, cnt,sizeof(tdm_i2s_tx_buf));
        DUMP8("0x%x,",tdm_i2s_tx_buf,sizeof(tdm_i2s_tx_buf) <= 32 ? sizeof(tdm_i2s_tx_buf) : 32 );
    }
    */
}

int32_t hal_i2s_tdm_open(enum HAL_I2S_ID_T i2s_id,enum AUD_SAMPRATE_T sample_rate,HAL_DMA_IRQ_HANDLER_T rx_handler,uint32_t rx_frame_num,uint8_t *rx_buff,uint32_t rx_buff_len)
{
    int i,j;
    struct HAL_DMA_CH_CFG_T dma_cfg;
    struct HAL_I2S_CONFIG_T i2s_cfg;
    int ret;
    uint16_t *buf;
    const uint32_t bitwides = 0x07;

    TRACE("hal_i2s_tdm_open:i2s_id = %d,frame_num = %d,rx_buff_len = %d.",
        i2s_id,rx_frame_num,rx_buff_len);

    // i2s open playback and capture.
    ret = hal_i2s_open(i2s_id, AUD_STREAM_PLAYBACK, HAL_I2S_MODE_MASTER);
    if(ret)
    {
        TRACE("hal_i2s_open playback failed.ret = %d.", ret);
        goto __func_fail;
    }
    ret = hal_i2s_open(i2s_id, AUD_STREAM_CAPTURE, HAL_I2S_MODE_MASTER);
    if(ret)
    {
        TRACE("hal_i2s_open capture failed.ret = %d.", ret);
        goto __func_fail;
    }

    // i2s setup stream playback and capture.
    memset(&i2s_cfg, 0, sizeof(i2s_cfg));
    i2s_cfg.use_dma = true;
    i2s_cfg.master_clk_wait = true;
    i2s_cfg.chan_sep_buf = false;
    i2s_cfg.bits = 16;
    i2s_cfg.channel_num = 2;
    i2s_cfg.channel_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
    i2s_cfg.sample_rate = sample_rate;
    ret = hal_i2s_setup_stream(i2s_id, AUD_STREAM_PLAYBACK, &i2s_cfg);
    if(ret)
    {
        TRACE("hal_i2s_setup_stream playback failed.ret = %d.", ret);
        goto __func_fail;
    }
    i2s_cfg.master_clk_wait = false;
    ret = hal_i2s_setup_stream(i2s_id, AUD_STREAM_CAPTURE, &i2s_cfg);
    if(ret)
    {
        TRACE("hal_i2s_setup_stream capture failed.ret = %d.", ret);
        goto __func_fail;
    }

    // tx dma init/start.
    for (i = 0; i < tx_frame_num ; i++) {
        buf = (uint16_t*)(&tdm_i2s_tx_buf[i][0]);
        for(j = 0; j < I2S_TDM_TX_FRAME_SIZE; j++) {
            if ((j & bitwides) == 0) {
                buf[j] = 0x8000;
            } else {
                buf[j] = 0x0;
            }
        }
    }
    memset(&dma_cfg, 0, sizeof(dma_cfg));
    dma_cfg.dst = 0; //useless
    dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
    dma_cfg.dst_periph = HAL_AUDMA_I2S0_TX;
    dma_cfg.dst_width = HAL_DMA_WIDTH_HALFWORD;
    dma_cfg.handler = i2s_tx_handler;
    dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
    dma_cfg.src_tsize = (sizeof(tdm_i2s_tx_buf[0]) /tx_frame_num);
    dma_cfg.src_width = HAL_DMA_WIDTH_HALFWORD;
    dma_cfg.try_burst = 1;
    dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;
    dma_cfg.ch = hal_audma_get_chan(dma_cfg.dst_periph, HAL_DMA_HIGH_PRIO);

    for (i = 0; i < tx_frame_num; i++) {
        dma_cfg.src = (uint32_t)&tdm_i2s_tx_buf[i][0];
        ret = hal_audma_init_desc(&i2s_tx_dma_desc[i],
                &dma_cfg,
                &i2s_tx_dma_desc[(i + 1) % tx_frame_num],
                1);
        if(ret)
        {
            TRACE("hal_audma_init_desc tx failed.ret = %d.", ret);
            goto __func_fail;
        }
    }

    ret = hal_audma_sg_start(&i2s_tx_dma_desc[0], &dma_cfg);
    if(ret)
    {
        TRACE("hal_audma_sg_start tx failed.ret = %d.", ret);
        goto __func_fail;
    }
    // rx dma init/start.
    memset(&dma_cfg, 0, sizeof(dma_cfg));
    dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
    dma_cfg.dst_width = HAL_DMA_WIDTH_HALFWORD;
    dma_cfg.handler = rx_handler;
    dma_cfg.src = 0; // useless
    dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
    dma_cfg.src_periph = HAL_AUDMA_I2S0_RX;
    dma_cfg.src_tsize = rx_buff_len / rx_frame_num;
    dma_cfg.src_width = HAL_DMA_WIDTH_HALFWORD;
    dma_cfg.try_burst = 1;
    dma_cfg.type = HAL_DMA_FLOW_P2M_DMA;
    dma_cfg.ch = hal_audma_get_chan(dma_cfg.src_periph, HAL_DMA_HIGH_PRIO);

    for (i = 0; i < rx_frame_num; i++) {
        dma_cfg.dst = (uint32_t)&rx_buff[i*(rx_buff_len/2)];
        TRACE("_debug: dst[%d] = 0x%x.",i,dma_cfg.dst);
        ret = hal_audma_init_desc(&i2s_rx_dma_desc[i],
                   &dma_cfg,
                   &i2s_rx_dma_desc[(i + 1) % rx_frame_num],
                   (i + 1) % (rx_frame_num / 2) == 0);
        if(ret)
        {
            TRACE("hal_audma_init_desc tx failed.ret = %d.", ret);
            goto __func_fail;
        }
    }

    ret = hal_audma_sg_start(&i2s_rx_dma_desc[0], &dma_cfg);
    if(ret)
    {
        TRACE("hal_audma_sg_start tx failed.ret = %d.", ret);
        goto __func_fail;
    }

    // i2s start stream playback and capture.
    ret = hal_i2s_start_stream(i2s_id, AUD_STREAM_PLAYBACK);
    if(ret)
    {
        TRACE("hal_i2s_start_stream tx failed.ret = %d.", ret);
        goto __func_fail;
    }

    ret = hal_i2s_start_stream(i2s_id, AUD_STREAM_CAPTURE);
    if(ret)
    {
        TRACE("hal_i2s_start_stream tx failed.ret = %d.", ret);
        goto __func_fail;
    }
    TRACE("hal_i2s_tdm_open done.");
    return 0;

__func_fail:
    return ret;
}

int32_t hal_i2s_tdm_close(enum HAL_I2S_ID_T i2s_id)
{
    return 0;
}


