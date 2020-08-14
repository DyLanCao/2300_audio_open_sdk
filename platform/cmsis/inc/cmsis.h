/* mbed Microcontroller Library - CMSIS
 * Copyright (C) 2009-2011 ARM Limited. All rights reserved.
 *
 * A generic CMSIS include header, pulling in LPC11U24 specifics
 */

#ifndef MBED_CMSIS_H
#define MBED_CMSIS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_addr_map.h"
#include _TO_STRING(CONCAT_SUFFIX(CHIP_ID_LITERAL, h))

#define IRQ_PRIORITY_REALTIME               0
#define IRQ_PRIORITY_HIGHPLUSPLUS           1
#define IRQ_PRIORITY_HIGHPLUS               2
#define IRQ_PRIORITY_HIGH                   3
#define IRQ_PRIORITY_ABOVENORMAL            4
#define IRQ_PRIORITY_NORMAL                 5
#define IRQ_PRIORITY_BELOWNORMAL            6
#define IRQ_PRIORITY_LOW                    7

#define NVIC_USER_IRQ_OFFSET                16
#define NVIC_NUM_VECTORS                    (NVIC_USER_IRQ_OFFSET + USER_IRQn_QTY)

#ifndef __ASSEMBLER__

__STATIC_FORCEINLINE uint32_t int_lock_global(void)
{
	uint32_t pri = __get_PRIMASK();
	if ((pri & 0x1) == 0) {
		__disable_irq();
	}
	return pri;
}

__STATIC_FORCEINLINE void int_unlock_global(uint32_t pri)
{
	if ((pri & 0x1) == 0) {
		__enable_irq();
	}
}

__STATIC_FORCEINLINE uint32_t int_lock(void)
{
#ifdef INT_LOCK_EXCEPTION
    uint32_t pri = __get_BASEPRI();
    // Only allow IRQs with priority IRQ_PRIORITY_HIGHPLUSPLUS and IRQ_PRIORITY_REALTIME
    __set_BASEPRI(((IRQ_PRIORITY_HIGHPLUS << (8 - __NVIC_PRIO_BITS)) & (uint32_t)0xFFUL));
    return pri;
#else
    return int_lock_global();
#endif
}

__STATIC_FORCEINLINE void int_unlock(uint32_t pri)
{
#ifdef INT_LOCK_EXCEPTION
    __set_BASEPRI(pri);
#else
    int_unlock_global(pri);
#endif
}

__STATIC_FORCEINLINE int32_t ftoi_nearest(float f)
{
    return (f >= 0) ? (int32_t)(f + 0.5) : (int32_t)(f - 0.5);
}

void GotBaseInit(void);

int set_bool_flag(bool *flag);

void clear_bool_flag(bool *flag);

float db_to_float(int32_t db);

uint32_t get_msb_pos(uint32_t val);

uint32_t get_lsb_pos(uint32_t val);

#endif

#ifdef __cplusplus
}
#endif

#endif
