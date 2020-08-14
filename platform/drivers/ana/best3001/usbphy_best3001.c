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
#include "usbphy.h"
//#include "reg_usbphy_best3001.h"
#include "cmsis_nvic.h"
#include "hal_spi.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "pmu.h"
#include "tgt_hardware.h"

#ifdef CHIP_HAS_USBPHY

#define USBPHY_READ_CMD(r)                  ((1 << 24) | (((r) & 0xFF) << 16))
#define USBPHY_WRITE_CMD(r, v)              ((((r) & 0xFF) << 16) | ((v) & 0xFFFF))
#define USBPHY_READ_VAL(v)                  ((v) & 0xFFFF)

void usbphy_open(void)
{
    unsigned short val_01, val;

#ifdef USB_HIGH_SPEED

    usbphy_read(0x01, &val_01);
    // cdr_clk polariy=0 (stable for vcore 1.3V)
    val_01 &= ~((1 << 0) | (1 << 4) | (1 << 11));
    usbphy_write(0x01, val_01);
    val_01 |= (1 << 1) | (1 << 2) | (1 << 13);
    usbphy_write(0x01, val_01);

    usbphy_read(0x02, &val);
    val = (val & ~(0x1F << 10)) | (0xC << 10);
    usbphy_write(0x02, val);

    usbphy_read(0x05, &val);
    val |= (1 << 8);
    usbphy_write(0x05, val);

    // Tx off during rx
    usbphy_read(0x06, &val);
    val |= (1 << 13) | (1 << 14);
    // Set hs_ibias_trim to 7 (default is 4)
    //val = (val & ~(7 << 4)) | (7 << 4);
    
#ifdef USBPHY_REG06_VAL
	val = USBPHY_REG06_VAL;
#endif
    usbphy_write(0x06, val);

    usbphy_read(0x07, &val);
    val &= ~(1 << 8);
    usbphy_write(0x07, val);

    usbphy_read(0x08, &val);
    val = (val & ~((0xF << 0) | (0x3 << 4) | (0x3 << 6))) | (0x0 << 0) | (0x3 << 4) | (0x1 << 6);
	
#ifdef USBPHY_REG08_VAL
	val = USBPHY_REG08_VAL;
#endif
    usbphy_write(0x08, val);

    usbphy_read(0x09, &val);
    val &= ~(1 << 0);
    val |= (1 << 3);
#ifdef USBPHY_REG09_VAL
	val = USBPHY_REG09_VAL;
#endif
    usbphy_write(0x09, val);

    // Disable force clocks, and disable hs son signal
    // Enable parallel dig-to-phy interface
    usbphy_read(0x22, &val);
    val &= ~((1 << 4) | (1 << 5) | (1 << 6) | (1 << 7));
    val |= (1 << 10) | (1 << 11);
    usbphy_write(0x22, val);

    // Ignore all UTMI errors
    usbphy_write(0x12, 0x0003);

    hal_sys_timer_delay(MS_TO_TICKS(1));

    val_01 |= (1 << 0);
    usbphy_write(0x01, val_01);

#if defined(CHIP_HAS_DCO)
	// to use the DCXO-960M
    usbphy_read(0x18, &val);
    val |= 1<<2;
    usbphy_write(0x18, val);
#endif

#if defined(USBPHY_REG18_VAL)
	usbphy_write(0x18, USBPHY_REG18_VAL);
#endif
#ifdef USBPHY_REG14_VAL
	usbphy_write(0x14, USBPHY_REG14_VAL);
#endif

#ifdef USBPHY_REG22_VAL
	usbphy_write(0x22, USBPHY_REG22_VAL);
#endif

	// RX FIFO is set to 32 bits
	usbphy_read(0x13, &val);
	val &= ~(0xf<<3);
	val |= (0x4<<3);
	usbphy_write(0x13, val);

#ifdef USB_HS_LOOPBACK_TEST
    usbphy_read(0x0A, &val);
    val |= (1 << 3) | (1 << 4) | (1 << 6);
    usbphy_write(0x0A, val);
    usbphy_read(0x0B, &val);
    val &= ~((1 << 3) | (1 << 4) | (3 << 6));
    usbphy_write(0x0B, val);
    usbphy_read(0x09, &val);
    val |= (1 << 2);
    usbphy_write(0x09, val);
    usbphy_read(0x06, &val);
    val |= (1 << 12);
    usbphy_write(0x06, val);

    hal_sys_timer_delay(MS_TO_TICKS(1));

#define LOOPBACK_PATTERN                (*(volatile uint8_t *)0x20000004)

    LOOPBACK_PATTERN = 0;
    uint8_t pattern = LOOPBACK_PATTERN;

    // Set tx pattern
    usbphy_read(0x03, &val);
    val = (val & ~(0xFF << 8)) | (pattern << 8);
    usbphy_write(0x03, val);

    // Enable force clocks
    usbphy_read(0x22, &val);
    val |= (1 << 4) | (1 << 5) | (1 << 6);
    usbphy_write(0x22, val);

    // Enable tx
    usbphy_read(0x04, &val);
    val |= (1 << 1);
    usbphy_write(0x04, val);

    // Update tx pattern
    while (1) {
        if (pattern != LOOPBACK_PATTERN) {
            pattern = LOOPBACK_PATTERN;
            usbphy_read(0x03, &val);
            val = (val & ~(0xFF << 8)) | (pattern << 8);
            usbphy_write(0x03, val);
        }
    }
#endif

#else // !USB_HIGH_SPEED

    usbphy_read(0x01, &val_01);
    val_01 &= ~(1 << 0);
    usbphy_write(0x01, val_01);

    val = (1 << 13);
    usbphy_write(0x06, val);

    usbphy_read(0x07, &val);
    val &= ~(1 << 8);
    usbphy_write(0x07, val);

    val = 0;
    usbphy_write(0x08, val);

    val = 0;
    usbphy_write(0x09, val);

    val_01 |= (1 << 0);
    usbphy_write(0x01, val_01);

#endif // USB_HIGH_SPEED

}

void usbphy_sleep(void)
{
#ifdef USB_HIGH_SPEED
    uint16_t val;

    usbphy_read(0x01, &val);
    val &= ~((1 << 1) | (1 << 2) | (1 << 3));
    usbphy_write(0x01, val);
#endif
}

void usbphy_wakeup(void)
{
#ifdef USB_HIGH_SPEED
    uint16_t val;

    usbphy_read(0x01, &val);
    val |= (1 << 1) | (1 << 2) | (1 << 3);
    usbphy_write(0x01, val);
#endif
}

#ifdef CHIP_HAS_DCO
void usbphy_dco_960m_sel(uint8_t sel)
{
	uint16_t val;
	
	sel = !!sel;

	// to use the DCXO-960M
	usbphy_read(0x18, &val);
	if (sel) {
		val |= 1<<2;
//		val &= ~(1<<10);
	} else {
		val &= ~(1<<2);
//		val |= 1<<10;
	}
	usbphy_write(0x18, val);
}
#endif

void usbphy_test_set(void)
{
	uint16_t val;

	usbphy_read(0x07, &val);
	val |= 1<<15;
	usbphy_write(0x07, val);
}

void usbphy_loopback_test (void)
{
	usbphy_write(0x01, 0x70ef);
	usbphy_write(0x02, 0x3018);
	usbphy_write(0x06, 0x28ff);
	usbphy_write(0x08, 0xac70);
	usbphy_write(0x09, 0x0008);
	usbphy_write(0x12, 0x0003);
	usbphy_write(0x13, 0x0021);
	usbphy_write(0x14, 0x0fa8);
	usbphy_write(0x22, 0x3dbf);

	*((uint32_t*)0x401c0000) = 0x20000;
	*((uint32_t*)0x401c0008) = 0x803f;

	uint16_t val;
	usbphy_read(0x0a, &val);
	val |= (1<<13);
	usbphy_write(0x0a, val);

	do {
		usbphy_read(0x20, &val);
		if (val == 0x528) {
			break;
		}

		hal_sys_timer_delay(MS_TO_TICKS(1000));
	} while(1);
}


#endif

void pll_config_update (uint32_t tgt_freq)
{
    int ret;
    uint64_t PLL_cfg_val;
    uint16_t v[3];
    uint16_t val;
    uint16_t freq_en;
//    uint32_t freq = 960000000;
    //uint32_t crystal = hal_cmu_get_crystal_freq();
    uint32_t crystal = 24000000;

    //PLL_cfg_val = ((uint64_t)(1 << 28) * (tgt_freq / 2) + crystal / 2) / crystal;
    PLL_cfg_val = ((uint64_t)(1 << 28) * tgt_freq + crystal / 2) / crystal;
    v[0] = PLL_cfg_val & 0xFFFF;
    v[1] = (PLL_cfg_val >> 16) & 0xFFFF;
    v[2] = (PLL_cfg_val >> 32) & 0xFFFF;

    ret = usbphy_read(0x36, &freq_en);
    if (ret) {
        return;
    }
    freq_en &= ~(1<<14);
    ret = usbphy_write(0x36, freq_en);
    if (ret) {
        return;
    }

    ret = usbphy_write(0x39, v[0]);
    if (ret) {
        return;
    }

    ret = usbphy_write(0x40, v[1]);
    if (ret) {
        return;
    }

    ret = usbphy_read(0x37, &val);
    if (ret) {
        return;
    }

#define USBPHY_37_PLL_FREQ_34_32_SHIFT      7
#define USBPHY_37_PLL_FREQ_34_32_MASK       (0x7 << USBPHY_37_PLL_FREQ_34_32_SHIFT)
#define USBPHY_37_PLL_FREQ_34_32(n)         BITFIELD_VAL(USBPHY_37_PLL_FREQ_34_32, n)
	
    val = SET_BITFIELD(val, USBPHY_37_PLL_FREQ_34_32, v[2]);
    ret = usbphy_write(0x37, val);
    if (ret) {
        return;
    }

    freq_en |= (1<<14);
    ret = usbphy_write(0x36, freq_en);
    if (ret) {
        return;
    }

	osDelay(1);
}


