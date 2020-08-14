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
#ifndef __REG_CODEC_BEST3001_H__
#define __REG_CODEC_BEST3001_H__

#include "plat_types.h"

struct CODEC_T {
    __IO uint32_t CONFIG;			// 0x0000
    __IO uint32_t FIFO_FLUSH;		// 0x0004
    __IO uint32_t FIFO_THRESHOLD;	// 0x0008
    __IO uint32_t ISR;				// 0x000C
    __IO uint32_t IMR;				// 0x0010
    __IO uint32_t FIFO_RX_COUNT;	// 0x0014
    __IO uint32_t FIFO_TX_COUNT;	// 0x0018
    __IO uint32_t ADC_FIFO;		// 0x001C, (R) Read from codec rx fifo
    __IO uint32_t ADC0_FIFO;		// 0x0020, (R) Read from adc fifo ch0
    __IO uint32_t ADC1_FIFO;		// 0x0024, (R) Read from adc fifo ch1
    __IO uint32_t DSD_RX_FIFO;		// 0x0028, (R) Read from dsd rx fifo; write this to push dsd tx fifo
    __IO uint32_t ADC_MODE;		// 0x002C
    __IO uint32_t DAC_MODE;		// 0x0030
    __IO uint32_t DSD_CFG0;          	// 0x0034
    __IO uint32_t CNT_KEEP;         	// 0x0038, how to use?
    __IO uint32_t SYNC_CFG;        	// 0x003C
    __IO uint32_t REG32B_040;			// CLK_CFG;          	// 0x0040
    __IO uint32_t REG32B_044;        	// 0x0044, how to use?
    __IO uint32_t SOFT_RSTN;			// SOFT_RSTN;           // 0x0048
        uint32_t RESERVED_004C[0x34/4];     // 0x004C
    __IO uint32_t ADC_CFG;                // 0x0080
    __IO uint32_t ADC0_CFG;		// 0x0084
    __IO uint32_t ADC1_CFG;		// 0x0088
    __IO uint32_t DAC_CFG;			// 0x008C
    __IO uint32_t DAC0_GAIN_CFG;	// 0x0090
    __IO uint32_t DAC1_GAIN_CFG;	// 0x0094
    __IO uint32_t DAC_GAIN_CFG;	// 0x0098
    __IO uint32_t DAC_DC_CFG;		// 0x009C
    __IO uint32_t PDM_CFG;			// 0x00A0
    __IO uint32_t RAMP0_CFG;		// 0x00A4
    __IO uint32_t RAMP1_CFG;		// 0x00A8
    __IO uint32_t ADC0_DC_DOUT;	// 0x00AC, (R)
    __IO uint32_t ADC1_DC_DOUT;	// 0x00B0, (R)
    __IO uint32_t ADC0_DC_CFG;		// 0x00B4
    __IO uint32_t ADC1_DC_CFG;		// 0x00B8
    __IO uint32_t DSD_CFG1;		// 0x00BC
    __IO uint32_t ADC0_DRE_CFG0;	// 0x00C0
    __IO uint32_t ADC0_DRE_CFG1;	// 0x00C4
    __IO uint32_t ADC1_DRE_CFG0;	// 0x00C8
    __IO uint32_t ADC1_DRE_CFG1;	// 0x00CC
    __IO uint32_t DAC0_DRE_CFG0;    // 0x00D0
    __IO uint32_t DAC0_DRE_CFG1;    // 0x00D4
    __IO uint32_t DAC1_DRE_CFG0;    // 0x00D8
    __IO uint32_t DAC1_DRE_CFG1;    // 0x00DC
    __IO uint32_t IIR_CFG;                // 0x00E0
    __IO uint32_t RSPL_CFG;                // 0x00E4, resample config
    __IO uint32_t RSPL_DAC_PHASE_INC;                // 0x00E8
    __IO uint32_t RSPL_ADC_PHASE_INC;                // 0x00EC
    __IO uint32_t FIR_CFG;                // 0x00F0
    __IO uint32_t FIR_ACCESS;                // 0x00F4
    __IO uint32_t FIR_CH0_CFG0;                // 0x00F8
    __IO uint32_t FIR_CH0_CFG1;                // 0x00FC
    __IO uint32_t FIR_CH1_CFG0;                // 0x0100
    __IO uint32_t FIR_CH1_CFG1;                // 0x0104
    __IO uint32_t FIR_CH2_CFG0;                // 0x0108
    __IO uint32_t FIR_CH2_CFG1;                // 0x010C
    __IO uint32_t FIR_CH3_CFG0;                // 0x0110
    __IO uint32_t FIR_CH3_CFG1;                // 0x0114
    __IO uint32_t CLASSG_CFG;                // 0x0118
    __IO uint32_t CLASSG_THD;                // 0x011C
    __IO uint32_t ADC0_DRE_GAIN_STEP_01;                // 0x0120
    __IO uint32_t ADC0_DRE_GAIN_STEP_23;                // 0x0124
    __IO uint32_t ADC0_DRE_GAIN_STEP_45;                // 0x0128
    __IO uint32_t ADC0_DRE_GAIN_STEP_67;                // 0x012C
    __IO uint32_t ADC0_DRE_GAIN_STEP_89;                // 0x0130
    __IO uint32_t ADC0_DRE_GAIN_STEP_AB;                // 0x0134
    __IO uint32_t ADC0_DRE_GAIN_STEP_CD;                // 0x0138
    __IO uint32_t ADC0_DRE_GAIN_STEP_EF;                // 0x013C
    __IO uint32_t ADC1_DRE_GAIN_STEP_01;                // 0x0140
    __IO uint32_t ADC1_DRE_GAIN_STEP_23;                // 0x0144
    __IO uint32_t ADC1_DRE_GAIN_STEP_45;                // 0x0148
    __IO uint32_t ADC1_DRE_GAIN_STEP_67;                // 0x014C
    __IO uint32_t ADC1_DRE_GAIN_STEP_89;                // 0x0150
    __IO uint32_t ADC1_DRE_GAIN_STEP_AB;                // 0x0154
    __IO uint32_t ADC1_DRE_GAIN_STEP_CD;                // 0x0158
    __IO uint32_t ADC1_DRE_GAIN_STEP_EF;                // 0x015C
    __IO uint32_t DAC_DRE_GAIN_STEP_01;                // 0x0160
    __IO uint32_t DAC_DRE_GAIN_STEP_23;                // 0x0164
    __IO uint32_t DAC_DRE_GAIN_STEP_45;                // 0x0168
    __IO uint32_t DAC_DRE_GAIN_STEP_67;                // 0x016C
    __IO uint32_t DAC_DRE_GAIN_STEP_89;                // 0x0170
    __IO uint32_t DAC_DRE_GAIN_STEP_AB;                // 0x0174
    __IO uint32_t DAC_DRE_GAIN_STEP_CD;                // 0x0178
    __IO uint32_t DAC_DRE_GAIN_STEP_EF;                // 0x017C
};

// REG0000: (RW) CONFIG
#define CODEC_CFG_DMACTRL_TX	(1 << 6)	// dma control of tx;
#define CODEC_CFG_DMACTRL_RX	(1 << 5)	// dma control of rx;
#define CODEC_CFG_DAC_ENABLE	(1 << 4)	// dac enable of dma part;
#define CODEC_CFG_ADC_ENABLE_CH1	(1 << 3)	// adc ch1 enable
#define CODEC_CFG_ADC_ENABLE_CH0	(1 << 2)	// adc ch0 enable
#define CODEC_CFG_ADC_ENABLE		(1 << 1)   // adc enable of dma part; when any of adc0~1 valid, this should be valid
#define CODEC_CFG_CODEC_IF_EN	(1 << 0)    // code enable of dma part; must be valid when adc/dac <-> dma

// REG0004: (RW) FIFO_FLUSH, write 1 will keep flush; write 0 end flush;
#define CODEC_FIFO_FLUSH_DSD_TX_FIFO_FLUSH  (1 << 4)
#define CODEC_FIFO_FLUSH_DSD_RX_FIFO_FLUSH  (1 << 3)
#define CODEC_FIFO_FLUSH_TX_FIFO_FLUSH          (1 << 2)
#define CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH1  (1 << 1)
#define CODEC_FIFO_FLUSH_RX_FIFO_FLUSH_CH0  (1 << 0)	// fifo flush; 

// REG0008: (RW) FIFO_THRESHOLD
#define CODEC_FIFO_THRESH_TX_DSD_SHIFT	12
#define CODEC_FIFO_THRESH_TX_DSD_MASK	(0x7 << CODEC_FIFO_THRESH_TX_DSD_SHIFT)
#define CODEC_FIFO_THRESH_TX_DSD(n)	BITFIELD_VAL(CODEC_FIFO_THRESH_TX_DSD, n)
#define CODEC_FIFO_THRESH_RX_DSD_SHIFT	8
#define CODEC_FIFO_THRESH_RX_DSD_MASK	(0xF << CODEC_FIFO_THRESH_RX_DSD_SHIFT)
#define CODEC_FIFO_THRESH_RX_DSD(n)	BITFIELD_VAL(CODEC_FIFO_THRESH_RX_DSD, n)
#define CODEC_FIFO_THRESH_TX_SHIFT		4
#define CODEC_FIFO_THRESH_TX_MASK		(0xF << CODEC_FIFO_THRESH_TX_SHIFT)
#define CODEC_FIFO_THRESH_TX(n)             BITFIELD_VAL(CODEC_FIFO_THRESH_TX, n)
#define CODEC_FIFO_THRESH_RX_SHIFT		0		
#define CODEC_FIFO_THRESH_RX_MASK		(0xF << CODEC_FIFO_THRESH_RX_SHIFT)
#define CODEC_FIFO_THRESH_RX(n)             BITFIELD_VAL(CODEC_FIFO_THRESH_RX, n)

// REG000C: (RW) ISR Status, when read, interrupt raw status; when write 1, clear corresponding interrupt status 
#define CODEC_ISR_ADC_MAX_OVERFLOW	(1 << 11)
#define CODEC_ISR_EVENT_TRIGGER		(1 << 10)
#define CODEC_ISR_DSD_TX_UNDERFLOW	(1 << 9)
#define CODEC_ISR_DSD_TX_OVERFLOW	(1 << 8)
#define CODEC_ISR_DSD_RX_UNDERFLOW	(1 << 7)
#define CODEC_ISR_DSD_RX_OVERFLOW	(1 << 6)
#define CODEC_ISR_TX_UNDERFLOW		(1 << 5)
#define CODEC_ISR_TX_OVERFLOW			(1 << 4)
#define CODEC_ISR_RX_UNDERFLOW_SHIFT		2
#define CODEC_ISR_RX_UNDERFLOW_MASK		(0x3 << CODEC_ISR_RX_UNDERFLOW_SHIFT)
#define CODEC_ISR_RX_UNDERFLOW(n)		BITFIELD_VAL(CODEC_ISR_RX_UNDERFLOW, n)
#define CODEC_ISR_RX_OVERFLOW_SHIFT		2
#define CODEC_ISR_RX_OVERFLOW_MASK 		(0x3 << CODEC_ISR_RX_OVERFLOW_SHIFT)
#define CODEC_ISR_RX_OVERFLOW(n)		BITFIELD_VAL(CODEC_ISR_RX_OVERFLOW, n)

// REG0010: (RW) interrupt mask; 0 will mask interrupt
#define CODEC_IMR_ADC_MAX_OVERFLOW_MASK	(1 << 11)
#define CODEC_IMR_EVENT_TRIGGER_MASK			(1 << 10)
#define CODEC_IMR_DSD_TX_UNDERFLOW_MASK	(1 << 9)
#define CODEC_IMR_DSD_TX_OVERFLOW_MASK		(1 << 8)
#define CODEC_IMR_DSD_RX_UNDERFLOW_MASK	(1 << 7)
#define CODEC_IMR_DSD_RX_OVERFLOW_MASK		(1 << 6)
#define CODEC_IMR_TX_UNDERFLOW_MASK			(1 << 5)
#define CODEC_IMR_TX_OVERFLOW_MASK			(1 << 4)
#define CODEC_IMR_RX_UNDERFLOW_MASK_SHIFT		2
#define CODEC_IMR_RX_UNDERFLOW_MASK_MASK		(0x3 << CODEC_IMR_RX_UNDERFLOW_MASK_SHIFT)
#define CODEC_IMR_RX_UNDERFLOW_MASK(n)		BITFIELD_VAL(CODEC_IMR_RX_UNDERFLOW_MASK, n)
#define CODEC_IMR_RX_OVERFLOW_MASK_SHIFT		0
#define CODEC_IMR_RX_OVERFLOW_MASK_MASK		(0x3 << CODEC_IMR_RX_OVERFLOW_MASK_SHIFT)
#define CODEC_IMR_RX_OVERFLOW_MASK(n)		BITFIELD_VAL(CODEC_IMR_RX_OVERFLOW_MASK, n)

// REG0014: (R) FIFO_RX_COUNT
#define CODEC_FIFO_DSD_RX_COUNT_SHIFT	(8)		// dsd rx fifo count, in 4 bits
#define CODEC_FIFO_DSD_RX_COUNT_MASK	(0xF << CODEC_FIFO_DSD_RX_COUNT_SHIFT)
#define CODEC_FIFO_RX_COUNT_CH1_SHIFT	(4)		// adc ch1 fifo count
#define CODEC_FIFO_RX_COUNT_CH1_MASK	(0xF << CODEC_FIFO_RX_COUNT_CH1_SHIFT)
#define CODEC_FIFO_RX_COUNT_CH0_SHIFT	(0)		// adc ch0 fifo count
#define CODEC_FIFO_RX_COUNT_CH0_MASK	(0xF << CODEC_FIFO_RX_COUNT_CH0_SHIFT)

// REG0018: (R) FIFO_TX_COUNT
#define CODEC_FIFO_DSD_TX_COUNT_SHIFT	(7)		// dsd tx fifo count, in 3 bits
#define CODEC_FIFO_DSD_TX_COUNT_MASK	(0x7 << CODEC_FIFO_DSD_TX_COUNT_SHIFT)
#define CODEC_STATE_RX_CH_SHIFT			(4)		// state of adc rx. (one ch in adc ch0~ch1 is being received)
#define CODEC_STATE_RX_CH_MASK			(0x7 << CODEC_STATE_RX_CH_SHIFT)
#define CODEC_FIFO_TX_COUNT_SHIFT			(0)		// dac fifo count
#define CODEC_FIFO_TX_COUNT_MASK			(0xF << CODEC_FIFO_TX_COUNT_SHIFT)

// REG002C: (RW) ADC_MODE
#define CODEC_ADC_MODE_32BITS			(1<<3)	// adc rx 32 bit mode; [31:12] is valid rx data; [11:0] is 0
#define CODEC_ADC_MODE_24BITS			(1<<2)	// adc rx 24 bit mode; dma configed as word; [23:4] is valid rx data; [31:24] is sign extended; [3:0] is 0
#define CODEC_ADC_MODE_16BITS_CH1	(1<<1)	// adc ch1 rx 16bit mode;  dma should be configed as halfword; rx sample is short format. 
#define CODEC_ADC_MODE_16BITS_CH0	(1<<0)	// adc ch0 rx 16bit mode;  dma should be configed as halfword; rx sample is short format. 

// REG0030: (RW) DAC MODE
#define CODEC_DAC_MODE_USB_24BITS	(1<<5)	// dac tx 24bit mode for usb; data transfered from usb in 32bit, and reorganized in 24bit format, [23:4] to dac
#define CODEC_DAC_MODE_32BITS			(1<<4)	// dac tx 32bit mode; [31:8] is valid tx data.  (default, dac is 20bit, [19:0] is valid, [31:20] donot care)
#define CODEC_DAC_MODE_24BITS			(1<<3)	// dac tx 24bit mode; dma configed as word;  [23:4] is valid tx data; (default, dac is 20bit, [19:0] is valid, [23:20] donot care)
#define CODEC_DAC_MODE_16BITS			(1<<2)	// dac tx 16bit mode; dma configed as halfword;  [15:0] is valid
#define CODEC_DAC_EXCHANGE_LR			(1<<1)	// If 1, exchange L/R
#define CODEC_DAC_DUAL_CHANNEL		(1<<0)	// dac tx is L/R two channel;  if 0, single channel, dma only transfer L; for dac, L=R

// REG0034: (RW) DSD_CTRL0
#define CODEC_DSD_INPUT_16BIT			(1<<9)	// dsd input data format; if 1, [R15~0] [L15~0] as 32bit; else [R15~8] [L15~8] [R7~0] [L7~0] as 32bit
#define CODEC_DSD_OUT_MODE_16BIT		(1<<8)
#define CODEC_DSD_DMA_CTRL_TX			(1<<7)
#define CODEC_DSD_DMA_CTRL_RX			(1<<6)
#define CODEC_DSD_OUT_MODE_32BIT		(1<<5)	// [31:12] is valid data; [11:0] is 0
#define CODEC_DSD_OUT_MODE_24BIT		(1<<4)	// dsd decoded data format;  [23:4] is valid data; [31:24] is sign extended.
#define CODEC_DSD_MSB_FIRST			(1<<3)	// dsd input data msb first control. If 1, for 8bit mode, [R8~15] [L8~15] [R0~7] [L0~7]
#define CODEC_DSD_DUAL_CHANNEL		(1<<2)	// L/R two channel
#define CODEC_DSD_ENABLE				(1<<1)	// dsd module enable;
#define CODEC_DSD_IF_EN				(1<<0)	// dsd dma part enable; 

// REG003C: (RW) SYNC_PLAY
#define CODEC_SYNC_FAULT_MUTE_EN				(1<<17)
#define CODEC_SYNC_PLL_OSC_TRIGGER_SEL_SHIFT	15	//  0: no trigger;  1: gpio trigger;  2: bt trigger
#define CODEC_SYNC_PLL_OSC_TRIGGER_SEL_MASK		(0x3 << CODEC_SYNC_PLL_OSC_TRIGGER_SEL_SHIFT)
#define CODEC_SYNC_PLL_OSC_TRIGGER_SEL(n)		BITFIELD_VAL(CODEC_SYNC_TEST_PORT_SEL, n)
#define CODEC_SYNC_TEST_PORT_SEL_SHIFT			12		// GPIO number
#define CODEC_SYNC_TEST_PORT_SEL_MASK			(0x7 << CODEC_SYNC_TEST_PORT_SEL_SHIFT)
#define CODEC_SYNC_TEST_PORT_SEL(n)			BITFIELD_VAL(CODEC_SYNC_TEST_PORT_SEL, n)
#define CODEC_SYNC_EVENT_FOR_CAP				(1<<11)	// 1: enable cnt keep
#define CODEC_SYNC_EVENT_SEL					(1<<10)	// 0: select bt event for cnt keep;  1: select gpio event for cnt keep;
#define CODEC_SYNC_STAMP_CLR_USED                   (1<<9)
#define CODEC_SYNC_GPIO_TRIGGER_DB_EN		(1<<8)	// gpio trigger debounce enable;
#define CODEC_SYNC_ADC_ENABLE_SEL_SHIFT			6	// 0: no trigger;  1: gpio trigger;  2: bt trigger;  this for codec adc enable;
#define CODEC_SYNC_ADC_ENABLE_SEL_MASK			(0x3 << CODEC_SYNC_ADC_ENABLE_SEL_SHIFT)
#define CODEC_SYNC_ADC_ENABLE_SEL(n)			BITFIELD_VAL(CODEC_SYNC_ADC_DMA_ENABLE_SEL, n)
#define CODEC_SYNC_DAC_ENABLE_SEL_SHIFT			4	// 0: no trigger;  1: gpio trigger;  2: bt trigger;  this for codec dac enable;
#define CODEC_SYNC_DAC_ENABLE_SEL_MASK			(0x3 << CODEC_SYNC_DAC_ENABLE_SEL_SHIFT)
#define CODEC_SYNC_DAC_ENABLE_SEL(n)			BITFIELD_VAL(CODEC_SYNC_DAC_DMA_ENABLE_SEL, n)
#define CODEC_SYNC_ADC_DMA_ENABLE_SEL_SHIFT	2	// 0: no trigger;  1: gpio trigger;  2: bt trigger;  this for adc dma interface enable;
#define CODEC_SYNC_ADC_DMA_ENABLE_SEL_MASK		(0x3 << CODEC_SYNC_ADC_ENABLE_SEL_SHIFT)
#define CODEC_SYNC_ADC_DMA_ENABLE_SEL(n)		BITFIELD_VAL(CODEC_SYNC_ADC_DMA_ENABLE_SEL, n)
#define CODEC_SYNC_DAC_DMA_ENABLE_SEL_SHIFT	0	// 0: no trigger;  1: gpio trigger;  2: bt trigger;  this for dac dma interface enable;
#define CODEC_SYNC_DAC_DMA_ENABLE_SEL_MASK		(0x3 << CODEC_SYNC_DAC_ENABLE_SEL_SHIFT)
#define CODEC_SYNC_DAC_DMA_ENABLE_SEL(n)		BITFIELD_VAL(CODEC_SYNC_DAC_DMA_ENABLE_SEL, n)

// REG0040: (RW) CLK_CTRL
#define CODEC_CLK_CLK_OUT_CFG					(1 << 9)		// debug clock out
#define CODEC_CLK_DAC_OUT_POL					(1 << 8)		// inv of clk_dac_out
#define CODEC_CLK_ADC_ANA_POL_SHIFT		6	// inv of clk_adc_ana
#define CODEC_CLK_ADC_ANA_POL_MASK			(0x3 <<CODEC_CLK_ADC_ANA_POL_SHIFT)
#define CODEC_CLK_ADC_ANA_POL(n)			BITFIELD_VAL(CODEC_CLK_ADC_ANA_POL, n)
#define CODEC_CLK_DAC_EN								(1 << 5)		// gate enable of clk_dac
#define CODEC_CLK_ADC_EN_SHIFT					2		// gate enable of clk_adc
#define CODEC_CLK_ADC_EN_MASK						(0x7 << CODEC_CLK_ADC_EN_SHIFT)
#define CODEC_CLK_ADC_EN(n)						BITFIELD_VAL(CODEC_CLK_ADC_EN, n)
#define CODEC_CLK_ADC_EN_ALL					CODEC_CLK_ADC_EN(7)
#define CODEC_CLK_ADC_ANA_EN_SHIFT			0	// gate enable of clk_adc_anc
#define CODEC_CLK_ADC_ANA_EN_MASK			(0x3 << CODEC_CLK_ADC_ANA_EN_SHIFT)
#define CODEC_CLK_ADC_ANA_EN(n)				BITFIELD_VAL(CODEC_CLK_ADC_ANA_EN, n)
#define CODEC_CLK_ADC_ANA_EN_ALL			CODEC_CLK_ADC_ANA_EN(3)

// REG0044: (RW) ?
#define CODEC_ROM0_PGEN	(1<<11)
#define CODEC_ROM1_PGEN	(1<<10)
#define CODEC_ROM_KEN		(1<<9)
#define CODEC_ROM_EMA_SHIFT	6
#define CODEC_ROM_EMA_MASK	(0x7 << CODEC_ROM_EMA_SHIFT)
#define CODEC_ROM_EMA(n)	BITFIELD_VAL(CODEC_ROM_EMA_SHIFT, n)
#define CODEC_RF_EMAW_SHIFT	4
#define CODEC_RF_EMAW_MASK	(0x3 << CODEC_RF_EMAW_SHIFT)
#define CODEC_RF_EMAW(n)	BITFIELD_VAL(CODEC_RF_EMAW, n)
#define CODEC_RF_EMA_SHIFT	1
#define CODEC_RF_EMA_MASK		(0x7 << CODEC_RF_EMA_SHIFT)
#define CODEC_RF_EMA(n)		BITFIELD_VAL(CODEC_RF_EMA, n)
#define CODEC_RF_RET1N		(1 << 0)

// REG0048: (RW) SOFT_RSTN
#define CODEC_SOFT_RSTN_IIR			(1<<7)	// soft reset of clk_iir domain
#define CODEC_SOFT_RSTN_RS			(1<<6)	// soft reset of clk_rs domain
#define CODEC_SOFT_RSTN_DAC			(1<<5)	// soft reset of clk_dac domain
#define CODEC_SOFT_RSTN_ADC_SHIFT			2	// soft reset of clk_adc domain
#define CODEC_SOFT_RSTN_ADC_MASK				(0x7 << CODEC_SOFT_RSTN_ADC_SHIFT)
#define CODEC_SOFT_RSTN_ADC(n)				BITFIELD_VAL(CODEC_SOFT_RSTN_ADC_ANA, n)
#define CODEC_SOFT_RSTN_ADC_ALL				CODEC_SOFT_RSTN_ADC(7)
#define CODEC_SOFT_RSTN_ADC_ANA_SHIFT		0	// soft reset of clk_adc_ana domain
#define CODEC_SOFT_RSTN_ADC_ANA_MASK		(0x3 << CODEC_SOFT_RSTN_ADC_ANA_SHIFT)
#define CODEC_SOFT_RSTN_ADC_ANA(n)		BITFIELD_VAL(CODEC_SOFT_RSTN_ADC_ANA, n)
#define CODEC_SOFT_RSTN_ADC_ANA_ALL	    CODEC_SOFT_RSTN_ADC_ANA(3)	

// REG0080: (RW) ADC_CFG
#define CODEC_TEST_PORT_SEL_SHIFT		15	// soft reset of clk_adc domain
#define CODEC_TEST_PORT_SEL_MASK		(0x1f << CODEC_SOFT_RSTN_ADC_SHIFT)
#define CODEC_TEST_PORT_SEL(n)		BITFIELD_VAL(CODEC_TEST_PORT_SEL, n)
#define CODEC_ADC_GAIN_UPD_CH1	(1<<14)	// 1: will update gain;  0: not update gain
#define CODEC_ADC_GAIN_UPD_CH0	(1<<13)	// 1: will update gain;  0: not update gain
#define CODEC_ADC_LOOP_SEL_R		(1<<12)	// select looped adc to R dac;  0: adc ch0;  1: adc ch1; 
#define CODEC_ADC_LOOP_SEL_L		(1<<11)	// select looped adc to R dac;  0: adc ch0;  1: adc ch1; 
#define CODEC_ADC_LOOP				(1<<10)	// 1: loop two adc to two dac
#define CODEC_SIDETONE_IIR_EN		(1<<9)	// sidetone for iir eq en
#define CODEC_SIDETONE_MIC_SEL	(1<<8)	// 0: select L loop adc;   1: select R loop adc
#define CODEC_SIDETONE_GAIN_SHIFT		3	// 30db ~ -30db;   0: 36db;  each step -2db;  30: -24db;  31: mute
#define CODEC_SIDETONE_GAIN_MASK		(0x1f << CODEC_SIDETONE_GAIN_SHIFT)
#define CODEC_SIDETONE_GAIN(n)		BITFIELD_VAL(CODEC_SIDETONE_GAIN, n)
#define CODEC_ADC_CH1_EN			(1<<2)	// adc ch1 enable
#define CODEC_ADC_CH0_EN			(1<<1)	// adc ch0 enable
#define CODEC_ADC_EN				(1<<0)	// adc enable; if any ch of adc enabled, this should be enabled

// REG0084: (RW) ADC_CFG_CH0
#define CODEC_ADC_CH0_IIR_EN		(1<<30)	// 1: use iir_a for adc eq; 
#define CODEC_ADC_CH0_FIR_DS_SEL	(1<<29)	// 1: adc  384k to 48k;  0: adc 192k to 48k
#define CODEC_ADC_CH0_FIR_DS_EN	(1<<28)	// 1: adc 192k/384k to 48k downsample use fir
#define CODEC_ADC_CH0_GAIN_SHIFT		8	// adc gain;  format 8.12
#define CODEC_ADC_CH0_GAIN_MASK		(0xfffff << CODEC_ADC_CH0_GAIN_SHIFT)
#define CODEC_ADC_CH0_GAIN(n)		BITFIELD_VAL(CODEC_ADC_CH0_GAIN, n)
#define CODEC_ADC_CH0_GAIN_SEL		(1<<7)	// 1: gain updated when pass 0
#define CODEC_ADC_CH0_HBF1_BYPASS	(1<<6)
#define CODEC_ADC_CH0_HBF2_BYPASS	(1<<5)
#define CODEC_ADC_CH0_HBF3_BYPASS	(1<<4)	// hbf3 bypass; cancel one div2; 
#define CODEC_ADC_CH0_DOWN_SEL_SHIFT	2
#define CODEC_ADC_CH0_DOWN_SEL_MASK		(0x3 << CODEC_ADC_CH0_DOWN_SEL_SHIFT)
#define CODEC_ADC_CH0_DOWN_SEL(n)		BITFIELD_VAL(CODEC_ADC_CH0_DOWN_SEL, n)
#define CODEC_ADC_CH0_IN_SEL		(1<<1)	// select which analog adc to dig adc ch0 path; 0: analog adc0; 1: analog adc1; 
#define CODEC_ADC_CH0_IN_INV		(1<<0)	// adc in bit reverse, change bit[1:0] to bit[0:1]

// REG0088: (RW) ADC_CFG_CH1
#define CODEC_ADC_CH1_IIR_EN		(1<<30)	// 1: use iir_a for adc eq; 
#define CODEC_ADC_CH1_FIR_DS_SEL	(1<<29)	// 1: adc  384k to 48k;  0: adc 192k to 48k
#define CODEC_ADC_CH1_FIR_DS_EN	(1<<28)	// 1: adc 192k/384k to 48k downsample use fir
#define CODEC_ADC_CH1_GAIN_SHIFT		8	// adc gain;  format 8.12
#define CODEC_ADC_CH1_GAIN_MASK		(0xfffff << CODEC_ADC_CH1_GAIN_SHIFT)
#define CODEC_ADC_CH1_GAIN(n)		BITFIELD_VAL(CODEC_ADC_CH1_GAIN, n)
#define CODEC_ADC_CH1_GAIN_SEL		(1<<7)	// 1: gain updated when pass 0
#define CODEC_ADC_CH1_HBF1_BYPASS	(1<<6)
#define CODEC_ADC_CH1_HBF2_BYPASS	(1<<5)
#define CODEC_ADC_CH1_HBF3_BYPASS	(1<<4)	// hbf3 bypass; cancel one div2; 
#define CODEC_ADC_CH1_DOWN_SEL_SHIFT	2
#define CODEC_ADC_CH1_DOWN_SEL_MASK		(0x1f << CODEC_ADC_CH1_DOWN_SEL_SHIFT)
#define CODEC_ADC_CH1_DOWN_SEL(n)		BITFIELD_VAL(CODEC_ADC_CH1_DOWN_SEL, n)
#define CODEC_ADC_CH1_IN_SEL		(1<<1)	// select which analog adc to dig adc ch1 path; 0: analog adc1; 1: analog adc0; 
#define CODEC_ADC_CH1_IN_INV		(1<<0)	// adc in bit reverse, change bit[1:0] to bit[0:1]

// REG008C: (RW) DAC_CFG
#define CODEC_DAC_R_IIR_EN			(1<<29)	// 1: use iir_d ch1 for dac R eq
#define CODEC_DAC_L_IIR_EN			(1<<28)	// 1: use iir_d ch0 for dac L eq
#define CODEC_DAC_R_FIR_US			(1<<27)	
#define CODEC_DAC_L_FIR_US			(1<<26)	// 1: use fir to upsample 4x;  save latency;  used for kala ok mode
#define CODEC_DAC_SDM_SWAP		(1<<25)	// sdm data swap
#define CODEC_DAC_LR_SWAP			(1<<24)	// LR Swap
#define CODEC_DAC_SIN1K_STEP_SHIFT	20	// dac tone step;  fin = fs*step/48;  for example: if step == 1, gen 1k tone for 48k fs
#define CODEC_DAC_SIN1K_STEP_MASK	(0x1f << CODEC_DAC_SIN1K_STEP_SHIFT)
#define CODEC_DAC_SIN1K_STEP(n)	BITFIELD_VAL(CODEC_DAC_SIN1K_STEP, n)
#define CODEC_DAC_TONE_TEST		(1<<19)	// 1: enable dac tone gen
#define CODEC_DAC_OSR_SEL_SHIFT		17	// 0: up4;  1: up2;  2: up1/bypass
#define CODEC_DAC_OSR_SEL_MASK		(0x3 << CODEC_DAC_OSR_SEL_SHIFT)
#define CODEC_DAC_OSR_SEL(n)		BITFIELD_VAL(CODEC_DAC_OSR_SEL, n)
#define CODEC_DAC_UP_SEL_SHIFT		14	// 0: up2;  1: up3;  2: up4;  3: up6;  4~7: up1/bypass
#define CODEC_DAC_UP_SEL_MASK			(0x7 << CODEC_DAC_UP_SEL_SHIFT)
#define CODEC_DAC_UP_SEL(n)		BITFIELD_VAL(CODEC_DAC_UP_SEL, n)
#define CODEC_DAC_HBF1_BYPASS		(1<<13)	
#define CODEC_DAC_HBF2_BYPASS		(1<<12)
#define CODEC_DAC_HBF3_BYPASS		(1<<11)	// hbf3 bypass; cancel one div2; 
#define CODEC_DAC_DITHER_BYPASS	(1<<10)	
#define CODEC_DAC_DITHER_GAIN_SHIFT	3
#define CODEC_DAC_DITHER_GAIN_MASK	(0x7f << CODEC_DAC_DITHER_GAIN_SHIFT)
#define CODEC_DAC_DITHER_GAIN(n)	BITFIELD_VAL(CODEC_DAC_DITHER_GAIN, n)
#define CODEC_DAC_CH0_EN			(1<<2)
#define CODEC_DAC_CH1_EN			(1<<1)
#define CODEC_DAC_EN				(1<<0)

// REG0090: (RW) DAC_GAIN_CH0
#define CODEC_DAC_CH0_GAIN_SEL	(1<<20)
#define CODEC_DAC_CH0_GAIN_SHIFT		(0)
#define CODEC_DAC_CH0_GAIN_MASK		(0xfffff<<CODEC_DAC_CH0_GAIN_SHIFT)
#define CODEC_DAC_CH0_GAIN(n)		BITFIELD_VAL(CODEC_DAC_CH0_GAIN, n)

// REG0094: (RW) DAC_GAIN_CH1
#define CODEC_DAC_CH1_GAIN_SEL	(1<<20)
#define CODEC_DAC_CH1_GAIN_SHIFT		(0)
#define CODEC_DAC_CH1_GAIN_MASK		(0xfffff<<CODEC_DAC_CH1_GAIN_SHIFT)
#define CODEC_DAC_CH1_GAIN(n)		BITFIELD_VAL(CODEC_DAC_CH1_GAIN, n)

// REG0098: (RW) DAC_GAIN_CFG
#define CODEC_DAC_CH1_DC_UPDATE_STATUS		(1<<24)
#define CODEC_DAC_CH0_DC_UPDATE_STATUS		(1<<23)		// read only; 1: updated
#define CODEC_DAC_CH1_ANA_GAIN_UPDATE_DELAY_SHIFT		15
#define CODEC_DAC_CH1_ANA_GAIN_UPDATE_DELAY_MASK		(0xff << CODEC_DAC_CH1_ANA_GAIN_UPDATE_DELAY_SHIFT)
#define CODEC_DAC_CH1_ANA_GAIN_UPDATE_DELAY(n)		BITFIELD_VAL(CODEC_DAC_CH1_ANA_GAIN_UPDATE_DELAY, n)
#define CODEC_DAC_CH0_ANA_GAIN_UPDATE_DELAY_SHIFT		7
#define CODEC_DAC_CH0_ANA_GAIN_UPDATE_DELAY_MASK		(0xff << CODEC_DAC_CH0_ANA_GAIN_UPDATE_DELAY_SHIFT)
#define CODEC_DAC_CH0_ANA_GAIN_UPDATE_DELAY(n)		BITFIELD_VAL(CODEC_DAC_CH0_ANA_GAIN_UPDATE_DELAY, n)
#define CODEC_DAC_CH1_DC_UPDATE_PASS0	(1<<6)
#define CODEC_DAC_CH0_DC_UPDATE_PASS0	(1<<5)
#define CODEC_DAC_CH1_DC_UPDATE			(1<<4)	// 1: dc update when pass0
#define CODEC_DAC_CH0_DC_UPDATE			(1<<3)	// update dc, also update ana gain.
#define CODEC_DAC_GAIN_TRIGGER_SEL_SHIFT		1	// 0: no trigger;  1: gpio trigger;  2: bt trigger;
#define CODEC_DAC_GAIN_TRIGGER_SEL_MASK		(0x3 << CODEC_DAC_GAIN_TRIGGER_SEL_SHIFT)
#define CODEC_DAC_GAIN_TRIGGER_SEL(n)		BITFIELD_VAL(CODEC_DAC_GAIN_TRIGGER_SEL, n)
#define CODEC_DAC_GAIN_UPDATE			(1<<0)	// used for dac L and R for update at same time; posedge is used to update gain

// REG009C: (RW) DAC_DC_CFG
#define CODEC_DAC_CH1_DC_SHIFT		15
#define CODEC_DAC_CH1_DC_MASK		(0x7fff << CODEC_DAC_CH1_DC_SHIFT)
#define CODEC_DAC_CH1_DC(n)		BITFIELD_VAL(CODEC_DAC_CH1_DC, n)
#define CODEC_DAC_CH0_DC_SHIFT		0	// Format 1.14,  added to dac [19:0]'s [14:0]
#define CODEC_DAC_CH0_DC_MASK		(0x7fff << CODEC_DAC_CH0_DC_SHIFT)
#define CODEC_DAC_CH0_DC(n)		BITFIELD_VAL(CODEC_DAC_CH1_DC, n)

// REG00A0: (RW) PDM_CFG
#define CODEC_PDM_CH1_CAP_PHASE_SHIFT	9
#define CODEC_PDM_CH1_CAP_PHASE_MASK	(0x7 << CODEC_PDM_CH1_CAP_PHASE_SHIFT)
#define CODEC_PDM_CH1_CAP_PHASE(n)	BITFIELD_VAL(CODEC_PDM_CH1_CAP_PHASE, n)
#define CODEC_PDM_CH0_CAP_PHASE_SHIFT	6	// mems capture data phase select
#define CODEC_PDM_CH0_CAP_PHASE_MASK	(0x7 << CODEC_PDM_CH0_CAP_PHASE_SHIFT)
#define CODEC_PDM_CH0_CAP_PHASE(n)	BITFIELD_VAL(CODEC_PDM_CH0_CAP_PHASE, n)
#define CODEC_PDM_CH1_MUX				(1<<5)	// select which mems to dig adc ch1;   0: mems0; 1:mems1
#define CODEC_PDM_CH0_MUX				(1<<4)	// select which mems to dig adc ch0;   0: mems0; 1:mems1
#define CODEC_PDM_CH1_ADC_SEL			(1<<3)	// 1: dig adc ch1 path use mems
#define CODEC_PDM_CH0_ADC_SEL			(1<<2)	// 1: dig adc ch0 path use mems
#define CODEC_PDM_DATA_INV			(1<<1)	// data inv;  1: 0 to 1, 1 to 0
#define CODEC_PDM_EN					(1<<0)	// dig mems enable

// REG00A4: (RW) RAMP_CFG_CH0
#define CODEC_RAMP_CH0_INTERVAL_SHIFT	14		
#define CODEC_RAMP_CH0_INTERVAL_MASK	(7 << CODEC_RAMP_CH0_INTERVAL_SHIFT)
#define CODEC_RAMP_CH0_INTERVAL(n)	BITFIELD_VAL(CODEC_RAMP_CH0_INTERVAL, n)
#define CODEC_RAMP_CH0_EN				(1<<13)
#define CODEC_RAMP_CH0_DIRECTION		(1<<12)
#define CODEC_RAMP_CH0_STEP_SHIFT		0		
#define CODEC_RAMP_CH0_STEP_MASK			(0xfff << CODEC_RAMP_CH0_INTERVAL_SHIFT)
#define CODEC_RAMP_CH0_STEPL(n)		BITFIELD_VAL(CODEC_RAMP_CH0_STEPL, n)

// REG00A8: (RW) RAMP_CFG_CH1
#define CODEC_RAMP_CH1_INTERVAL_SHIFT	14	// 0: each sample update gain;  1: 2 sample;  2: 4 sample;   3: 8 sample;   4: 16 sample;		
#define CODEC_RAMP_CH1_INTERVAL_MASK	(7 << CODEC_RAMP_CH1_INTERVAL_SHIFT)
#define CODEC_RAMP_CH1_INTERVAL(n)	BITFIELD_VAL(CODEC_RAMP_CH1_INTERVAL, n)
#define CODEC_RAMP_CH1_EN				(1<<13)	//  ramp enable,  ramp up or down to target gain
#define CODEC_RAMP_CH1_DIRECTION		(1<<12)	//  0: ramp up;   1: ramp down;
#define CODEC_RAMP_CH1_STEP_SHIFT		0	//  Format 0.12
#define CODEC_RAMP_CH1_STEP_MASK			(0xfff << CODEC_RAMP_CH1_INTERVAL_SHIFT)
#define CODEC_RAMP_CH1_STEPL(n)		BITFIELD_VAL(CODEC_RAMP_CH1_STEPL, n)

// REG00B4: (RW) ADC0_DC_CFG
#define CODEC_ADC0_DC_UPDATE			(1<<23)
#define CODEC_ADC0_DC_DIN_SHIFT			0	// adc dc cancel;  do at 768k sample rate
#define CODEC_ADC0_DC_DIN_MASK			(0x7fffff << CODEC_ADC0_DC_DIN_SHIFT)
#define CODEC_ADC0_DC_DIN(n)			BITFIELD_VAL(CODEC_ADC0_DC_DIN, n)

// REG00B8: (RW) ADC1_DC_CFG
#define CODEC_ADC1_DC_UPDATE			(1<<23)
#define CODEC_ADC1_DC_DIN_SHIFT			0
#define CODEC_ADC1_DC_DIN_MASK			(0x7fffff << CODEC_ADC1_DC_DIN_SHIFT)
#define CODEC_ADC1_DC_DIN(n)			BITFIELD_VAL(CODEC_ADC1_DC_DIN, n)

// REG00BC: (RW) DSD_CFG1
#define CODEC_DSD_SAMPLE_RATE_SHIFT		3	//  0: dsdx64;  1: dsdx128;  2: dsdx256;
#define CODEC_DSD_SAMPLE_RATE_MASK		(0x3 << CODEC_DSD_SAMPLE_RATE_SHIFT)
#define CODEC_DSD_SAMPE_RATE(n)		BITFIELD_VAL(CODEC_DSD_SAMPE_RATE, n)
#define CODEC_DSD_DATA_INV			(1<<2)	// dsd data inv;  1:  0 to 1, 1 to 0
#define CODEC_DSD_R_EN					(1<<1)	// dsd R enable
#define CODEC_DSD_L_EN					(1<<0)	// dsd L enable

// REG00C0: (RW) ADC0_DRE_CFG0
#define CODEC_ADC_CH0_DRE_MUTE_STATUS	                (1<<26)		// (RO)
#define CODEC_ADC_CH0_DRE_MUTE_RANGE_SEL_SHIFT		24
#define CODEC_ADC_CH0_DRE_MUTE_RANGE_SEL_MASK		(0x3 << CODEC_ADC_CH0_DRE_MUTE_RANGE_SEL_SHIFT)
#define CODEC_ADC_CH0_DRE_MUTE_RANGE_SEL(n)         BITFIELD_VAL(CODEC_ADC_CH0_DRE_MUTE_RANGE_SEL, n)
#define CODEC_ADC_CH0_DRE_MUTE_MODE                     (1<<23)
#define CODEC_ADC_CH0_DRE_OVERFLOW_MUTE_EN      (1<<22)
#define CODEC_ADC_CH0_DRE_BIT_SEL_SHIFT     20
#define CODEC_ADC_CH0_DRE_BIT_SEL_MASK      (0x3 << CODEC_ADC_CH0_DRE_BIT_SEL_SHIFT)
#define CODEC_ADC_CH0_DRE_BIT_SEL(n)        BITFIELD_VAL(CODEC_ADC_CH0_DRE_BIT_SEL, n)
#define CODEC_ADC_CH0_DRE_THD_DB_OFFSET_SIGN    (1<<19)
#define CODEC_ADC_CH0_DRE_DELAY_ANA_SHIFT       14
#define CODEC_ADC_CH0_DRE_DELAY_ANA_MASK        (0x1f << CODEC_ADC_CH0_DRE_DELAY_ANA_SHIFT)
#define CODEC_ADC_CH0_DRE_DELAY_ANA(n)      BITFIELD_VAL(CODEC_ADC_CH0_DRE_DELAY_ANA, n)
#define CODEC_ADC_CH0_DRE_DELAY_DIG_SHIFT       11
#define CODEC_ADC_CH0_DRE_DELAY_DIG_MASK        (0x7 << CODEC_ADC_CH0_DRE_DELAY_DIG_SHIFT)
#define CODEC_ADC_CH0_DRE_DELAY_DIG(n)      BITFIELD_VAL(CODEC_ADC_CH0_DRE_DELAY_DIG, n)
#define CODEC_ADC_CH0_DRE_INI_ANA_GAIN_SHIFT       7
#define CODEC_ADC_CH0_DRE_INI_ANA_GAIN_MASK        (0xf << CODEC_ADC_CH0_DRE_INI_ANA_GAIN_SHIFT)
#define CODEC_ADC_CH0_DRE_INI_ANA_GAIN(n)       BITFIELD_VAL(CODEC_ADC_CH0_DRE_INI_ANA_GAIN, n)
#define CODEC_ADC_CH0_DRE_THD_DB_OFFSET_SHIFT   3
#define CODEC_ADC_CH0_DRE_THD_DB_OFFSET_MASK    (0xf << CODEC_ADC_CH0_DRE_THD_DB_OFFSET_SHIFT)
#define CODEC_ADC_CH0_DRE_THD_DB_OFFSET(n)      BITFIELD_VAL(CODEC_ADC_CH0_DRE_THD_DB_OFFSET, n)
#define CODEC_ADC_CH0_DRE_STEP_MODE_SHIFT   1
#define CODEC_ADC_CH0_DRE_STEP_MODE_MASK    (0x3 << CODEC_ADC_CH0_DRE_STEP_MODE_SHIFT)
#define CODEC_ADC_CH0_DRE_STEP_MODE(n)      BITFIELD_VAL(CODEC_ADC_CH0_DRE_STEP_MODE, n)
#define CODEC_ADC_CH0_DRE_EN                         (1<<0)

// REG00C4: (RW) ADC0_DRE_CFG1
#define CODEC_ADC_CH0_DRE_WINDOW_SHIFT          11
#define CODEC_ADC_CH0_DRE_WINDOW_MASK           (0xfffff << CODEC_ADC_CH0_DRE_WINDOW_SHIFT)
#define CODEC_ADC_CH0_DRE_WINDOW(n)      BITFIELD_VAL(CODEC_ADC_CH0_DRE_WINDOW, n)
#define CODEC_ADC_CH0_DRE_AMP_HIGH_SHIFT       0
#define CODEC_ADC_CH0_DRE_AMP_HIGH_MASK        (0x7ff << CODEC_ADC_CH0_DRE_AMP_HIGH_SHIFT)
#define CODEC_ADC_CH0_DRE_AMP_HIGH(n)      BITFIELD_VAL(CODEC_ADC_CH0_DRE_AMP_HIGH, n)

// REG00C8: (RW) ADC1_DRE_CFG0
#define CODEC_ADC_CH1_DRE_MUTE_STATUS	                (1<<26)		// (RO)
#define CODEC_ADC_CH1_DRE_MUTE_RANGE_SEL_SHIFT		24
#define CODEC_ADC_CH1_DRE_MUTE_RANGE_SEL_MASK		(0x3 << CODEC_ADC_CH1_DRE_MUTE_RANGE_SEL_SHIFT)
#define CODEC_ADC_CH1_DRE_MUTE_RANGE_SEL(n)         BITFIELD_VAL(CODEC_ADC_CH1_DRE_MUTE_RANGE_SEL, n)
#define CODEC_ADC_CH1_DRE_MUTE_MODE                     (1<<23)
#define CODEC_ADC_CH1_DRE_OVERFLOW_MUTE_EN      (1<<22)
#define CODEC_ADC_CH1_DRE_BIT_SEL_SHIFT     20
#define CODEC_ADC_CH1_DRE_BIT_SEL_MASK      (0x3 << CODEC_ADC_CH1_DRE_BIT_SEL_SHIFT)
#define CODEC_ADC_CH1_DRE_BIT_SEL(n)        BITFIELD_VAL(CODEC_ADC_CH1_DRE_BIT_SEL, n)
#define CODEC_ADC_CH1_DRE_THD_DB_OFFSET_SIGN    (1<<19)
#define CODEC_ADC_CH1_DRE_DELAY_ANA_SHIFT       14
#define CODEC_ADC_CH1_DRE_DELAY_ANA_MASK        (0x1f << CODEC_ADC_CH1_DRE_DELAY_ANA_SHIFT)
#define CODEC_ADC_CH1_DRE_DELAY_ANA(n)      BITFIELD_VAL(CODEC_ADC_CH1_DRE_DELAY_ANA, n)
#define CODEC_ADC_CH1_DRE_DELAY_DIG_SHIFT       11
#define CODEC_ADC_CH1_DRE_DELAY_DIG_MASK        (0x7 << CODEC_ADC_CH1_DRE_DELAY_DIG_SHIFT)
#define CODEC_ADC_CH1_DRE_DELAY_DIG(n)      BITFIELD_VAL(CODEC_ADC_CH1_DRE_DELAY_DIG, n)
#define CODEC_ADC_CH1_DRE_INI_ANA_GAIN_SHIFT       7
#define CODEC_ADC_CH1_DRE_INI_ANA_GAIN_MASK        (0xf << CODEC_ADC_CH1_DRE_INI_ANA_GAIN_SHIFT)
#define CODEC_ADC_CH1_DRE_INI_ANA_GAIN(n)       BITFIELD_VAL(CODEC_ADC_CH1_DRE_INI_ANA_GAIN, n)
#define CODEC_ADC_CH1_DRE_THD_DB_OFFSET_SHIFT   3
#define CODEC_ADC_CH1_DRE_THD_DB_OFFSET_MASK    (0xf << CODEC_ADC_CH1_DRE_THD_DB_OFFSET_SHIFT)
#define CODEC_ADC_CH1_DRE_THD_DB_OFFSET(n)      BITFIELD_VAL(CODEC_ADC_CH1_DRE_THD_DB_OFFSET, n)
#define CODEC_ADC_CH1_DRE_STEP_MODE_SHIFT   1
#define CODEC_ADC_CH1_DRE_STEP_MODE_MASK    (0x3 << CODEC_ADC_CH1_DRE_STEP_MODE_SHIFT)
#define CODEC_ADC_CH1_DRE_STEP_MODE(n)      BITFIELD_VAL(CODEC_ADC_CH1_DRE_STEP_MODE, n)
#define CODEC_ADC_CH1_DRE_EN                         (1<<0)

// REG00CC: (RW) ADC1_DRE_CFG1
#define CODEC_ADC_CH1_DRE_WINDOW_SHIFT          11
#define CODEC_ADC_CH1_DRE_WINDOW_MASK           (0xfffff << CODEC_ADC_CH1_DRE_WINDOW_SHIFT)
#define CODEC_ADC_CH1_DRE_WINDOW(n)         BITFIELD_VAL(CODEC_ADC_CH1_DRE_WINDOW, n)
#define CODEC_ADC_CH1_DRE_AMP_HIGH_SHIFT       0
#define CODEC_ADC_CH1_DRE_AMP_HIGH_MASK        (0x7ff << CODEC_ADC_CH1_DRE_AMP_HIGH_SHIFT)
#define CODEC_ADC_CH1_DRE_AMP_HIGH(n)      BITFIELD_VAL(CODEC_ADC_CH1_DRE_AMP_HIGH, n)

// REG00D0: (RW) DAC0_DRE_CFG0
#define CODEC_DAC_CH0_DRE_GAIN_OFFSET_SHIFT     18
#define CODEC_DAC_CH0_DRE_GAIN_OFFSET_MASK      (0x7 << CODEC_DAC_CH0_DRE_GAIN_OFFSET_SHIFT)
#define CODEC_DAC_CH0_DRE_GAIN_OFFSET(n)    BITFIELD_VAL(CODEC_DAC_CH0_DRE_GAIN_OFFSET, n)
#define CODEC_DAC_CH0_DRE_THD_DB_OFFSET_SIGN    (1<<17)
#define CODEC_DAC_CH0_DRE_DELAY_SHIFT       11
#define CODEC_DAC_CH0_DRE_DELAY_MASK        (0x3f << CODEC_DAC_CH0_DRE_DELAY_SHIFT)
#define CODEC_DAC_CH0_DRE_DELAY(n)          BITFIELD_VAL(CODEC_DAC_CH0_DRE_DELAY, n)
#define CODEC_DAC_CH0_DRE_INI_ANA_GAIN_SHIFT    7
#define CODEC_DAC_CH0_DRE_INI_ANA_GAIN_MASK     (0xf << CODEC_DAC_CH0_DRE_INI_ANA_GAIN_SHIFT)
#define CODEC_DAC_CH0_DRE_INI_ANA_GAIN(n)       BITFIELD_VAL(CODEC_DAC_CH0_DRE_INI_ANA_GAIN, n)
#define CODEC_DAC_CH0_DRE_THD_DB_OFFSET_SHIFT   3
#define CODEC_DAC_CH0_DRE_THD_DB_OFFSET_MASK    (0xf << CODEC_DAC_CH0_DRE_THD_DB_OFFSET_SHIFT)
#define CODEC_DAC_CH0_DRE_THD_DB_OFFSET(n)      BITFIELD_VAL(CODEC_DAC_CH0_DRE_THD_DB_OFFSET, n)
#define CODEC_DAC_CH0_DRE_STEP_MODE_SHIFT           1
#define CODEC_DAC_CH0_DRE_STEP_MODE_MASK            (0x3 << CODEC_DAC_CH0_DRE_STEP_MODE_SHIFT)
#define CODEC_DAC_CH0_DRE_STEP_MODE(n)              BITFIELD_VAL(CODEC_DAC_CH0_DRE_STEP_MODE, n)
#define CODEC_DAC_CH0_DRE_EN                    (1<<0)

// REG00D4: (RW) DAC0_DRE_CFG1
#define CODEC_DAC_CH0_DRE_WINDOW_SHIFT          11
#define CODEC_DAC_CH0_DRE_WINDOW_MASK           (0xfffff << CODEC_DAC_CH0_DRE_WINDOW_SHIFT)
#define CODEC_DAC_CH0_DRE_WINDOW(n)         BITFIELD_VAL(CODEC_DAC_CH0_DRE_WINDOW, n)
#define CODEC_DAC_CH0_DRE_AMP_HIGH_SHIFT       0
#define CODEC_DAC_CH0_DRE_AMP_HIGH_MASK        (0x7ff << CODEC_DAC_CH0_DRE_AMP_HIGH_SHIFT)
#define CODEC_DAC_CH0_DRE_AMP_HIGH(n)      BITFIELD_VAL(CODEC_DAC_CH0_DRE_AMP_HIGH, n)

// REG00D8: (RW) DAC1_DRE_CFG0
#define CODEC_DAC_CH1_DRE_GAIN_OFFSET_SHIFT     18
#define CODEC_DAC_CH1_DRE_GAIN_OFFSET_MASK      (0x7 << CODEC_DAC_CH1_DRE_GAIN_OFFSET_SHIFT)
#define CODEC_DAC_CH1_DRE_GAIN_OFFSET(n)    BITFIELD_VAL(CODEC_DAC_CH1_DRE_GAIN_OFFSET, n)
#define CODEC_DAC_CH1_DRE_THD_DB_OFFSET_SIGN    (1<<17)
#define CODEC_DAC_CH1_DRE_DELAY_SHIFT       11
#define CODEC_DAC_CH1_DRE_DELAY_MASK        (0x3f << CODEC_DAC_CH1_DRE_DELAY_SHIFT)
#define CODEC_DAC_CH1_DRE_DELAY(n)          BITFIELD_VAL(CODEC_DAC_CH1_DRE_DELAY, n)
#define CODEC_DAC_CH1_DRE_INI_ANA_GAIN_SHIFT    7
#define CODEC_DAC_CH1_DRE_INI_ANA_GAIN_MASK     (0xf << CODEC_DAC_CH1_DRE_INI_ANA_GAIN_SHIFT)
#define CODEC_DAC_CH1_DRE_INI_ANA_GAIN(n)       BITFIELD_VAL(CODEC_DAC_CH1_DRE_INI_ANA_GAIN, n)
#define CODEC_DAC_CH1_DRE_THD_DB_OFFSET_SHIFT   3
#define CODEC_DAC_CH1_DRE_THD_DB_OFFSET_MASK    (0xf << CODEC_DAC_CH1_DRE_THD_DB_OFFSET_SHIFT)
#define CODEC_DAC_CH1_DRE_THD_DB_OFFSET(n)      BITFIELD_VAL(CODEC_DAC_CH1_DRE_THD_DB_OFFSET, n)
#define CODEC_DAC_CH1_DRE_STEP_MODE_SHIFT           1
#define CODEC_DAC_CH1_DRE_STEP_MODE_MASK            (0x3 << CODEC_DAC_CH1_DRE_STEP_MODE_SHIFT)
#define CODEC_DAC_CH1_DRE_STEP_MODE(n)              BITFIELD_VAL(CODEC_DAC_CH1_DRE_STEP_MODE, n)
#define CODEC_DAC_CH1_DRE_EN                    (1<<0)

// REG00DC: (RW) DAC0_DRE_CFG1
#define CODEC_DAC_CH1_DRE_WINDOW_SHIFT          11
#define CODEC_DAC_CH1_DRE_WINDOW_MASK           (0xfffff << CODEC_DAC_CH1_DRE_WINDOW_SHIFT)
#define CODEC_DAC_CH1_DRE_WINDOW(n)         BITFIELD_VAL(CODEC_DAC_CH1_DRE_WINDOW, n)
#define CODEC_DAC_CH1_DRE_AMP_HIGH_SHIFT       0
#define CODEC_DAC_CH1_DRE_AMP_HIGH_MASK        (0x7ff << CODEC_DAC_CH1_DRE_AMP_HIGH_SHIFT)
#define CODEC_DAC_CH1_DRE_AMP_HIGH(n)      BITFIELD_VAL(CODEC_DAC_CH1_DRE_AMP_HIGH, n)

// REG00E0: (RW) IIR_CFG
#define CODEC_IIR_A_COEF_SWAP_STATUS            (1<<30)     // RO, coef memoryX used currently
#define CODEC_IIR_D_COEF_SWAP_STATUS            (1<<29)     // RO
#define CODEC_IIR_A_COEF_SWAP                           (1<<28)     // 0: use iir coef memory0;  1: use iir coef memory1;
#define CODEC_IIR_D_COEF_SWAP                           (1<<27)     
#define CODEC_IIR_A_CH2_COUNT_SHIFT     23
#define CODEC_IIR_A_CH2_COUNT_MASK      (0xf << CODEC_IIR_A_CH2_COUNT_SHIFT)
#define CODEC_IIR_A_CH2_COUNT(n)        BITFIELD_VAL(CODEC_IIR_A_CH2_COUNT, n)
#define CODEC_IIR_A_CH1_COUNT_SHIFT     19
#define CODEC_IIR_A_CH1_COUNT_MASK      (0xf << CODEC_IIR_A_CH1_COUNT_SHIFT)
#define CODEC_IIR_A_CH1_COUNT(n)        BITFIELD_VAL(CODEC_IIR_A_CH1_COUNT, n)
#define CODEC_IIR_A_CH0_COUNT_SHIFT     15          // ch0 iir number  0~8
#define CODEC_IIR_A_CH0_COUNT_MASK      (0xf << CODEC_IIR_A_CH0_COUNT_SHIFT)
#define CODEC_IIR_A_CH0_COUNT(n)        BITFIELD_VAL(CODEC_IIR_A_CH0_COUNT, n)
#define CODEC_IIR_A_CH2_BYPASS          (1<<14)
#define CODEC_IIR_A_CH1_BYPASS          (1<<13)
#define CODEC_IIR_A_CH0_BYPASS          (1<<12)     // 1: ch0 bypass
#define CODEC_IIR_A_EN                          (1<<11)     // iir_a module enable
#define CODEC_IIR_D_CH1_COUNT_SHIFT     7
#define CODEC_IIR_D_CH1_COUNT_MASK      (0xf << CODEC_IIR_D_CH1_COUNT_SHIFT)
#define CODEC_IIR_D_CH1_COUNT(n)        BITFIELD_VAL(CODEC_IIR_D_CH1_COUNT, n)
#define CODEC_IIR_D_CH0_COUNT_SHIFT     3
#define CODEC_IIR_D_CH0_COUNT_MASK      (0xf << CODEC_IIR_D_CH0_COUNT_SHIFT)
#define CODEC_IIR_D_CH0_COUNT(n)        BITFIELD_VAL(CODEC_IIR_D_CH0_COUNT, n)
#define CODEC_IIR_D_CH1_BYPASS          (1<<2)
#define CODEC_IIR_D_CH0_BYPASS          (1<<1)      // 1: ch0 bypass
#define CODEC_IIR_D_EN                          (1<<0)      // iir_d module enable

// REG00E4: (RW) RSPL_CFG
#define CODEC_RSPL_ADC_TRIG_SEL_SHIFT       9       // 0: no trigger;  1: gpio trigger;  2: bt trigger;
#define CODEC_RSPL_ADC_TRIG_SEL_MASK        (0x3 << CODEC_RSPL_ADC_TRIG_SEL_SHIFT)
#define CODEC_RSPL_ADC_TRIG_SEL(n)     BITFIELD_VAL(CODEC_RSPL_ADC_TRIG_SEL, n) 
#define CODEC_RSPL_ADC_PHASE_UPD        (1<<8)      // 1 will update phase
#define CODEC_RSPL_ADC_DUAL_CH            (1<<7)        // If 0, single ch resample;  only ch0 used
#define CODEC_RSPL_ADC_EN                       (1<<6)
#define CODEC_RSPL_DAC_TRIG_SEL_SHIFT       4       // 0: no trigger;  1: gpio trigger;  2: bt trigger;
#define CODEC_RSPL_DAC_TRIG_SEL_MASK        (0x3 << CODEC_RSPL_DAC_TRIG_SEL_SHIFT)
#define CODEC_RSPL_DAC_TRIG_SEL             BITFIELD_VAL(CODEC_RSPL_DAC_TRIG_SEL, n) 
#define CODEC_RSPL_DAC_PHASE_UPD        (1<<3)      // 1 will update phase
#define CODEC_RSPL_DAC_R_EN                   (1<<2)
#define CODEC_RSPL_DAC_L_EN                   (1<<1)
#define CODEC_RSPL_DAC_EN                       (1<<0)

// REG00E8
#define CODEC_RSPL_ADC_PHASE_INC_SHIFT      0       // Format 2.30
#define CODEC_RSPL_ADC_PHASE_INC_MASK       (0xFFFFFFFF << CODEC_RSPL_ADC_PHASE_INC_SHIFT)
#define CODEC_RSPL_ADC_PHASE_INC(n)         BITFIELD_VAL(CODEC_RSPL_ADC_PHASE_INC, n) 

// REG00EC
#define CODEC_RSPL_DAC_PHASE_INC_SHIFT      0       // Format 2.30
#define CODEC_RSPL_DAC_PHASE_INC_MASK       (0xFFFFFFFF << CODEC_RSPL_ADC_PHASE_INC_SHIFT)
#define CODEC_RSPL_DAC_PHASE_INC(n)         BITFIELD_VAL(CODEC_RSPL_DAC_PHASE_INC, n) 

// REG00F0
#define CODEC_STREAM_CH0_EN             (1<<0)      // 1: used for dacL upsample fir
#define CODEC_STREAM_CH1_EN             (1<<1)      // 1: used for dacR upsample fir
#define CODEC_STREAM_CH2_EN             (1<<2)      // 1: used for adc_ch0 downsample fir
#define CODEC_STREAM_CH3_EN             (1<<3)      // 1: used for adc_ch1 downsample fir
#define CODEC_FIR_CH0_EN                    (1<<4)      // 1: used for memory to memory fir
#define CODEC_FIR_CH1_EN                    (1<<5)
#define CODEC_FIR_CH2_EN                    (1<<6)
#define CODEC_FIR_CH3_EN                    (1<<7)
#define CODEC_FIR_CH0_UPSAMPLE                    (1<<8)        // 1: use for fir upsample
#define CODEC_FIR_CH1_UPSAMPLE                    (1<<9)
#define CODEC_FIR_CH2_UPSAMPLE                    (1<<10)
#define CODEC_FIR_CH3_UPSAMPLE                    (1<<11)
#define CODEC_FIR0_MODE                    (1<<12)              // 1: 512 fir mode; 0: 2 256 fir mode
#define CODEC_FIR1_MODE                    (1<<13)
#define CODEC_DMACTRL_FIR_RX                    (1<<14)
#define CODEC_DMACTRL_FIR_TX                    (1<<15)

// REG00F4
#define CODEC_FIR_CH0_ACCESS_OFFSET_SHIFT       0
#define CODEC_FIR_CH0_ACCESS_OFFSET_MASK       (0x7 << CODEC_FIR_CH0_ACCESS_OFFSET_SHIFT)
#define CODEC_FIR_CH0_ACCESS_OFFSET(n)      BITFIELD_VAL(CODEC_FIR_CH0_ACCESS_OFFSET, n) 
#define CODEC_FIR_CH1_ACCESS_OFFSET_SHIFT       3
#define CODEC_FIR_CH1_ACCESS_OFFSET_MASK       (0x7 << CODEC_FIR_CH1_ACCESS_OFFSET_SHIFT)
#define CODEC_FIR_CH1_ACCESS_OFFSET(n)      BITFIELD_VAL(CODEC_FIR_CH1_ACCESS_OFFSET, n) 
#define CODEC_FIR_CH2_ACCESS_OFFSET_SHIFT       6
#define CODEC_FIR_CH2_ACCESS_OFFSET_MASK       (0x7 << CODEC_FIR_CH2_ACCESS_OFFSET_SHIFT)
#define CODEC_FIR_CH2_ACCESS_OFFSET(n)      BITFIELD_VAL(CODEC_FIR_CH2_ACCESS_OFFSET, n) 
#define CODEC_FIR_CH3_ACCESS_OFFSET_SHIFT       7
#define CODEC_FIR_CH3_ACCESS_OFFSET_MASK       (0x7 << CODEC_FIR_CH3_ACCESS_OFFSET_SHIFT)
#define CODEC_FIR_CH3_ACCESS_OFFSET(n)      BITFIELD_VAL(CODEC_FIR_CH3_ACCESS_OFFSET, n) 

// REG00F8
#define CODEC_CH0_MODE              (1<<0)              // 0: stream mode; 1: fir mode
#define CODEC_CH0_FIR_MODE_SHIFT        1       // fir operation mode;  0: normal fir;   1: synthesis1;  2: synthesis2;
#define CODEC_CH0_FIR_MODE_MASK         (0x3 << CODEC_CH0_FIR_MODE_SHIFT)
#define CODEC_CH0_FIR_MODE(n)           BITFIELD_VAL(CODEC_CH0_FIR_MODE, n)
#define CODEC_CH0_FIR_ORDER_SHIFT        3
#define CODEC_CH0_FIR_ORDER_MASK         (0x1FF << CODEC_CH0_FIR_ORDER_SHIFT)
#define CODEC_CH0_FIR_ORDER(n)           BITFIELD_VAL(CODEC_CH0_FIR_ORDER, n)
#define CODEC_CH0_FIR_SAMPLE_START_SHIFT        12
#define CODEC_CH0_FIR_SAMPLE_START_MASK         (0x1FF << CODEC_CH0_FIR_SAMPLE_START_SHIFT)
#define CODEC_CH0_FIR_SAMPLE_START(n)           BITFIELD_VAL(CODEC_CH0_FIR_SAMPLE_START, n)
#define CODEC_CH0_FIR_NUM_SHIFT        21
#define CODEC_CH0_FIR_NUM_MASK         (0x1FF << CODEC_CH0_FIR_NUM_SHIFT)
#define CODEC_CH0_FIR_NUM(n)           BITFIELD_VAL(CODEC_CH0_FIR_NUM, n)

// REG00FC
#define CODEC_CH0_FIR_RESULT_BASE_ADDR_SHIFT        0
#define CODEC_CH0_FIR_RESULT_BASE_ADDR_MASK         (0x1ff << CODEC_CH0_FIR_RESULT_BASE_ADDR_SHIFT)
#define CODEC_CH0_FIR_RESULT_BASE_ADDR(n)       BITFIELD_VAL(CODEC_CH0_FIR_RESULT_BASE_ADDR, n)
#define CODEC_CH0_FIR_SLIDE_OFFSET_SHIFT        9
#define CODEC_CH0_FIR_SLIDE_OFFSET_MASK         (0x1f << CODEC_CH0_FIR_SLIDE_OFFSET_SHIFT)
#define CODEC_CH0_FIR_SLIDE_OFFSET(n)       BITFIELD_VAL(CODEC_CH0_FIR_SLIDE_OFFSET, n)
#define CODEC_CH0_FIR_BURST_LENGTH_SHIFT        14
#define CODEC_CH0_FIR_BURST_LENGTH_MASK         (0x1f << CODEC_CH0_FIR_BURST_LENGTH_SHIFT)
#define CODEC_CH0_FIR_BURST_LENGTH(n)       BITFIELD_VAL(CODEC_CH0_FIR_BURST_LENGTH, n)
#define CODEC_CH0_FIR_GAIN_SEL_SHIFT        19
#define CODEC_CH0_FIR_GAIN_SEL_MASK         (0xf << CODEC_CH0_FIR_GAIN_SEL_SHIFT)
#define CODEC_CH0_FIR_GAIN_SEL(n)       BITFIELD_VAL(CODEC_CH0_FIR_GAIN_SEL, n)
#define CODEC_CH0_FIR_LOOP_NUM_SHIFT        23
#define CODEC_CH0_FIR_LOOP_NUM_MASK         (0xff << CODEC_CH0_FIR_LOOP_NUM_SHIFT)
#define CODEC_CH0_FIR_LOOP_NUM(n)       BITFIELD_VAL(CODEC_CH0_FIR_LOOP_NUM, n)
#define CODEC_CH0_FIR_DO_REMAP          (1<<31)

// REG0100
#define CODEC_CH1_MODE              (1<<0)
#define CODEC_CH1_FIR_MODE_SHIFT        1
#define CODEC_CH1_FIR_MODE_MASK         (0x3 << CODEC_CH1_FIR_MODE_SHIFT)
#define CODEC_CH1_FIR_MODE(n)           BITFIELD_VAL(CODEC_CH1_FIR_MODE, n)
#define CODEC_CH1_FIR_ORDER_SHIFT        3
#define CODEC_CH1_FIR_ORDER_MASK         (0x1FF << CODEC_CH1_FIR_ORDER_SHIFT)
#define CODEC_CH1_FIR_ORDER(n)           BITFIELD_VAL(CODEC_CH1_FIR_ORDER, n)
#define CODEC_CH1_FIR_SAMPLE_START_SHIFT        12
#define CODEC_CH1_FIR_SAMPLE_START_MASK         (0x1FF << CODEC_CH1_FIR_SAMPLE_START_SHIFT)
#define CODEC_CH1_FIR_SAMPLE_START(n)           BITFIELD_VAL(CODEC_CH1_FIR_SAMPLE_START, n)
#define CODEC_CH1_FIR_NUM_SHIFT        21
#define CODEC_CH1_FIR_NUM_MASK         (0x1FF << CODEC_CH1_FIR_NUM_SHIFT)
#define CODEC_CH1_FIR_NUM(n)           BITFIELD_VAL(CODEC_CH1_FIR_NUM, n)

// REG0104
#define CODEC_CH1_FIR_RESULT_BASE_ADDR_SHIFT        0
#define CODEC_CH1_FIR_RESULT_BASE_ADDR_MASK         (0x1ff << CODEC_CH1_FIR_RESULT_BASE_ADDR_SHIFT)
#define CODEC_CH1_FIR_RESULT_BASE_ADDR(n)       BITFIELD_VAL(CODEC_CH1_FIR_RESULT_BASE_ADDR, n)
#define CODEC_CH1_FIR_SLIDE_OFFSET_SHIFT        9
#define CODEC_CH1_FIR_SLIDE_OFFSET_MASK         (0x1f << CODEC_CH1_FIR_SLIDE_OFFSET_SHIFT)
#define CODEC_CH1_FIR_SLIDE_OFFSET(n)       BITFIELD_VAL(CODEC_CH1_FIR_SLIDE_OFFSET, n)
#define CODEC_CH1_FIR_BURST_LENGTH_SHIFT        14
#define CODEC_CH1_FIR_BURST_LENGTH_MASK         (0x1f << CODEC_CH1_FIR_BURST_LENGTH_SHIFT)
#define CODEC_CH1_FIR_BURST_LENGTH(n)       BITFIELD_VAL(CODEC_CH1_FIR_BURST_LENGTH, n)
#define CODEC_CH1_FIR_GAIN_SEL_SHIFT        19
#define CODEC_CH1_FIR_GAIN_SEL_MASK         (0xf << CODEC_CH1_FIR_GAIN_SEL_SHIFT)
#define CODEC_CH1_FIR_GAIN_SEL(n)       BITFIELD_VAL(CODEC_CH1_FIR_GAIN_SEL, n)
#define CODEC_CH1_FIR_LOOP_NUM_SHIFT        23
#define CODEC_CH1_FIR_LOOP_NUM_MASK         (0xff << CODEC_CH1_FIR_LOOP_NUM_SHIFT)
#define CODEC_CH1_FIR_LOOP_NUM(n)       BITFIELD_VAL(CODEC_CH1_FIR_LOOP_NUM, n)
#define CODEC_CH1_FIR_DO_REMAP          (1<<31)

// REG0108
#define CODEC_CH2_MODE              (1<<0)
#define CODEC_CH2_FIR_MODE_SHIFT        1
#define CODEC_CH2_FIR_MODE_MASK         (0x3 << CODEC_CH2_FIR_MODE_SHIFT)
#define CODEC_CH2_FIR_MODE(n)           BITFIELD_VAL(CODEC_CH2_FIR_MODE, n)
#define CODEC_CH2_FIR_ORDER_SHIFT        3
#define CODEC_CH2_FIR_ORDER_MASK         (0x1FF << CODEC_CH2_FIR_ORDER_SHIFT)
#define CODEC_CH2_FIR_ORDER(n)           BITFIELD_VAL(CODEC_CH2_FIR_ORDER, n)
#define CODEC_CH2_FIR_SAMPLE_START_SHIFT        12
#define CODEC_CH2_FIR_SAMPLE_START_MASK         (0x1FF << CODEC_CH2_FIR_SAMPLE_START_SHIFT)
#define CODEC_CH2_FIR_SAMPLE_START(n)           BITFIELD_VAL(CODEC_CH2_FIR_SAMPLE_START, n)
#define CODEC_CH2_FIR_NUM_SHIFT        21
#define CODEC_CH2_FIR_NUM_MASK         (0x1FF << CODEC_CH2_FIR_NUM_SHIFT)
#define CODEC_CH2_FIR_NUM(n)           BITFIELD_VAL(CODEC_CH2_FIR_NUM, n)

// REG010C
#define CODEC_CH2_FIR_RESULT_BASE_ADDR_SHIFT        0
#define CODEC_CH2_FIR_RESULT_BASE_ADDR_MASK         (0x1ff << CODEC_CH2_FIR_RESULT_BASE_ADDR_SHIFT)
#define CODEC_CH2_FIR_RESULT_BASE_ADDR(n)       BITFIELD_VAL(CODEC_CH2_FIR_RESULT_BASE_ADDR, n)
#define CODEC_CH2_FIR_SLIDE_OFFSET_SHIFT        9
#define CODEC_CH2_FIR_SLIDE_OFFSET_MASK         (0x1f << CODEC_CH2_FIR_SLIDE_OFFSET_SHIFT)
#define CODEC_CH2_FIR_SLIDE_OFFSET(n)       BITFIELD_VAL(CODEC_CH2_FIR_SLIDE_OFFSET, n)
#define CODEC_CH2_FIR_BURST_LENGTH_SHIFT        14
#define CODEC_CH2_FIR_BURST_LENGTH_MASK         (0x1f << CODEC_CH2_FIR_BURST_LENGTH_SHIFT)
#define CODEC_CH2_FIR_BURST_LENGTH(n)       BITFIELD_VAL(CODEC_CH2_FIR_BURST_LENGTH, n)
#define CODEC_CH2_FIR_GAIN_SEL_SHIFT        19
#define CODEC_CH2_FIR_GAIN_SEL_MASK         (0xf << CODEC_CH2_FIR_GAIN_SEL_SHIFT)
#define CODEC_CH2_FIR_GAIN_SEL(n)       BITFIELD_VAL(CODEC_CH2_FIR_GAIN_SEL, n)
#define CODEC_CH2_FIR_LOOP_NUM_SHIFT        23
#define CODEC_CH2_FIR_LOOP_NUM_MASK         (0xff << CODEC_CH2_FIR_LOOP_NUM_SHIFT)
#define CODEC_CH2_FIR_LOOP_NUM(n)       BITFIELD_VAL(CODEC_CH2_FIR_LOOP_NUM, n)
#define CODEC_CH2_FIR_DO_REMAP          (1<<31)

// REG0110
#define CODEC_CH3_MODE              (1<<0)
#define CODEC_CH3_FIR_MODE_SHIFT        1
#define CODEC_CH3_FIR_MODE_MASK         (0x3 << CODEC_CH3_FIR_MODE_SHIFT)
#define CODEC_CH3_FIR_MODE(n)           BITFIELD_VAL(CODEC_CH3_FIR_MODE, n)
#define CODEC_CH3_FIR_ORDER_SHIFT        3
#define CODEC_CH3_FIR_ORDER_MASK         (0x1FF << CODEC_CH3_FIR_ORDER_SHIFT)
#define CODEC_CH3_FIR_ORDER(n)           BITFIELD_VAL(CODEC_CH3_FIR_ORDER, n)
#define CODEC_CH3_FIR_SAMPLE_START_SHIFT        12
#define CODEC_CH3_FIR_SAMPLE_START_MASK         (0x1FF << CODEC_CH3_FIR_SAMPLE_START_SHIFT)
#define CODEC_CH3_FIR_SAMPLE_START(n)           BITFIELD_VAL(CODEC_CH3_FIR_SAMPLE_START, n)
#define CODEC_CH3_FIR_NUM_SHIFT        21
#define CODEC_CH3_FIR_NUM_MASK         (0x1FF << CODEC_CH3_FIR_NUM_SHIFT)
#define CODEC_CH3_FIR_NUM(n)           BITFIELD_VAL(CODEC_CH3_FIR_NUM, n)

// REG0114
#define CODEC_CH3_FIR_RESULT_BASE_ADDR_SHIFT        0
#define CODEC_CH3_FIR_RESULT_BASE_ADDR_MASK         (0x1ff << CODEC_CH3_FIR_RESULT_BASE_ADDR_SHIFT)
#define CODEC_CH3_FIR_RESULT_BASE_ADDR(n)       BITFIELD_VAL(CODEC_CH3_FIR_RESULT_BASE_ADDR, n)
#define CODEC_CH3_FIR_SLIDE_OFFSET_SHIFT        9
#define CODEC_CH3_FIR_SLIDE_OFFSET_MASK         (0x1f << CODEC_CH3_FIR_SLIDE_OFFSET_SHIFT)
#define CODEC_CH3_FIR_SLIDE_OFFSET(n)       BITFIELD_VAL(CODEC_CH3_FIR_SLIDE_OFFSET, n)
#define CODEC_CH3_FIR_BURST_LENGTH_SHIFT        14
#define CODEC_CH3_FIR_BURST_LENGTH_MASK         (0x1f << CODEC_CH3_FIR_BURST_LENGTH_SHIFT)
#define CODEC_CH3_FIR_BURST_LENGTH(n)       BITFIELD_VAL(CODEC_CH3_FIR_BURST_LENGTH, n)
#define CODEC_CH3_FIR_GAIN_SEL_SHIFT        19
#define CODEC_CH3_FIR_GAIN_SEL_MASK         (0xf << CODEC_CH3_FIR_GAIN_SEL_SHIFT)
#define CODEC_CH3_FIR_GAIN_SEL(n)       BITFIELD_VAL(CODEC_CH3_FIR_GAIN_SEL, n)
#define CODEC_CH3_FIR_LOOP_NUM_SHIFT        23
#define CODEC_CH3_FIR_LOOP_NUM_MASK         (0xff << CODEC_CH3_FIR_LOOP_NUM_SHIFT)
#define CODEC_CH3_FIR_LOOP_NUM(n)       BITFIELD_VAL(CODEC_CH3_FIR_LOOP_NUM, n)
#define CODEC_CH3_FIR_DO_REMAP          (1<<31)

// REG0118
#define CODEC_CLASSG_EN                         (1<<0)          // classg enable
#define CODEC_CLASSG_QUICK_DOWN         (1<<1)          // 1: go to the step directly
#define CODEC_CLASSG_STEP_4_3N                   (1<<2)              // 1: 4 step;  0: 3 step
#define CODEC_CLASSG_LR                     (1<<3)              // 1: classg for L/R two ch;  0: only for L ch
#define CODEC_CLASSG_WINDOW_SHIFT       4
#define CODEC_CLASSG_WINDOW_MASK        (0xfff << CODEC_CLASSG_WINDOW_SHIFT)
#define CODEC_CLASSG_WINDOW(n)          BITFIELD_VAL(CODEC_CLASSG_WINDOW, n)

// REG011C
#define CODEC_CLASSG_THD0_SHIFT         0       // min threshold
#define CODEC_CLASSG_THD0_MASK          (0xff << CODEC_CLASSG_THD0_SHIFT)
#define CODEC_CLASSG_THD0(n)            BITFIELD_VAL(CODEC_CLASSG_THD0, n)
#define CODEC_CLASSG_THD1_SHIFT         8
#define CODEC_CLASSG_THD1_MASK          (0xff << CODEC_CLASSG_THD1_SHIFT)
#define CODEC_CLASSG_THD1(n)            BITFIELD_VAL(CODEC_CLASSG_THD1, n)
#define CODEC_CLASSG_THD2_SHIFT         16      // max threshold
#define CODEC_CLASSG_THD2_MASK          (0xff << CODEC_CLASSG_THD2_SHIFT)
#define CODEC_CLASSG_THD2(n)            BITFIELD_VAL(CODEC_CLASSG_THD2, n)

// REG0120 ...
#define CODEC_DRE_GAIN_BITS         14

#endif

