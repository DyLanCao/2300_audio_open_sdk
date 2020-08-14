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
#include "plat_types.h"
#include "plat_addr_map.h"
#include "cmsis.h"
#include "hal_norflaship.h"
#include "hal_norflash.h"
#include "hal_bootmode.h"
#include "hal_cmu.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "norflash_cfg.h"
#include "norflash_drv.h"
#include "pmu.h"
#include "string.h"

/* Demo:
 *
 *  uint8_t data[1024];
 *  hal_norflash_open(HAL_NORFLASH_ID_0, HAL_NORFLASH_SPEED_26M, 0);
 *  \/\/ hal_norflash_open(HAL_NORFLASH_ID_0, HAL_NORFLASH_SPEED_13M, HAL_NORFLASH_OP_MODE_QUAD);
 *  \/\/ hal_norflash_open(HAL_NORFLASH_ID_0, HAL_NORFLASH_SPEED_13M, HAL_NORFLASH_OP_MODE_QUAD|HAL_NORFLASH_OP_MODE_CONTINUOUS_READ);
 *  hal_norflash_erase(HAL_I2C_ID_0, 0, 4096);
 *  memset(data, 0xcc, 1024);
 *  hal_norflash_write(HAL_I2C_ID_0, 0, data, 1024);
 *  for (i = 0; i < 10; ++i) {
 *      TRACE("[0x%x] - 0x%x\n", 0x08000000 + i, *((volatile uint8_t *)(0x08000000 + i)));
 *  }
*/

#define HAL_NORFLASH_CP_ID_LEN              2

#define HAL_NORFLASH_YES                    1
#define HAL_NORFLASH_NO                     0

#define HAL_NORFLASH_ADDR_MASK              0x00FFFFFF

struct HAL_Norflash_Context {
    bool opened;
    uint8_t device_id[HAL_NORFLASH_DEVICE_ID_LEN];
#ifdef FLASH_UNIQUE_ID
    uint8_t unique_id[HAL_NORFLASH_UNIQUE_ID_LEN + HAL_NORFLASH_CP_ID_LEN];
#endif
    uint32_t total_size;
    uint32_t block_size;
    uint32_t sector_size;
    uint32_t page_size;
    enum HAL_NORFLASH_RET_T open_state;
};

static struct HAL_Norflash_Context norflash_ctx[HAL_NORFLASH_ID_NUM];

static const char * const err_not_opened = "norflash not opened";

static const struct HAL_NORFLASH_CONFIG_T norflash_cfg = {
#if defined(CHIP_BEST1400)
    .source_clk = HAL_NORFLASH_SPEED_52M,
    .speed = HAL_NORFLASH_SPEED_52M,
#else
#ifdef FPGA
    .source_clk = HAL_NORFLASH_SPEED_13M * 2,
    .speed = HAL_NORFLASH_SPEED_13M,
#elif defined(FLASH_LOW_SPEED)
    .source_clk = HAL_NORFLASH_SPEED_26M * 2,
    .speed = HAL_NORFLASH_SPEED_26M,
#elif defined(OSC_26M_X4_AUD2BB)
    .source_clk = HAL_NORFLASH_SPEED_52M * 2,
    .speed = HAL_NORFLASH_SPEED_52M,
#else
    .source_clk = HAL_NORFLASH_SPEED_104M * 2,
    .speed = HAL_NORFLASH_SPEED_104M,
#endif
#endif
    .mode = HAL_NORFLASH_OP_MODE_STAND_SPI |
            HAL_NORFLASH_OP_MODE_FAST_SPI |
            HAL_NORFLASH_OP_MODE_DUAL_OUTPUT |
            HAL_NORFLASH_OP_MODE_DUAL_IO |
            HAL_NORFLASH_OP_MODE_QUAD_OUTPUT |
            HAL_NORFLASH_OP_MODE_QUAD_IO |
            HAL_NORFLASH_OP_MODE_CONTINUOUS_READ |
            HAL_NORFLASH_OP_MODE_READ_WRAP |
            HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
            HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM |
            HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM,
    .override_config = 0,
};

#ifdef FLASH_SUSPEND
enum SUSPEND_STATE_T {
    SUSPEND_STATE_NO,
    SUSPEND_STATE_ERASE,
    SUSPEND_STATE_PROGRAM,
};

static enum SUSPEND_STATE_T suspend_state;
static uint32_t op_next_addr;
static const uint8_t *op_next_buf;
static uint32_t op_remain_len;
#endif

#ifdef FLASH_SECURITY_REGISTER
static uint32_t sec_reg_base;
static uint16_t sec_reg_size;
static uint16_t sec_reg_offset;
static uint16_t sec_reg_total_size;
static uint16_t sec_reg_pp_size;
static bool sec_reg_enabled;
#endif

enum HAL_CMU_FREQ_T hal_norflash_clk_to_cmu_freq(uint32_t clk)
{
    if (clk >= HAL_NORFLASH_SPEED_208M) {
        return HAL_CMU_FREQ_208M;
    } else if (clk >= HAL_NORFLASH_SPEED_104M) {
        return HAL_CMU_FREQ_104M;
    } else if (clk >= HAL_NORFLASH_SPEED_78M) {
        return HAL_CMU_FREQ_78M;
    } else if (clk >= HAL_NORFLASH_SPEED_52M) {
        return HAL_CMU_FREQ_52M;
    } else {
        return HAL_CMU_FREQ_26M;
    }
}

enum HAL_NORFLASH_RET_T hal_norflash_get_size(enum HAL_NORFLASH_ID_T id, uint32_t *total_size,
                                              uint32_t *block_size, uint32_t *sector_size,
                                              uint32_t *page_size)
{
    ASSERT(norflash_ctx[id].opened, err_not_opened);

    if (total_size) {
        *total_size = norflash_ctx[id].total_size;
    }
    if (block_size) {
        *block_size = norflash_ctx[id].block_size;
    }
    if (sector_size) {
        *sector_size = norflash_ctx[id].sector_size;
    }
    if (page_size) {
        *page_size = norflash_ctx[id].page_size;
    }
    return HAL_NORFLASH_OK;
}

enum HAL_NORFLASH_RET_T hal_norflash_get_boundary(enum HAL_NORFLASH_ID_T id, uint32_t address,
                                                  uint32_t *block_boundary, uint32_t *sector_boundary)
{
    ASSERT(norflash_ctx[id].opened, err_not_opened);

    static const uint32_t flash_base[] = { FLASH_BASE, FLASHX_BASE, FLASH_NC_BASE, FLASHX_NC_BASE, };
    int i;

    for (i = 0; i < ARRAY_SIZE(flash_base); i++) {
        if (flash_base[i] <= address && address < flash_base[i] + norflash_ctx[id].total_size) {
            address -= flash_base[i];
            if (block_boundary)
                *block_boundary  = flash_base[i] + (address/norflash_ctx[id].block_size)*norflash_ctx[id].block_size;
            if (sector_boundary)
                *sector_boundary = flash_base[i] + (address/norflash_ctx[id].sector_size)*norflash_ctx[id].sector_size;
            break;
        }
    }

    if (i == ARRAY_SIZE(flash_base)) {
        return HAL_NORFLASH_BAD_ADDR;
    }

    return HAL_NORFLASH_OK;
}

enum HAL_NORFLASH_RET_T hal_norflash_get_id(enum HAL_NORFLASH_ID_T id, uint8_t *value, uint32_t len)
{
    len = MIN(len, sizeof(norflash_ctx[id].device_id));

    memcpy(value, norflash_ctx[id].device_id, len);

    return HAL_NORFLASH_OK;
}

#ifdef FLASH_UNIQUE_ID
enum HAL_NORFLASH_RET_T hal_norflash_get_unique_id(enum HAL_NORFLASH_ID_T id, uint8_t *value, uint32_t len)
{
    ASSERT(norflash_ctx[id].opened, err_not_opened);

    len = MIN(len, sizeof(norflash_ctx[id].unique_id));

    memcpy(value, norflash_ctx[id].unique_id, len);

    return HAL_NORFLASH_OK;
}
#endif

#ifdef FLASH_SECURITY_REGISTER
enum HAL_NORFLASH_RET_T hal_norflash_security_register_lock(enum HAL_NORFLASH_ID_T id, uint32_t start_address, uint32_t len)
{
    uint32_t remain_len;
    int ret = 0;
    uint32_t reg_base;
    uint32_t reg_pos;
    uint32_t pos;
    uint32_t lock_size;
    uint32_t reg_id;

    ASSERT(norflash_ctx[id].opened, err_not_opened);

    if (!sec_reg_enabled) {
        return HAL_NORFLASH_BAD_OP;
    }
    // Check address and length
    if (sec_reg_total_size <= start_address) {
        return HAL_NORFLASH_BAD_ADDR;
    }
    remain_len = sec_reg_total_size - start_address;
    if (len > remain_len) {
        return HAL_NORFLASH_BAD_LEN;
    }
    // Align to register boundary
    remain_len = start_address & (sec_reg_size - 1);
    if (remain_len) {
        start_address -= remain_len;
        len += remain_len;
    }
    remain_len = len & (sec_reg_size - 1);
    if (remain_len) {
        len += sec_reg_size - remain_len;
    }

    pos = start_address;
    remain_len = len;

    reg_base = sec_reg_base;
    reg_pos = 0;
    reg_id = 0;

    norflash_pre_operation();

    while (remain_len > 0 && ret == 0) {
        if (reg_pos <= pos && pos < reg_pos + sec_reg_size) {
            // lock a register
            lock_size = sec_reg_size - (pos - reg_pos);
            ret = norflash_security_register_lock(reg_id);
            if (remain_len > lock_size) {
                remain_len -= lock_size;
            } else {
                remain_len = 0;
            }
            pos += lock_size;
        }
        reg_pos += sec_reg_size;
        reg_base += sec_reg_offset;
        reg_id++;
    }

    norflash_post_operation();

    return ret;
}

enum HAL_NORFLASH_RET_T hal_norflash_security_register_erase(enum HAL_NORFLASH_ID_T id, uint32_t start_address, uint32_t len)
{
    uint32_t remain_len;
    enum HAL_NORFLASH_RET_T ret = HAL_NORFLASH_OK;
    uint32_t reg_base;
    uint32_t reg_pos;
    uint32_t pos;
    uint32_t erase_size;

    ASSERT(norflash_ctx[id].opened, err_not_opened);

    if (!sec_reg_enabled) {
        return HAL_NORFLASH_BAD_OP;
    }
    // Check address and length
    if (sec_reg_total_size <= start_address) {
        return HAL_NORFLASH_BAD_ADDR;
    }
    remain_len = sec_reg_total_size - start_address;
    if (len > remain_len) {
        return HAL_NORFLASH_BAD_LEN;
    }
    // Align to register boundary
    remain_len = start_address & (sec_reg_size - 1);
    if (remain_len) {
        start_address -= remain_len;
        len += remain_len;
    }
    remain_len = len & (sec_reg_size - 1);
    if (remain_len) {
        len += sec_reg_size - remain_len;
    }

    pos = start_address;
    remain_len = len;

    reg_base = sec_reg_base;
    reg_pos = 0;

    norflash_pre_operation();

    while (remain_len > 0 && ret == HAL_NORFLASH_OK) {
        if (reg_pos <= pos && pos < reg_pos + sec_reg_size) {
            // erase a register
            erase_size = sec_reg_size - (pos - reg_pos);
            ret = norflash_security_register_erase(reg_base + (pos - reg_pos));
            if (remain_len > erase_size) {
                remain_len -= erase_size;
            } else {
                remain_len = 0;
            }
            pos += erase_size;
        }
        reg_pos += sec_reg_size;
        reg_base += sec_reg_offset;
    }

    norflash_post_operation();

    return ret;
}

enum HAL_NORFLASH_RET_T hal_norflash_security_register_write(enum HAL_NORFLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len)
{
    const uint8_t *current_buffer;
    uint32_t remain_len;
    enum HAL_NORFLASH_RET_T ret = HAL_NORFLASH_OK;
    uint32_t reg_base;
    uint32_t reg_pos;
    uint32_t pos;
    uint32_t write_size;
    uint32_t each_write;
    uint32_t pp_remain;

    ASSERT(norflash_ctx[id].opened, err_not_opened);

    if (!sec_reg_enabled) {
        return HAL_NORFLASH_BAD_OP;
    }
    // Check address and length
    if (sec_reg_total_size <= start_address) {
        return HAL_NORFLASH_BAD_ADDR;
    }
    remain_len = sec_reg_total_size - start_address;
    if (len > remain_len) {
        return HAL_NORFLASH_BAD_LEN;
    }

    pos = start_address;
    current_buffer = buffer;
    remain_len = len;

    reg_base = sec_reg_base;
    reg_pos = 0;

    norflash_pre_operation();

    while (remain_len > 0 && ret == HAL_NORFLASH_OK) {
        if (reg_pos <= pos && pos < reg_pos + sec_reg_size) {
            // write a register
            if (pos + remain_len <= reg_pos + sec_reg_size) {
                write_size = remain_len;
            } else {
                write_size = sec_reg_size - (pos - reg_pos);
            }
            remain_len -= write_size;
            while (write_size > 0 && ret == HAL_NORFLASH_OK) {
                if (write_size > sec_reg_pp_size) {
                    each_write = sec_reg_pp_size;
                } else {
                    each_write = write_size;
                }
                // Align to security register program page size
                pp_remain = sec_reg_pp_size - ((pos - reg_pos) & (sec_reg_pp_size - 1));
                if (each_write > pp_remain) {
                    each_write = pp_remain;
                }
                ret = norflash_security_register_write(reg_base + (pos - reg_pos), current_buffer, each_write);
                write_size -= each_write;
                pos += each_write;
                current_buffer += each_write;
            }
        }
        reg_pos += sec_reg_size;
        reg_base += sec_reg_offset;
    }

    norflash_post_operation();

    return ret;
}

enum HAL_NORFLASH_RET_T hal_norflash_security_register_read(enum HAL_NORFLASH_ID_T id, uint32_t start_address, uint8_t *buffer, uint32_t len)
{
    uint8_t *current_buffer;
    uint32_t remain_len, read_size;
    int ret = 0;
    uint32_t reg_base;
    uint32_t reg_pos;
    uint32_t pos;
#ifdef FLASH_SEC_REG_FIFO_READ
    uint32_t each_read;
#endif

    ASSERT(norflash_ctx[id].opened, err_not_opened);

    if (!sec_reg_enabled) {
        return HAL_NORFLASH_BAD_OP;
    }
    // Check address and length
    if (sec_reg_total_size <= start_address) {
        return HAL_NORFLASH_BAD_ADDR;
    }
    remain_len = sec_reg_total_size - start_address;
    if (len > remain_len) {
        return HAL_NORFLASH_BAD_LEN;
    }

    pos = start_address;
    current_buffer = buffer;
    remain_len = len;

    reg_base = sec_reg_base;
    reg_pos = 0;

    norflash_pre_operation();

#ifndef FLASH_SEC_REG_FIFO_READ
    uint32_t mode = norflash_security_register_enable_read();
#endif

    while (remain_len > 0 && ret == 0) {
        if (reg_pos <= pos && pos < reg_pos + sec_reg_size) {
            // read a register
            if (pos + remain_len <= reg_pos + sec_reg_size) {
                read_size = remain_len;
            } else {
                read_size = sec_reg_size - (pos - reg_pos);
            }
            remain_len -= read_size;
#ifdef FLASH_SEC_REG_FIFO_READ
            while (read_size > 0 && ret == 0) {
                if (read_size > NORFLASHIP_RXFIFO_SIZE) {
                    each_read = NORFLASHIP_RXFIFO_SIZE;
                } else {
                    each_read = read_size;
                }
                ret = norflash_security_register_read(reg_base + (pos - reg_pos), current_buffer, each_read);
                read_size -= each_read;
                pos += each_read;
                current_buffer += each_read;
            }
#else
            memcpy(current_buffer, (void *)(reg_base + (pos - reg_pos)), read_size);
            pos += read_size;
            current_buffer += read_size;
#endif
        }
        reg_pos += sec_reg_size;
        reg_base += sec_reg_offset;
    }

#ifndef FLASH_SEC_REG_FIFO_READ
    norflash_security_register_disable_read(mode);
#endif

    norflash_post_operation();

    return (ret ? HAL_NORFLASH_ERR : HAL_NORFLASH_OK);
}

static enum HAL_NORFLASH_RET_T hal_norflash_parse_security_register_config(void)
{
    union DRV_NORFLASH_SEC_REG_CFG_T cfg;
    uint32_t reg_cnt;

    cfg = norflash_get_security_register_config();

    if (!cfg.s.enabled) {
        return HAL_NORFLASH_BAD_OP;
    }

    if (cfg.s.base == SEC_REG_BASE_0X1000) {
        sec_reg_base = 0x1000;
    } else if (cfg.s.base == SEC_REG_BASE_0X0000) {
        sec_reg_base = 0;
    } else {
        return HAL_NORFLASH_BAD_CFG;
    }
    sec_reg_base += FLASH_NC_BASE;

    if (cfg.s.size == SEC_REG_SIZE_1024) {
        sec_reg_size = 1024;
    } else if (cfg.s.size == SEC_REG_SIZE_512) {
        sec_reg_size = 512;
    } else if (cfg.s.size == SEC_REG_SIZE_256) {
        sec_reg_size = 256;
    } else {
        return HAL_NORFLASH_BAD_CFG;
    }

    if (cfg.s.offset == SEC_REG_OFFSET_0X1000) {
        sec_reg_offset = 0x1000;
    } else if (cfg.s.offset == SEC_REG_OFFSET_0X0100) {
        sec_reg_offset = 0x0100;
    } else {
        return HAL_NORFLASH_BAD_CFG;
    }

    if (sec_reg_size > sec_reg_offset) {
        return HAL_NORFLASH_BAD_CFG;
    }

    if (cfg.s.cnt == SEC_REG_CNT_3) {
        reg_cnt = 3;
    } else if (cfg.s.cnt == SEC_REG_CNT_4) {
        reg_cnt = 4;
    } else {
        return HAL_NORFLASH_BAD_CFG;
    }

    if (cfg.s.pp == SEC_REG_PP_256) {
        sec_reg_pp_size = 256;
    } else if (cfg.s.pp == SEC_REG_PP_1024) {
        sec_reg_pp_size = 1024;
    } else {
        return HAL_NORFLASH_BAD_CFG;
    }
#if (CHIP_FLASH_CTRL_VER <= 1)
#ifdef FLASH_SEC_REG_PP_1024
    // To write more than 256 bytes on flash controller V1, SPI rate must be lowered to avoid tx FIFO underflow.
    // Otherwise, the data must be split into pieces with size no more than 256 bytes.
#else
    sec_reg_pp_size = 256;
#endif
#endif

    sec_reg_total_size = sec_reg_size * reg_cnt;

    return HAL_NORFLASH_OK;
}
#endif

static void hal_norflash_reset_timing(void)
{
    const uint32_t default_div = 8;

    // Restore default divider
    norflaship_div(default_div);
    norflash_init_sample_delay_by_div(default_div);
}

enum HAL_NORFLASH_RET_T hal_norflash_open(enum HAL_NORFLASH_ID_T id, const struct HAL_NORFLASH_CONFIG_T *cfg)
{
    struct HAL_NORFLASH_CONFIG_T norcfg;
    int result;
    uint32_t op;
    uint8_t dev_id[HAL_NORFLASH_DEVICE_ID_LEN];
    int found;

    // Place the config into ram
    memcpy(&norcfg, cfg, sizeof(norcfg));

#if (CHIP_FLASH_CTRL_VER >= 2)
    // Set the direction of 4 IO pins to output when in idle
    norflaship_set_idle_io_dir(0);
#endif

    // Reset controller timing
    hal_norflash_reset_timing();

    // Reset norflash in slow clock configuration
    norflash_reset();

    /* over write config */
    if (norcfg.override_config) {
        /* div */
        norflaship_div(norcfg.div);

        /* cmd quad */
        norflaship_cmdquad(norcfg.cmdquad?HAL_NORFLASH_YES:HAL_NORFLASH_NO);

        /* sample delay */
        norflaship_samdly(norcfg.samdly);

#if 0
        /* dummy clc */
        norflaship_dummyclc(norcfg.dummyclc);

        /* dummy clc en */
        norflaship_dummyclcen(norcfg.dummyclcen);

        /* 4 byte address */
        norflaship_addrbyte4(norcfg.byte4byteaddr);
#endif

        /* ru en */
        norflaship_ruen(norcfg.spiruen);

        /* rd en */
        norflaship_rden(norcfg.spirden);

        /* rd cmd */
        norflaship_rdcmd(norcfg.rdcmd);

        /* frd cmd */
        norflaship_frdcmd(norcfg.frdcmd);

        /* qrd cmd */
        norflaship_qrdcmd(norcfg.qrdcmd);
    }

#ifdef SIMU
#ifdef SIMU_FAST_FLASH
#define MAX_SIMU_FLASH_FREQ     HAL_CMU_FREQ_104M
#else
#define MAX_SIMU_FLASH_FREQ     HAL_CMU_FREQ_52M
#endif
    {
        enum HAL_CMU_FREQ_T source_clk;

        source_clk = hal_norflash_clk_to_cmu_freq(norcfg.source_clk);
        if (source_clk > MAX_SIMU_FLASH_FREQ) {
            source_clk = MAX_SIMU_FLASH_FREQ;
            hal_cmu_flash_set_freq(source_clk);
        }
    }
#endif

    // Get device ID
    norflash_get_id(norflash_ctx[id].device_id, sizeof(norflash_ctx[id].device_id));

    // foreach driver in array, match chip and select drv_ops
    found = norflash_match_chip(norflash_ctx[id].device_id, sizeof(norflash_ctx[id].device_id));
    if (!found) {
        result = HAL_NORFLASH_BAD_ID;
        goto _exit;
    }

    if (!norcfg.override_config) {
        // Init divider
        result = norflash_init_div(cfg);
        if (result != 0) {
            result = HAL_NORFLASH_BAD_DIV;
            goto _exit;
        }

        // Check whether divider is good
        norflash_get_id(dev_id, sizeof(dev_id));
        if (dev_id[0] != norflash_ctx[id].device_id[0] || dev_id[1] != norflash_ctx[id].device_id[1] ||
                dev_id[2] != norflash_ctx[id].device_id[2]) {
            result = HAL_NORFLASH_BAD_DIV_VERIF;
            goto _exit;
        }
    }

    norflash_get_size(&norflash_ctx[id].total_size, &norflash_ctx[id].block_size,
        &norflash_ctx[id].sector_size, &norflash_ctx[id].page_size);

#ifdef FLASH_SECURITY_REGISTER
    result = hal_norflash_parse_security_register_config();
    if (result == HAL_NORFLASH_OK) {
        sec_reg_enabled = true;
    } else if (result != HAL_NORFLASH_BAD_OP) {
        goto _exit;
    }
#endif

#ifdef FLASH_UNIQUE_ID
#if (CHIP_FLASH_CTRL_VER <= 1)
    uint32_t old_div = norflaship_get_div();
    // Slow down to avoid rx fifo overflow
    norflaship_div(8);
#endif
    norflash_get_unique_id(norflash_ctx[id].unique_id, sizeof(norflash_ctx[id].unique_id));
#if (CHIP_FLASH_CTRL_VER <= 1)
    norflaship_div(old_div);
#endif
#endif

#if (CHIP_FLASH_CTRL_VER <= 1) && !defined(FLASH_LOW_SPEED) && !defined(OSC_26M_X4_AUD2BB)
    // 1) Flash controller V2 or later
    //    No requirement on system_freq
    // 2) Flash controller V1
    //    Requirement on system_freq when running in quad mode (4-line mode):
    //      Byte Access:     flash_line_speed < 2 * system_freq
    //      Halfword Access: flash_line_speed < 4 * system_freq
    //      Word Access:     flash_line_speed < 8 * system_freq
    //    The maximum flash_line_speed is 120M in spec, and PLL_FREQ / 2 in our system.
    //    Normally it is 24.576M * 8 / 2 ~= 100M.
    //    So the safe system_freq should be larger than 50M/25M/12.5M for byte/halfword/word access.
    //    Cached access to flash is always safe, because it is always word-aligned (system_freq is never below 26M).
    //    However, uncached access (e.g., access to audio/user/factory data sections) is under risk.
    hal_sysfreq_set_min_freq(HAL_CMU_FREQ_52M);
#endif

    op = norcfg.mode;
    if (norcfg.speed >= HAL_NORFLASH_SPEED_104M) {
        op |= HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE;
    }

    // Divider will be set to normal read mode
    result = norflash_set_mode(op, true);
    if (result != 0) {
        result = HAL_NORFLASH_BAD_OP;
        goto _exit;
    }

    // -----------------------------
    // From now on, norflash_pre_operation() must be called before
    // sending any command to flash
    // -----------------------------

#if defined(CHIP_BEST1400)
    if (norcfg.dec_enable && (norcfg.dec_size > 0)) {
        norflaship_dec_saddr(norcfg.dec_addr);
        norflaship_dec_eaddr(norcfg.dec_addr + norcfg.dec_size);
        norflaship_dec_index(norcfg.dec_idx);
        norflaship_dec_enable();
    } else {
        norflaship_dec_disable();
    }
#endif

    if (!norcfg.override_config) {
        result = norflash_sample_delay_calib();
        if (result != 0) {
            result = HAL_NORFLASH_BAD_CALIB;
            goto _exit;
        }
    }

    norflash_ctx[id].opened = true;

    result = HAL_NORFLASH_OK;

_exit:
    if (result != HAL_NORFLASH_OK) {
        hal_norflash_reset_timing();
    }

    norflash_ctx[id].open_state = result;

    return result;
}

enum HAL_NORFLASH_RET_T hal_norflash_erase_chip(enum HAL_NORFLASH_ID_T id)
{
    uint32_t total_size = 0;

    ASSERT(norflash_ctx[id].opened, err_not_opened);

    total_size = norflash_ctx[id].total_size;

    return hal_norflash_erase(id, 0, total_size);
}

static enum HAL_NORFLASH_RET_T hal_norflash_erase_int(enum HAL_NORFLASH_ID_T id, uint32_t start_address, uint32_t len, int suspend)
{
    uint32_t remain_len, current_address, total_size, block_size, sector_size;
    enum HAL_NORFLASH_RET_T ret = HAL_NORFLASH_OK;

    total_size      = norflash_ctx[id].total_size;
    block_size      = norflash_ctx[id].block_size;
    sector_size     = norflash_ctx[id].sector_size;

    // Check address and length
    if (total_size <= (start_address & HAL_NORFLASH_ADDR_MASK)) {
        return HAL_NORFLASH_BAD_ADDR;
    }
    remain_len      = total_size - (start_address & HAL_NORFLASH_ADDR_MASK);
    if (len > remain_len) {
        return HAL_NORFLASH_BAD_LEN;
    }
    // Align to sector boundary
    remain_len      = start_address & (sector_size - 1);
    if (remain_len) {
        start_address -= remain_len;
        len         += remain_len;
    }
    remain_len      = len & (sector_size - 1);
    if (remain_len) {
        len         += sector_size - remain_len;
    }

    current_address = start_address;
    remain_len      = len;

    norflash_pre_operation();

    if ((current_address & HAL_NORFLASH_ADDR_MASK) == 0 && remain_len >= total_size) {
        // erase whole chip
        ret = norflash_erase(current_address, DRV_NORFLASH_ERASE_CHIP, suspend);
    } else {
        while (remain_len > 0 && ret == HAL_NORFLASH_OK) {
            if (remain_len >= block_size && ((current_address & (block_size - 1)) == 0)) {
                // if large enough to erase a block and current_address is block boundary - erase a block
                ret = norflash_erase(current_address, DRV_NORFLASH_ERASE_BLOCK, suspend);
                remain_len -= block_size;
                current_address += block_size;
            } else {
                // erase a sector
                ret = norflash_erase(current_address, DRV_NORFLASH_ERASE_SECTOR, suspend);
                if (remain_len > sector_size) {
                    remain_len -= sector_size;
                } else {
                    remain_len = 0;
                }
                current_address += sector_size;
            }
        }
    }

    norflash_post_operation();

#ifdef FLASH_SUSPEND
    if (ret == HAL_NORFLASH_SUSPENDED) {
        suspend_state = SUSPEND_STATE_ERASE;
        op_next_addr = current_address;
        op_remain_len = remain_len;
    } else {
        suspend_state = SUSPEND_STATE_NO;
    }
#endif

    return ret;
}

enum HAL_NORFLASH_RET_T hal_norflash_erase_suspend(enum HAL_NORFLASH_ID_T id, uint32_t start_address, uint32_t len, int suspend)
{
    ASSERT(norflash_ctx[id].opened, err_not_opened);

#ifdef FLASH_SUSPEND
    if (suspend_state != SUSPEND_STATE_NO) {
        return HAL_NORFLASH_BAD_OP;
    }
    if ((norflash_get_supported_mode() & HAL_NORFLASH_OP_MODE_SUSPEND) == 0) {
        suspend = 0;
    }
#endif

    return hal_norflash_erase_int(id, start_address, len, suspend);
}

enum HAL_NORFLASH_RET_T hal_norflash_erase(enum HAL_NORFLASH_ID_T id, uint32_t start_address, uint32_t len)
{
    return hal_norflash_erase_suspend(id, start_address, len, 0);
}

enum HAL_NORFLASH_RET_T hal_norflash_erase_resume(enum HAL_NORFLASH_ID_T id, int suspend)
{
#ifdef FLASH_SUSPEND
    enum HAL_NORFLASH_RET_T ret;

    ASSERT(norflash_ctx[id].opened, err_not_opened);

    if (suspend_state != SUSPEND_STATE_ERASE) {
        return HAL_NORFLASH_BAD_OP;
    }
    if ((norflash_get_supported_mode() & HAL_NORFLASH_OP_MODE_SUSPEND) == 0) {
        return HAL_NORFLASH_BAD_OP;
    }

    ret = norflash_erase_resume(suspend);
    if (ret == HAL_NORFLASH_SUSPENDED) {
        return ret;
    }

    return hal_norflash_erase_int(id, op_next_addr, op_remain_len, suspend);
#else
    return HAL_NORFLASH_OK;
#endif
}

static enum HAL_NORFLASH_RET_T hal_norflash_write_int(enum HAL_NORFLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len, int suspend)
{
    const uint8_t *current_buffer;
    uint32_t remain_len, current_address, total_size, page_size, write_size;
    uint32_t pp_remain;
    enum HAL_NORFLASH_RET_T ret = HAL_NORFLASH_OK;

    total_size      = norflash_ctx[id].total_size;
    page_size       = norflash_ctx[id].page_size;

    // Check address and length
    if (total_size <= (start_address & HAL_NORFLASH_ADDR_MASK)) {
        return HAL_NORFLASH_BAD_ADDR;
    }
    remain_len      = total_size - (start_address & HAL_NORFLASH_ADDR_MASK);
    if (len > remain_len) {
        return HAL_NORFLASH_BAD_LEN;
    }

    current_address = start_address;
    current_buffer  = buffer;
    remain_len      = len;

    norflash_pre_operation();

    while (remain_len > 0 && ret == HAL_NORFLASH_OK) {
        if (remain_len > page_size) {
            write_size = page_size;
        } else {
            write_size = remain_len;
        }
        // Align to program page
        pp_remain = page_size - (current_address & (page_size - 1));
        if (write_size > pp_remain) {
            write_size = pp_remain;
        }

        ret = norflash_write(current_address, current_buffer, write_size, suspend);

        current_address += write_size;
        current_buffer  += write_size;
        remain_len      -= write_size;
    }

    norflash_post_operation();

#ifdef FLASH_SUSPEND
    if (ret == HAL_NORFLASH_SUSPENDED) {
        suspend_state = SUSPEND_STATE_PROGRAM;
        op_next_addr = current_address;
        op_next_buf = current_buffer;
        op_remain_len = remain_len;
    } else {
        suspend_state = SUSPEND_STATE_NO;
    }
#endif

    return ret;
}

enum HAL_NORFLASH_RET_T hal_norflash_write_suspend(enum HAL_NORFLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len, int suspend)
{
    ASSERT(norflash_ctx[id].opened, err_not_opened);

#ifdef FLASH_SUSPEND
    if (suspend_state != SUSPEND_STATE_NO) {
        return HAL_NORFLASH_BAD_OP;
    }
    if ((norflash_get_supported_mode() & HAL_NORFLASH_OP_MODE_SUSPEND) == 0) {
        suspend = 0;
    }
#endif

    return hal_norflash_write_int(id, start_address, buffer, len, suspend);
}

enum HAL_NORFLASH_RET_T hal_norflash_write(enum HAL_NORFLASH_ID_T id, uint32_t start_address, const uint8_t *buffer, uint32_t len)
{
    return hal_norflash_write_suspend(id, start_address, buffer, len, 0);
}

enum HAL_NORFLASH_RET_T hal_norflash_write_resume(enum HAL_NORFLASH_ID_T id, int suspend)
{
#ifdef FLASH_SUSPEND
    enum HAL_NORFLASH_RET_T ret;

    ASSERT(norflash_ctx[id].opened, err_not_opened);

    if (suspend_state != SUSPEND_STATE_PROGRAM) {
        return HAL_NORFLASH_BAD_OP;
    }
    if ((norflash_get_supported_mode() & HAL_NORFLASH_OP_MODE_SUSPEND) == 0) {
        return HAL_NORFLASH_BAD_OP;
    }

    ret = norflash_write_resume(suspend);
    if (ret == HAL_NORFLASH_SUSPENDED) {
        return ret;
    }

    return hal_norflash_write_int(id, op_next_addr, op_next_buf, op_remain_len, suspend);
#else
    return HAL_NORFLASH_OK;
#endif
}

enum HAL_NORFLASH_RET_T hal_norflash_suspend_check_irq(enum HAL_NORFLASH_ID_T id, uint32_t irq_num)
{
#ifdef FLASH_SUSPEND
    int ret;

    ret = norflash_suspend_check_irq(irq_num);
    if (ret) {
        return HAL_NORFLASH_ERR;
    }
#endif

    return HAL_NORFLASH_OK;
}

enum HAL_NORFLASH_RET_T hal_norflash_read(enum HAL_NORFLASH_ID_T id, uint32_t start_address, uint8_t *buffer, uint32_t len)
{
    uint8_t *current_buffer;
    uint32_t remain_len, current_address, total_size, read_size;

    ASSERT(norflash_ctx[id].opened, err_not_opened);

    total_size      = norflash_ctx[id].total_size;

    // Check address and length
    if (total_size <= (start_address & HAL_NORFLASH_ADDR_MASK)) {
        return HAL_NORFLASH_BAD_ADDR;
    }
    remain_len      = total_size - (start_address & HAL_NORFLASH_ADDR_MASK);
    if (len > remain_len) {
        return HAL_NORFLASH_BAD_LEN;
    }

    read_size       = NORFLASHIP_RXFIFO_SIZE;
    remain_len      = len;
    current_address = start_address;
    current_buffer  = buffer;

    norflash_pre_operation();

    while (remain_len > 0) {
        read_size = (remain_len > NORFLASHIP_RXFIFO_SIZE) ? NORFLASHIP_RXFIFO_SIZE : remain_len;
        norflash_read(current_address, current_buffer, read_size);

        current_address += read_size;
        current_buffer  += read_size;
        remain_len      -= read_size;
    }

    norflash_post_operation();

    return HAL_NORFLASH_OK;
}

enum HAL_NORFLASH_RET_T hal_norflash_close(enum HAL_NORFLASH_ID_T id)
{
    return HAL_NORFLASH_OK;
}

void hal_norflash_sleep(enum HAL_NORFLASH_ID_T id)
{
    if (!norflash_ctx[id].opened) {
        return;
    }

#ifdef FLASH_DEEP_SLEEP
    norflash_sleep();
#else
    norflash_pre_operation();
#endif

    norflaship_busy_wait();
    norflaship_sleep();
}

void hal_norflash_wakeup(enum HAL_NORFLASH_ID_T id)
{
    if (!norflash_ctx[id].opened) {
        return;
    }

    norflaship_wakeup();

#ifdef FLASH_DEEP_SLEEP
    norflash_wakeup();
#else
    norflash_post_operation();
#endif

#if 0
    // Flush prefetch buffer
    *(volatile uint32_t *)FLASH_NC_BASE;
    *(volatile uint32_t *)(FLASH_NC_BASE + 0x1000);
#else
    norflaship_clear_rxfifo();
    norflaship_busy_wait();
#endif
}

int hal_norflash_busy(void)
{
    return norflaship_is_busy();
}

static void hal_norflash_prefetch_idle(void)
{
    hal_sys_timer_delay(4);
    if (norflaship_is_busy()) {
        hal_sys_timer_delay(4);
    }
}

enum HAL_NORFLASH_RET_T hal_norflash_init(void)
{
    enum HAL_NORFLASH_RET_T ret;

    // Pmu codes might be located in flash
    pmu_flash_freq_config(norflash_cfg.speed);

    // Avoid flash access from here
    hal_norflash_prefetch_idle();

    hal_cmu_flash_set_freq(hal_norflash_clk_to_cmu_freq(norflash_cfg.source_clk));

    ret = hal_norflash_open(HAL_NORFLASH_ID_0, &norflash_cfg);

    if (ret != HAL_NORFLASH_OK) {
        hal_cmu_flash_set_freq(HAL_CMU_FREQ_26M);
    }
    // Flash can be access again

    return ret;
}

uint32_t hal_norflash_get_flash_total_size(enum HAL_NORFLASH_ID_T id)
{
    return norflash_ctx[id].total_size;
}

int hal_norflash_opened(enum HAL_NORFLASH_ID_T id)
{
    return norflash_ctx[id].opened;
}

enum HAL_NORFLASH_RET_T hal_norflash_get_open_state(enum HAL_NORFLASH_ID_T id)
{
    return norflash_ctx[id].open_state;
}

enum HAL_NORFLASH_RET_T hal_norflash_enable_remap(enum HAL_NORFLASH_ID_T id, uint32_t addr, uint32_t len)
{
#if (CHIP_FLASH_CTRL_VER >= 2)
    uint32_t flash_size;
    uint32_t remap_addr, remap_len, remap_end, region_len;
    uint8_t remap_id;
    uint8_t msb_pos;
    int ret;

    STATIC_ASSERT((FLASH_BASE & HAL_NORFLASH_ADDR_MASK) == 0, "Bad FLASH_BASE");
    STATIC_ASSERT((FLASH_SIZE & (FLASH_SIZE - 1)) == 0, "Bad FLASH_SIZE");

#ifdef OTA_BARE_BOOT
    flash_size = FLASH_SIZE;
#else
    if (!norflash_ctx[id].opened) {
        return HAL_NORFLASH_NOT_OPENED;
    }

    flash_size = norflash_ctx[id].total_size;
#ifndef PROGRAMMER
    if (flash_size > FLASH_SIZE) {
        flash_size = FLASH_SIZE;
    }
#endif
#endif

    remap_addr = addr & HAL_NORFLASH_ADDR_MASK;
    if (remap_addr < flash_size / (1 << (NORFLASHIP_REMAP_NUM + 1))) {
        return HAL_NORFLASH_BAD_ADDR;
    }
    if (remap_addr % (flash_size / (1 << (NORFLASHIP_REMAP_NUM + 1)))) {
        return HAL_NORFLASH_BAD_ADDR;
    }

    if (remap_addr + len > flash_size / 2) {
        return HAL_NORFLASH_BAD_LEN;
    }

    norflaship_busy_wait();

    remap_id = 0;
    remap_end = flash_size / 2;
    remap_len = remap_end - remap_addr;
    while (remap_len && remap_id < NORFLASHIP_REMAP_NUM) {
        msb_pos = 31 - __CLZ(remap_len);
        region_len = 1 << msb_pos;
        remap_addr = (addr & ~HAL_NORFLASH_ADDR_MASK) + remap_end - region_len;
        ret = norflaship_config_remap_section(remap_id, remap_addr, region_len, remap_addr + flash_size / 2);
        if (ret) {
            return HAL_NORFLASH_BAD_OP;
        }
        remap_len -= region_len;
        remap_end -= region_len;
        remap_id++;
    }

    if (remap_len) {
        return HAL_NORFLASH_BAD_LEN;
    }

    hal_norflash_re_enable_remap(id);

    return HAL_NORFLASH_OK;
#else
    return HAL_NORFLASH_ERR;
#endif
}

enum HAL_NORFLASH_RET_T hal_norflash_disable_remap(enum HAL_NORFLASH_ID_T id)
{
#if (CHIP_FLASH_CTRL_VER >= 2)
    norflaship_busy_wait();

    norflaship_disable_remap();

    norflaship_busy_wait();
    norflaship_clear_fifos();

    return HAL_NORFLASH_OK;
#else
    return HAL_NORFLASH_ERR;
#endif
}

enum HAL_NORFLASH_RET_T hal_norflash_re_enable_remap(enum HAL_NORFLASH_ID_T id)
{
#if (CHIP_FLASH_CTRL_VER >= 2)
    norflaship_busy_wait();

    norflaship_enable_remap();

    norflaship_busy_wait();
    norflaship_clear_fifos();

    return HAL_NORFLASH_OK;
#else
    return HAL_NORFLASH_ERR;
#endif
}

int hal_norflash_get_remap_status(enum HAL_NORFLASH_ID_T id)
{
    return norflaship_get_remap_status();
}

