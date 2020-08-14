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
#ifdef CHIP_HAS_CP

#include "cmsis_nvic.h"
#include "plat_types.h"
#include "plat_addr_map.h"
#include "hal_cache.h"
#include "hal_location.h"

// The vector table must be aligned to NVIC_NUM_VECTORS-word boundary, rounding up to the next power of two
// -- 0x100 for 33~64 vectors, and 0x200 for 65~128 vectors
#if defined(ROM_BUILD) || defined(PROGRAMMER) || (RAMCP_SIZE <= 0)
#define VECTOR_LOC_CP                           ALIGNED(0x200)
#else
#define VECTOR_LOC_CP                           __attribute__((section(".vector_table_cp")))
#endif

#define FAULT_HANDLER_CP                        __attribute__((weak,alias("NVIC_default_handler_cp")))

static uint32_t VECTOR_LOC_CP vector_table_cp[NVIC_NUM_VECTORS];

static void NAKED NVIC_default_handler_cp(void)
{
    do { asm volatile("nop \n nop \n nop \n nop"); } while (1);
}

void FAULT_HANDLER_CP Reset_Handler_cp(void);
void FAULT_HANDLER_CP NMI_Handler_cp(void);
void FAULT_HANDLER_CP HardFault_Handler_cp(void);
void FAULT_HANDLER_CP MemManage_Handler_cp(void);
void FAULT_HANDLER_CP BusFault_Handler_cp(void);
void FAULT_HANDLER_CP UsageFault_Handler_cp(void);
void FAULT_HANDLER_CP SVC_Handler_cp(void);
void FAULT_HANDLER_CP DebugMon_Handler_cp(void);
void FAULT_HANDLER_CP PendSV_Handler_cp(void);
void FAULT_HANDLER_CP SysTick_Handler_cp(void);

static const uint32_t fault_handlers_cp[NVIC_USER_IRQ_OFFSET] = {
    (uint32_t)0,
    (uint32_t)Reset_Handler_cp,
    (uint32_t)NMI_Handler_cp,
    (uint32_t)HardFault_Handler_cp,
    (uint32_t)MemManage_Handler_cp,
    (uint32_t)BusFault_Handler_cp,
    (uint32_t)UsageFault_Handler_cp,
    (uint32_t)NVIC_default_handler_cp,
    (uint32_t)NVIC_default_handler_cp,
    (uint32_t)NVIC_default_handler_cp,
    (uint32_t)NVIC_default_handler_cp,
    (uint32_t)SVC_Handler_cp,
    (uint32_t)DebugMon_Handler_cp,
    (uint32_t)NVIC_default_handler_cp,
    (uint32_t)PendSV_Handler_cp,
    (uint32_t)SysTick_Handler_cp,
};

void NVIC_InitVectors_cp(void)
{
    int i;

    for (i = 0; i < NVIC_NUM_VECTORS; i++) {
        vector_table_cp[i] = (i < ARRAY_SIZE(fault_handlers_cp)) ?
            fault_handlers_cp[i] : (uint32_t)NVIC_default_handler_cp;
    }

    SCB->VTOR = (uint32_t)vector_table_cp;
}

void SystemInit_cp(void)
{
#if (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2) |                 /* set CP10 Full Access */
                   (3UL << 11*2)  );               /* set CP11 Full Access */
#endif

    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
#ifdef UNALIGNED_ACCESS
    SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk;
#else
    SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
#endif
#ifdef USAGE_FAULT
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
    NVIC_SetPriority(UsageFault_IRQn, IRQ_PRIORITY_REALTIME);
#else
    SCB->SHCSR &= ~SCB_SHCSR_USGFAULTENA_Msk;
#endif
#ifdef BUS_FAULT
    SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;
    NVIC_SetPriority(BusFault_IRQn, IRQ_PRIORITY_REALTIME);
#else
    SCB->SHCSR &= ~SCB_SHCSR_BUSFAULTENA_Msk;
#endif
#ifdef MEM_FAULT
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
    NVIC_SetPriority(MemoryManagement_IRQn, IRQ_PRIORITY_REALTIME);
#else
    SCB->SHCSR &= ~SCB_SHCSR_MEMFAULTENA_Msk;
#endif
}

extern uint32_t __cp_text_sram_start_flash__[];
extern uint32_t __cp_text_sram_start[];
extern uint32_t __cp_text_sram_end[];
extern uint32_t __cp_data_sram_start_flash__[];
extern uint32_t __cp_data_sram_start[];
extern uint32_t __cp_data_sram_end[];
extern uint32_t __cp_bss_sram_start[];
extern uint32_t __cp_bss_sram_end[];

void NAKED system_cp_reset_handler(void)
{
    asm volatile (
        "movs r4, 0 \n"
        "mov r5, r4 \n"
        "mov r6, r4 \n"
        "mov r7, r4 \n"
        "mov r8, r4 \n"
        "mov r9, r4 \n"
        "mov r10, r4 \n"
        "mov r11, r4 \n"
        "mov r12, r4 \n"
#if !defined(__SOFTFP__) && defined(__ARM_FP) && (__ARM_FP >= 4)
        "ldr.w r0, =0xE000ED88 \n"
        "ldr r1, [r0] \n"
        "orr r1, r1, #(0xF << 20) \n"
        "str r1, [r0] \n"
        "dsb \n"
        "isb \n"
        "vmov s0, s1, r4, r5 \n"
        "vmov s2, s3, r4, r5 \n"
        "vmov s4, s5, r4, r5 \n"
        "vmov s6, s7, r4, r5 \n"
        "vmov s8, s9, r4, r5 \n"
        "vmov s10, s11, r4, r5 \n"
        "vmov s12, s13, r4, r5 \n"
        "vmov s14, s15, r4, r5 \n"
        "vmov s16, s17, r4, r5 \n"
        "vmov s18, s19, r4, r5 \n"
        "vmov s20, s21, r4, r5 \n"
        "vmov s22, s23, r4, r5 \n"
        "vmov s24, s25, r4, r5 \n"
        "vmov s26, s27, r4, r5 \n"
        "vmov s28, s29, r4, r5 \n"
        "vmov s30, s31, r4, r5 \n"
#endif
        "bl hal_cmu_cp_get_entry_addr \n"
        "blx r0 \n"
    );
}

void system_cp_init(int load_code)
{
    NVIC_InitVectors_cp();

    // Enable icache
    hal_cachecp_enable(HAL_CACHE_ID_I_CACHE);
    // Enable dcache
    hal_cachecp_enable(HAL_CACHE_ID_D_CACHE);

    SystemInit_cp();

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
    uint32_t *dst;
    uint32_t *src;

    if (load_code) {
        dst = __cp_text_sram_start;
        src = __cp_text_sram_start_flash__;
        for (; dst < __cp_text_sram_end; dst++, src++) {
            *dst = *src;
        }
    }

    dst = __cp_data_sram_start;
    src = __cp_data_sram_start_flash__;
    for (; dst < __cp_data_sram_end; dst++, src++) {
        *dst = *src;
    }

    dst = __cp_bss_sram_start;
    for (; dst < __cp_bss_sram_end; dst++) {
        *dst = 0;
    }
#endif
}

void system_cp_term(void)
{
    // Disable dcache
    hal_cachecp_disable(HAL_CACHE_ID_D_CACHE);
    // Disable icache
    hal_cachecp_disable(HAL_CACHE_ID_I_CACHE);
}

#endif

