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
#ifndef __HAL_LOCATION_H__
#define __HAL_LOCATION_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_SEC_DEF_A(s, d, n)          __attribute__((section(#s #d #n))) n
#define HAL_SEC_DEF(section, name)      HAL_SEC_DEF_A(section, ., name)
#define HAL_SEC_LOC(section)            HAL_SEC_DEF_A(section, , )

#if defined(__GNUC__) && !(defined(ROM_BUILD) || defined(PROGRAMMER))

#define BOOT_TEXT_SRAM_LOC              HAL_SEC_LOC(.boot_text_sram)
#define BOOT_TEXT_SRAM_DEF(n)           HAL_SEC_DEF(.boot_text_sram, n)
#define BOOT_TEXT_FLASH_LOC             HAL_SEC_LOC(.boot_text_flash)
#define BOOT_TEXT_FLASH_DEF(n)          HAL_SEC_DEF(.boot_text_flash, n)
#define BOOT_RODATA_LOC                 HAL_SEC_LOC(.boot_rodata)
#define BOOT_RODATA_DEF(n)              HAL_SEC_DEF(.boot_rodata, n)
#define BOOT_DATA_LOC                   HAL_SEC_LOC(.boot_data)
#define BOOT_DATA_DEF(n)                HAL_SEC_DEF(.boot_data, n)
#define BOOT_BSS_LOC                    HAL_SEC_LOC(.boot_bss)
#define BOOT_BSS_DEF(n)                 HAL_SEC_DEF(.boot_bss, n)

#define SRAM_TEXT_LOC                   HAL_SEC_LOC(.sram_text)
#define SRAM_TEXT_DEF(n)                HAL_SEC_DEF(.sram_text, n)
#define SRAM_DATA_LOC                   HAL_SEC_LOC(.sram_data)
#define SRAM_DATA_DEF(n)                HAL_SEC_DEF(.sram_data, n)
#define SRAM_STACK_LOC                  ALIGNED(8) HAL_SEC_LOC(.sram_data)
#define SRAM_STACK_DEF(n)               ALIGNED(8) HAL_SEC_LOC(.sram_data, n)
#define SRAM_BSS_LOC                    HAL_SEC_LOC(.sram_bss)
#define SRAM_BSS_DEF(n)                 HAL_SEC_DEF(.sram_bss, n)

#define FRAM_TEXT_LOC                   HAL_SEC_LOC(.fast_text_sram)
#define FRAM_TEXT_DEF(n)                HAL_SEC_DEF(.fast_text_sram, n)

#define CP_TEXT_SRAM_LOC                HAL_SEC_LOC(.cp_text_sram)
#define CP_TEXT_SRAM_DEF(n)             HAL_SEC_DEF(.cp_text_sram, n)
#define CP_DATA_LOC                     HAL_SEC_LOC(.cp_data)
#define CP_DATA_DEF(n)                  HAL_SEC_DEF(.cp_data, n)
#define CP_BSS_LOC                      HAL_SEC_LOC(.cp_bss)
#define CP_BSS_DEF(n)                   HAL_SEC_DEF(.cp_bss, n)

#define FLASH_TEXT_LOC                  HAL_SEC_LOC(.flash_text)
#define FLASH_TEXT_DEF(n)               HAL_SEC_DEF(.flash_text, n)
#define FLASH_RODATA_LOC                HAL_SEC_LOC(.flash_rodata)
#define FLASH_RODATA_DEF(n)             HAL_SEC_DEF(.flash_rodata, n)

#else

#define BOOT_TEXT_SRAM_LOC
#define BOOT_TEXT_SRAM_DEF(n)           n
#if defined(__GNUC__) && defined(PROGRAMMER_INFLASH)
#define BOOT_TEXT_FLASH_LOC             HAL_SEC_LOC(.boot_text_flash)
#define BOOT_TEXT_FLASH_DEF(n)          HAL_SEC_DEF(.boot_text_flash, n)
#else
#define BOOT_TEXT_FLASH_LOC
#define BOOT_TEXT_FLASH_DEF(n)          n
#endif
#define BOOT_RODATA_LOC
#define BOOT_RODATA_DEF(n)              n
#define BOOT_DATA_LOC
#define BOOT_DATA_DEF(n)                n
#define BOOT_BSS_LOC
#define BOOT_BSS_DEF(n)                 n

#define SRAM_TEXT_LOC
#define SRAM_TEXT_DEF(n)                n
#define SRAM_DATA_LOC
#define SRAM_DATA_DEF(n)                n
#define SRAM_STACK_LOC
#define SRAM_STACK_DEF(n)               n
#define SRAM_BSS_LOC
#define SRAM_BSS_DEF(n)                 n

#define FRAM_TEXT_LOC
#define FRAM_TEXT_DEF(n)                n

#define CP_TEXT_SRAM_LOC
#define CP_TEXT_SRAM_DEF(n)             n
#define CP_DATA_LOC
#define CP_DATA_DEF(n)                  n
#define CP_BSS_LOC
#define CP_BSS_DEF(n)                   n

#define FLASH_TEXT_LOC
#define FLASH_TEXT_DEF(n)               n
#define FLASH_RODATA_LOC
#define FLASH_RODATA_DEF(n)             n

#endif // !__GNUC__ || ROM_BUILD || PROGRAMMER

#ifdef __cplusplus
}
#endif

#endif
