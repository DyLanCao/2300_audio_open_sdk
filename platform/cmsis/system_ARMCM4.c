/**************************************************************************//**
 * @file     system_ARMCM4.c
 * @brief    CMSIS Device System Source File for
 *           ARMCM4 Device Series
 * @version  V1.09
 * @date     27. August 2014
 *
 * @note
 *
 ******************************************************************************/
/* Copyright (c) 2011 - 2014 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/

#include "cmsis.h"
#include "hal_location.h"

/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System.
 */
void BOOT_TEXT_FLASH_LOC SystemInit (void)
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

#ifndef UNALIGNED_ACCESS

bool get_unaligned_access_status(void)
{
    return !(SCB->CCR & SCB_CCR_UNALIGN_TRP_Msk);
}

bool config_unaligned_access(bool enable)
{
    bool en;

    en = !(SCB->CCR & SCB_CCR_UNALIGN_TRP_Msk);

    if (enable) {
        SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk;
    } else {
        SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
    }

    return en;
}

#endif

// -----------------------------------------------------------
// CPU ID
// -----------------------------------------------------------

uint32_t BOOT_TEXT_SRAM_DEF(get_cpu_id) (void)
{
    return (SCB->ADR & 3);
}

