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
#include "plat_addr_map.h"
#include "reg_iomux_best1400.h"
#include "hal_iomux.h"
#include "hal_gpio.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "pmu.h"

//#define IOMUX_DBG

#if defined(IOMUX_DBG)
#define IOMUX_DEBUG TRACE
#else
#define IOMUX_DEBUG(...) do{ }while(0)
#endif

#ifdef I2S_VOLTAGE_VMEM
#define I2S_VOLTAGE_SEL                     HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define I2S_VOLTAGE_SEL                     HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef SPDIF0_VOLTAGE_VMEM
#define SPDIF0_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define SPDIF0_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef DIGMIC_VOLTAGE_VMEM
#define DIGMIC_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define DIGMIC_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef SPI_VOLTAGE_VMEM
#define SPI_VOLTAGE_SEL                     HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define SPI_VOLTAGE_SEL                     HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifdef CLKOUT_VOLTAGE_VMEM
#define CLKOUT_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_MEM
#else
#define CLKOUT_VOLTAGE_SEL                  HAL_IOMUX_PIN_VOLTAGE_VIO
#endif

#ifndef I2S_IOMUX_INDEX
#define I2S_IOMUX_INDEX                     0
#endif

#ifndef SPDIF0_IOMUX_INDEX
#define SPDIF0_IOMUX_INDEX                  0
#endif

#ifndef SPI_IOMUX_INDEX
#define SPI_IOMUX_INDEX                     0
#endif

#ifndef CLKOUT_IOMUX_INDEX
#define CLKOUT_IOMUX_INDEX                  0
#endif

#define IOMUX_FUNC_VAL_GPIO                 0

#define IOMUX_ALT_FUNC_NUM                  6

// Other func values: 0 -> gpio, 11 -> jtag, 10 -> ana_test
static const uint8_t index_to_func_val[IOMUX_ALT_FUNC_NUM] = { 1, 2, 3, 4, 5, 6, };

static const enum HAL_IOMUX_FUNCTION_T pin_func_map[HAL_IOMUX_PIN_NUM][IOMUX_ALT_FUNC_NUM] = {
    // P0_0
    { HAL_IOMUX_FUNC_I2S_SCK, HAL_IOMUX_FUNC_I2C_M0_SCL, HAL_IOMUX_FUNC_SPI_DI0, HAL_IOMUX_FUNC_UART2_RX,
      HAL_IOMUX_FUNC_CLK_32K_IN, HAL_IOMUX_FUNC_PWM0, },
    // P0_1
    { HAL_IOMUX_FUNC_I2S_WS, HAL_IOMUX_FUNC_I2C_M0_SDA, HAL_IOMUX_FUNC_SPI_CLK, HAL_IOMUX_FUNC_UART2_TX,
      HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_FUNC_PWM1, },
    // P0_2
    { HAL_IOMUX_FUNC_I2S_SDI0, HAL_IOMUX_FUNC_PDM0_D, HAL_IOMUX_FUNC_SPI_CS0, HAL_IOMUX_FUNC_UART1_CTS,
      HAL_IOMUX_FUNC_CLK_REQ_IN, HAL_IOMUX_FUNC_PWM2, },
    // P0_3
    { HAL_IOMUX_FUNC_I2S_SDO, HAL_IOMUX_FUNC_PDM0_CK, HAL_IOMUX_FUNC_SPI_DIO, HAL_IOMUX_FUNC_UART1_RTS,
      HAL_IOMUX_FUNC_CLK_REQ_OUT, HAL_IOMUX_FUNC_PWM3, },
    // P0_4
    { HAL_IOMUX_FUNC_UART1_RX, HAL_IOMUX_FUNC_PDM0_D, HAL_IOMUX_FUNC_SPI_CS1, HAL_IOMUX_FUNC_NONE,
      HAL_IOMUX_FUNC_I2S_MCLK, HAL_IOMUX_FUNC_PWM0, },
    // P0_5
    { HAL_IOMUX_FUNC_UART1_TX, HAL_IOMUX_FUNC_PDM0_CK, HAL_IOMUX_FUNC_SPI_DCN, HAL_IOMUX_FUNC_NONE,
      HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_FUNC_PWM1, },
    // P0_6
    { HAL_IOMUX_FUNC_UART0_RX, HAL_IOMUX_FUNC_I2C_M0_SCL, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_BT_UART_RX,
      HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, },
    // P0_7
    { HAL_IOMUX_FUNC_UART0_TX, HAL_IOMUX_FUNC_I2C_M0_SDA, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_BT_UART_TX,
      HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, },
    // P1_0
    { HAL_IOMUX_FUNC_I2C_M0_SCL, HAL_IOMUX_FUNC_PDM0_D, HAL_IOMUX_FUNC_BT_UART_CTS, HAL_IOMUX_FUNC_BT_UART_RX,
      HAL_IOMUX_FUNC_I2S_MCLK, HAL_IOMUX_FUNC_PWM2, },
    // P1_1
    { HAL_IOMUX_FUNC_I2C_M0_SDA, HAL_IOMUX_FUNC_PDM0_CK, HAL_IOMUX_FUNC_BT_UART_RTS, HAL_IOMUX_FUNC_BT_UART_TX,
      HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_FUNC_PWM3, },
    // P1_2
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_UART2_RX,
      HAL_IOMUX_FUNC_I2S_MCLK, HAL_IOMUX_FUNC_NONE, },
    // P1_3
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_UART2_TX,
      HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_FUNC_NONE, },
    // P1_4
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE,
      HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, },
    // P1_5
    { HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE,
      HAL_IOMUX_FUNC_NONE, HAL_IOMUX_FUNC_NONE, },
    // P1_6
    { HAL_IOMUX_FUNC_UART0_RX, HAL_IOMUX_FUNC_UART1_RX, HAL_IOMUX_FUNC_BT_UART_RX, HAL_IOMUX_FUNC_UART2_RX,
      HAL_IOMUX_FUNC_I2S_MCLK, HAL_IOMUX_FUNC_NONE, },
    // P1_7
    { HAL_IOMUX_FUNC_UART0_TX, HAL_IOMUX_FUNC_UART1_TX, HAL_IOMUX_FUNC_BT_UART_TX, HAL_IOMUX_FUNC_UART2_TX,
      HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_FUNC_NONE, },
};

static struct IOMUX_T * const iomux = (struct IOMUX_T *)IOMUX_BASE;

static enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T BOOT_DATA_LOC uart0_volt = HAL_IOMUX_PIN_VOLTAGE_VIO;
static enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T BOOT_DATA_LOC uart1_volt = HAL_IOMUX_PIN_VOLTAGE_VIO;
static enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T BOOT_DATA_LOC uart2_volt = HAL_IOMUX_PIN_VOLTAGE_VIO;

void hal_iomux_set_default_config(void)
{
    uint32_t i;

    // Set uart0 voltage domain
    iomux->REG_008 &= ~((1 << HAL_IOMUX_PIN_P0_6) | (1 << HAL_IOMUX_PIN_P0_7));

    // Set all unused GPIOs to pull-down by default
    for (i = 0; i < 8; i++) {
        if (((iomux->REG_004 & (0xF << (i * 4))) >> (i * 4)) == 0xF) {
            iomux->REG_030 |= (1 << i);
        }
    }
    for (i = 0; i < 8; i++) {
        if (((iomux->REG_078 & (0xF << (i * 4))) >> (i * 4)) == 0xF) {
            iomux->REG_030 |= (1 << (i + 24));
        }
    }
}

uint32_t hal_iomux_check(const struct HAL_IOMUX_PIN_FUNCTION_MAP *map, uint32_t count)
{
    uint32_t i = 0;
    for (i = 0; i < count; ++i) {
    }
    return 0;
}

static void hal_iomux_pmu_uart_select_enable(enum HAL_IOMUX_FUNCTION_T uart)
{
    uint8_t pmu_uart_id = 0;
    ASSERT(uart >= HAL_IOMUX_FUNC_PMU_UART0 && uart <= HAL_IOMUX_FUNC_PMU_UART1,"hal_iomux_pmu_uart_select error uart:%d", uart);

    if (uart == HAL_IOMUX_FUNC_PMU_UART0){
        pmu_uart_id = 0;
    }else{
        pmu_uart_id = 1;
    }

    iomux->REG_050 &= ~IOMUX_SEL_PMU_UART_MASK;
    iomux->REG_050 |= IOMUX_SEL_PMU_UART(pmu_uart_id);
    iomux->REG_050 |= IOMUX_EN_PMU_UART;
}

static void hal_iomux_pmu_uart_select_disable(void)
{
    iomux->REG_050 &= ~IOMUX_SEL_PMU_UART_MASK;
    iomux->REG_050 &= ~IOMUX_EN_PMU_UART;    
}

uint32_t hal_iomux_init(const struct HAL_IOMUX_PIN_FUNCTION_MAP *map, uint32_t count)
{
    uint32_t i;
    uint32_t ret;

    for (i = 0; i < count; ++i) {
        ret = hal_iomux_set_function(map[i].pin, map[i].function, HAL_IOMUX_OP_CLEAN_OTHER_FUNC_BIT);
        if (ret) {
            return (i << 8) + 1;
        }
        ret = hal_iomux_set_io_voltage_domains(map[i].pin, map[i].volt);
        if (ret) {
            return (i << 8) + 2;
        }
        ret = hal_iomux_set_io_pull_select(map[i].pin, map[i].pull_sel);
        if (ret) {
            return (i << 8) + 3;
        }
    }

    return 0;
}

uint32_t hal_iomux_set_function(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_FUNCTION_T func,
                            enum HAL_IOMUX_OP_TYPE_T type)
{
    int i;
    uint8_t val;
    __IO uint32_t *reg;
    uint32_t shift;
    if (pin >= HAL_IOMUX_PIN_LED_NUM) {
        IOMUX_DEBUG("pin is out of range");
        return 1;
    }
    if (func >= HAL_IOMUX_FUNC_END) {
        IOMUX_DEBUG("func is out of range");
        return 2;
    }

    if (pin == HAL_IOMUX_PIN_P0_6 || pin == HAL_IOMUX_PIN_P0_7) {
        if (func ==  HAL_IOMUX_FUNC_I2C_SCL || func == HAL_IOMUX_FUNC_I2C_SDA) {
            // Enable analog I2C slave
            iomux->REG_050 &= ~IOMUX_GPIO_I2C_MODE;
            iomux->REG_050 |= IOMUX_I2C0_M_SEL_GPIO;
            // Set mcu GPIO func
            iomux->REG_004 = (iomux->REG_004 & ~(IOMUX_GPIO_P06_SEL_MASK | IOMUX_GPIO_P07_SEL_MASK)) |
                IOMUX_GPIO_P06_SEL(IOMUX_FUNC_VAL_GPIO) | IOMUX_GPIO_P07_SEL(IOMUX_FUNC_VAL_GPIO);
            return 0;
        } else {
            iomux->REG_050 |= IOMUX_GPIO_I2C_MODE | IOMUX_I2C0_M_SEL_GPIO;
            // Continue to set the alt func
        }
    }else if(pin == HAL_IOMUX_PIN_LED1 || pin == HAL_IOMUX_PIN_LED2){
        if(func == HAL_IOMUX_FUNC_PMU_UART0 ||
           func == HAL_IOMUX_FUNC_PMU_UART1){
            pmu_led_uart_enable(pin);
            hal_iomux_pmu_uart_select_enable(func);
            return 0;
        }else if (func == HAL_IOMUX_FUNC_GPIO){
            pmu_led_uart_disable(pin);
            hal_iomux_pmu_uart_select_disable();
        }
        ASSERT(func == HAL_IOMUX_FUNC_GPIO, "Bad func=%d for IOMUX pin=%d", func, pin);
        return 0;
    }

    if (func == HAL_IOMUX_FUNC_GPIO) {
        val = IOMUX_FUNC_VAL_GPIO;
    } else {
        for (i = 0; i < IOMUX_ALT_FUNC_NUM; i++) {
            if (pin_func_map[pin][i] == func) {
                break;
            }
        }
        if (i == IOMUX_ALT_FUNC_NUM) {
            IOMUX_DEBUG("invalid func");
            return 3;
        }
        val = index_to_func_val[i];
    }

    shift = (pin % 8) * 4;

    if (pin <= HAL_IOMUX_PIN_P0_7) {
        reg = &iomux->REG_004;
    } else {
        reg = &iomux->REG_078;
    }
    *reg = (*reg & ~(0xF << shift)) | (val << shift);
    return 0;
}

#define iomux_read32(addr) (*(volatile uint32_t *)(addr))

enum HAL_IOMUX_FUNCTION_T hal_iomux_get_function(enum HAL_IOMUX_PIN_T pin)
{
    uint32_t shift, i;
    uint32_t func, base;

    if (pin >= HAL_IOMUX_PIN_NUM) {
        return 1;
    }
    if (pin <= HAL_IOMUX_PIN_P0_7)
        base = (uint32_t)&iomux->REG_004;
    else
        base = (uint32_t)&iomux->REG_078;

    shift = (pin % 8) * 4;

    func = (iomux_read32(base) >> shift) & 0xF;

    for(i = 0;i < ARRAY_SIZE(index_to_func_val); i++) {
        if (index_to_func_val[i] == func) {
            return pin_func_map[pin][i];
        }
    }
    return HAL_IOMUX_FUNC_NONE;
}

uint32_t hal_iomux_set_io_voltage_domains(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt)
{
    if (pin >= HAL_IOMUX_PIN_LED_NUM) {
        return 1;
    }

    if (pin < HAL_IOMUX_PIN_LED1) {
        // 0: VIO vol = 3.3v, 1: MEM vol = 1.8v
        if (volt == HAL_IOMUX_PIN_VOLTAGE_VIO) {
            iomux->REG_008 &= ~(1 << pin);
        } else {
            iomux->REG_008 |= (1 << pin);
        }
    } else if (pin == HAL_IOMUX_PIN_LED1 || pin == HAL_IOMUX_PIN_LED2) {
        pmu_led_set_voltage_domains(pin, volt);
    }

    return 0;
}

uint32_t hal_iomux_set_io_pull_select(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_PULL_SELECT_T pull_sel)
{
    if (pin >= HAL_IOMUX_PIN_LED_NUM) {
        return 1;
    }

    if (pin < HAL_IOMUX_PIN_LED1) {
        switch(pull_sel) {
        case HAL_IOMUX_PIN_PULLUP_ENALBE:
            iomux->REG_030 &= ~(1 << pin); //pulldown=0
            iomux->REG_02C |= (1 << pin);  //pullup=1
            break;
        case HAL_IOMUX_PIN_PULLDOWN_ENALBE:
            iomux->REG_02C &= ~(1 << pin); //pullup=0
            iomux->REG_030 |= (1 << pin);  //pulldown=1
            break;
        case HAL_IOMUX_PIN_NOPULL:
        default:
            iomux->REG_02C &= ~(1 << pin); //pullup=0
            iomux->REG_030 &= ~(1 << pin); //pulldown=0
            break;
        }
    } else if (pin == HAL_IOMUX_PIN_LED1 || pin == HAL_IOMUX_PIN_LED2) {
        pmu_led_set_pull_select(pin, pull_sel);
    }

    return 0;
}

uint32_t hal_iomux_set_io_drive(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_DRV_T drv)
{
    if (pin >= HAL_IOMUX_PIN_NUM) {
        return 1;
    }
    if (drv == HAL_IOMUX_PIN_DRV_FAST) {
        iomux->REG_074 |= (3 << (pin << 1));
    } else {
        iomux->REG_074 &= ~(3 << (pin << 1));
    }
    return 0;
}

void hal_iomux_set_sdmmc_dt_n_out_group(int enable)
{
}

void hal_iomux_set_uart0_voltage(enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt)
{
    uart0_volt = volt;
}

void hal_iomux_set_uart1_voltage(enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt)
{
    uart1_volt = volt;
}

void hal_iomux_set_uart2_voltage(enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt)
{
    uart2_volt = volt;
}

bool hal_iomux_uart0_connected(void)
{
    uint32_t reg_050, reg_004, reg_02c, reg_030;
    uint32_t mask;
    int val;

    // Save current iomux settings
    reg_050 = iomux->REG_050;
    reg_004 = iomux->REG_004;
    reg_02c = iomux->REG_02C;
    reg_030 = iomux->REG_030;

    // Disable analog I2C slave & master
    iomux->REG_050 |= IOMUX_GPIO_I2C_MODE | IOMUX_I2C0_M_SEL_GPIO;
    // Set uart0-rx as gpio
    iomux->REG_004 = SET_BITFIELD(iomux->REG_004, IOMUX_GPIO_P06_SEL, IOMUX_FUNC_VAL_GPIO);

    mask = (1 << HAL_IOMUX_PIN_P0_6);
    // Set voltage domain
    if (uart0_volt == HAL_IOMUX_PIN_VOLTAGE_VIO) {
        iomux->REG_008 &= ~(mask | (1 << HAL_IOMUX_PIN_P0_7));
    } else {
        iomux->REG_008 |= (mask | (1 << HAL_IOMUX_PIN_P0_7));
    }
    // Clear pullup
    iomux->REG_02C &= ~mask;
    // Setup pulldown
    iomux->REG_030 |= mask;

    hal_gpio_pin_set_dir(HAL_IOMUX_PIN_P0_6, HAL_GPIO_DIR_IN, 0);

    hal_sys_timer_delay(MS_TO_TICKS(2));

    val = hal_gpio_pin_get_val(HAL_IOMUX_PIN_P0_6);

    // Restore iomux settings
    iomux->REG_030 = reg_030;
    iomux->REG_02C = reg_02c;
    iomux->REG_004 = reg_004;
    iomux->REG_050 = reg_050;

    hal_sys_timer_delay(MS_TO_TICKS(2));

    return !!val;
}

bool hal_iomux_uart1_connected(void)
{
    uint32_t reg_004, reg_02c, reg_030;
    uint32_t mask;
    int val;

    // Save current iomux settings
    reg_004 = iomux->REG_004;
    reg_02c = iomux->REG_02C;
    reg_030 = iomux->REG_030;

    // Set uart1-rx as gpio
    iomux->REG_004 = SET_BITFIELD(iomux->REG_004, IOMUX_GPIO_P04_SEL, IOMUX_FUNC_VAL_GPIO);

    mask = (1 << HAL_IOMUX_PIN_P0_4);
    // Set voltage domain
    if (uart1_volt == HAL_IOMUX_PIN_VOLTAGE_VIO) {
        iomux->REG_008 &= ~(mask | (1 << HAL_IOMUX_PIN_P0_5));
    } else {
        iomux->REG_008 |= (mask | (1 << HAL_IOMUX_PIN_P0_5));
    }
    // Clear pullup
    iomux->REG_02C &= ~mask;
    // Setup pulldown
    iomux->REG_030 |= mask;

    hal_gpio_pin_set_dir(HAL_IOMUX_PIN_P0_4, HAL_GPIO_DIR_IN, 0);

    hal_sys_timer_delay(MS_TO_TICKS(2));

    val = hal_gpio_pin_get_val(HAL_IOMUX_PIN_P0_4);

    // Restore iomux settings
    iomux->REG_030 = reg_030;
    iomux->REG_02C = reg_02c;
    iomux->REG_004 = reg_004;

    hal_sys_timer_delay(MS_TO_TICKS(2));

    return !!val;
}

void hal_iomux_set_uart0(void)
{
    uint32_t mask;

    // Disable analog I2C slave & master
    iomux->REG_050 |= IOMUX_GPIO_I2C_MODE | IOMUX_I2C0_M_SEL_GPIO;
    // Set uart0 func
    iomux->REG_004 = (iomux->REG_004 & ~(IOMUX_GPIO_P06_SEL_MASK | IOMUX_GPIO_P07_SEL_MASK)) |
        IOMUX_GPIO_P06_SEL(1) | IOMUX_GPIO_P07_SEL(1);

    mask = (1 << HAL_IOMUX_PIN_P0_6) | (1 << HAL_IOMUX_PIN_P0_7);
    // Set voltage domain
    if (uart0_volt == HAL_IOMUX_PIN_VOLTAGE_VIO) {
        iomux->REG_008 &= ~mask;
    } else {
        iomux->REG_008 |= mask;
    }
    // Setup pullup
    iomux->REG_02C |= (1 << HAL_IOMUX_PIN_P0_6);
    iomux->REG_02C &= ~(1 << HAL_IOMUX_PIN_P0_7);
    // Clear pulldown
    iomux->REG_030 &= ~mask;
}

void hal_iomux_set_uart1(void)
{
    uint32_t mask;

    // Set uart1 func
    iomux->REG_004 = (iomux->REG_004 & ~(IOMUX_GPIO_P04_SEL_MASK | IOMUX_GPIO_P05_SEL_MASK)) |
        IOMUX_GPIO_P04_SEL(1) | IOMUX_GPIO_P05_SEL(1);

    mask = (1 << HAL_IOMUX_PIN_P0_4) | (1 << HAL_IOMUX_PIN_P0_5);
    // Set voltage domain
    if (uart1_volt == HAL_IOMUX_PIN_VOLTAGE_VIO) {
        iomux->REG_008 &= ~mask;
    } else {
        iomux->REG_008 |= mask;
    }
    // Setup pullup
    iomux->REG_02C |= (1 << HAL_IOMUX_PIN_P0_4);
    iomux->REG_02C &= ~(1 << HAL_IOMUX_PIN_P0_5);
    // Clear pulldown
    iomux->REG_030 &= ~mask;
}

void hal_iomux_set_uart2(void)
{
    uint32_t mask;

    // Set uart2 func, p00=rxd, p01=txd
    iomux->REG_004 = (iomux->REG_004 & ~(IOMUX_GPIO_P00_SEL_MASK | IOMUX_GPIO_P01_SEL_MASK)) |
        IOMUX_GPIO_P00_SEL(4) | IOMUX_GPIO_P01_SEL(4);

    mask = (1 << HAL_IOMUX_PIN_P0_0) | (1 << HAL_IOMUX_PIN_P0_1);
    // Set voltage domain
    if (uart2_volt == HAL_IOMUX_PIN_VOLTAGE_VIO) {
        iomux->REG_008 &= ~mask;
    } else {
        iomux->REG_008 |= mask;
    }
    // Setup pullup
    iomux->REG_02C |= (1 << HAL_IOMUX_PIN_P0_0);
    iomux->REG_02C &= ~(1 << HAL_IOMUX_PIN_P0_1);
    // Clear pulldown
    iomux->REG_030 &= ~mask;
}

void hal_iomux_set_analog_i2c(void)
{
    uint32_t mask;

    // Disable analog I2C master
    iomux->REG_050 |= IOMUX_GPIO_I2C_MODE | IOMUX_I2C0_M_SEL_GPIO;
    // Set mcu GPIO func
    iomux->REG_004 = (iomux->REG_004 & ~(IOMUX_GPIO_P06_SEL_MASK | IOMUX_GPIO_P07_SEL_MASK)) |
        IOMUX_GPIO_P06_SEL(0) | IOMUX_GPIO_P07_SEL(0);
    // Enable analog I2C slave
    iomux->REG_050 &= ~IOMUX_GPIO_I2C_MODE;

    mask = (1 << HAL_IOMUX_PIN_P0_6) | (1 << HAL_IOMUX_PIN_P0_7);
    // Set voltage domain
    if (uart0_volt == HAL_IOMUX_PIN_VOLTAGE_VIO) {
        iomux->REG_008 &= ~mask;
    } else {
        iomux->REG_008 |= mask;
    }
    // Setup pullup
    iomux->REG_02C |= mask;
    // Clear pulldown
    iomux->REG_030 &= ~mask;
}

void hal_iomux_set_jtag(void)
{
    uint32_t mask;
    uint32_t val;

    // SWCLK/TCK, SWDIO/TMS
    mask = IOMUX_GPIO_P01_SEL_MASK | IOMUX_GPIO_P02_SEL_MASK;
    val = IOMUX_GPIO_P01_SEL(11) | IOMUX_GPIO_P02_SEL(11);
    // TDI, TDO
#ifdef JTAG_TDI_TDO_PIN
    mask |= IOMUX_GPIO_P00_SEL_MASK | IOMUX_GPIO_P03_SEL_MASK;
    val |= IOMUX_GPIO_P00_SEL(11) | IOMUX_GPIO_P03_SEL(11);
#endif
    // RESET
#ifdef JTAG_RESET_PIN
    mask |= IOMUX_GPIO_P05_SEL_MASK;
    val |= IOMUX_GPIO_P05_SEL(11);
#endif

    iomux->REG_004 = (iomux->REG_004 & ~mask) | val;

    mask = (1 << HAL_IOMUX_PIN_P0_1) | (1 << HAL_IOMUX_PIN_P0_2);
#ifdef JTAG_TDI_TDO_PIN
    mask |= (1 << HAL_IOMUX_PIN_P0_0) | (1 << HAL_IOMUX_PIN_P0_3);
#endif
#ifdef JTAG_RESET_PIN
    mask |= (1 << HAL_IOMUX_PIN_P0_5);
#endif
    // Clear pullup
    iomux->REG_02C &= ~mask;
    // Clear pulldown
    iomux->REG_030 &= ~mask;
}

enum HAL_IOMUX_ISPI_ACCESS_T hal_iomux_ispi_access_enable(enum HAL_IOMUX_ISPI_ACCESS_T access)
{
    return access;
}

enum HAL_IOMUX_ISPI_ACCESS_T hal_iomux_ispi_access_disable(enum HAL_IOMUX_ISPI_ACCESS_T access)
{
    return access;
}

void hal_iomux_ispi_access_init(void)
{
}

void hal_iomux_set_i2s0(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_i2s[] = {
        {HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_I2S_SCK,  I2S_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_I2S_WS,   I2S_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_2, HAL_IOMUX_FUNC_I2S_SDI0, I2S_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_3, HAL_IOMUX_FUNC_I2S_SDO,  I2S_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };

    hal_iomux_init(pinmux_i2s, ARRAY_SIZE(pinmux_i2s));
}

void hal_iomux_set_spdif0(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spdif[] = {
#if (SPDIF0_IOMUX_INDEX == 4)
        {HAL_IOMUX_PIN_P0_4, HAL_IOMUX_FUNC_SPDIF0_DI, SPDIF0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P0_5, HAL_IOMUX_FUNC_SPDIF0_DO, SPDIF0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#else
        {HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_SPDIF0_DI, SPDIF0_VOLTAGE_SEL, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_SPDIF0_DO, SPDIF0_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
#endif
    };

    hal_iomux_init(pinmux_spdif, ARRAY_SIZE(pinmux_spdif));
}

void hal_iomux_set_spdif1(void)
{
}

void hal_iomux_set_dig_mic(uint32_t map)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_digitalmic0[] = {
    };
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_digitalmic1[] = {
    };
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_digitalmic2[] = {
    };
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_digitalmic3[] = {
    };

    if (map & (1 << 0)) {
        hal_iomux_init(pinmux_digitalmic0, ARRAY_SIZE(pinmux_digitalmic0));
    }
    if (map & (1 << 1)) {
        hal_iomux_init(pinmux_digitalmic1, ARRAY_SIZE(pinmux_digitalmic1));
    }
    if (map & (1 << 2)) {
        hal_iomux_init(pinmux_digitalmic2, ARRAY_SIZE(pinmux_digitalmic2));
    }
    if (map & (1 << 3)) {
        hal_iomux_init(pinmux_digitalmic3, ARRAY_SIZE(pinmux_digitalmic3));
    }
}

void hal_iomux_set_spi(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spi_3wire[3] = {
        {HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_SPI_CLK, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_2, HAL_IOMUX_FUNC_SPI_CS0, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
        {HAL_IOMUX_PIN_P0_3, HAL_IOMUX_FUNC_SPI_DIO, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };
#ifdef SPI_IOMUX_4WIRE
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_spi_4wire[1] = {
        {HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_SPI_DI0, SPI_VOLTAGE_SEL, HAL_IOMUX_PIN_NOPULL},
    };
#endif

    hal_iomux_init(pinmux_spi_3wire, ARRAY_SIZE(pinmux_spi_3wire));
#ifdef SPI_IOMUX_4WIRE
    hal_iomux_init(pinmux_spi_4wire, ARRAY_SIZE(pinmux_spi_4wire));
#endif
}

void hal_iomux_set_spilcd(void)
{
}

void hal_iomux_set_clock_out(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_clkout[] = {
        {HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_PIN_VOLTAGE_MEM, HAL_IOMUX_PIN_NOPULL},
//        {HAL_IOMUX_PIN_P0_5, HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_PIN_VOLTAGE_MEM, HAL_IOMUX_PIN_NOPULL},
//        {HAL_IOMUX_PIN_P1_1, HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_PIN_VOLTAGE_MEM, HAL_IOMUX_PIN_NOPULL},
//        {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_PIN_VOLTAGE_MEM, HAL_IOMUX_PIN_NOPULL},
//        {HAL_IOMUX_PIN_P1_7, HAL_IOMUX_FUNC_CLK_OUT, HAL_IOMUX_PIN_VOLTAGE_MEM, HAL_IOMUX_PIN_NOPULL},
    };

    hal_iomux_init(pinmux_clkout, ARRAY_SIZE(pinmux_clkout));
}

int hal_pwrkey_set_irq(enum HAL_PWRKEY_IRQ_T type)
{
    uint32_t v;

    if (type == HAL_PWRKEY_IRQ_NONE) {
        v = IOMUX_PWR_KEY_DOWN_INT_CLR | IOMUX_PWR_KEY_UP_INT_CLR;
    } else if (type == HAL_PWRKEY_IRQ_FALLING_EDGE) {
        v = IOMUX_PWR_KEY_DOWN_INT_EN | IOMUX_PWR_KEY_DOWN_INT_MASK;
    } else if (type == HAL_PWRKEY_IRQ_RISING_EDGE) {
        v = IOMUX_PWR_KEY_UP_INT_EN | IOMUX_PWR_KEY_UP_INT_MASK;
    } else if (type == HAL_PWRKEY_IRQ_BOTH_EDGE) {
        v = IOMUX_PWR_KEY_DOWN_INT_EN | IOMUX_PWR_KEY_DOWN_INT_MASK |
            IOMUX_PWR_KEY_UP_INT_EN | IOMUX_PWR_KEY_UP_INT_MASK;
    } else {
        return 1;
    }

    iomux->REG_040 = v;

    return 0;
}

bool hal_pwrkey_pressed(void)
{
    uint32_t v = iomux->REG_040;
    return !!(v & IOMUX_PWR_KEY_VAL);
}

bool hal_pwrkey_startup_pressed(void)
{
    return hal_pwrkey_pressed();
}

enum HAL_PWRKEY_IRQ_T hal_pwrkey_get_irq_state(void)
{
    enum HAL_PWRKEY_IRQ_T state = HAL_PWRKEY_IRQ_NONE;
    uint32_t v = iomux->REG_040;

    if (v & IOMUX_PWR_KEY_DOWN_INT_ST) {
        state |= HAL_PWRKEY_IRQ_FALLING_EDGE;
    }

    if (v & IOMUX_PWR_KEY_UP_INT_ST) {
        state |= HAL_PWRKEY_IRQ_RISING_EDGE;
    }

    return state;
}

void hal_iomux_set_bt_tport(void)
{
    uint32_t val;

    // P0_0 ~ P0_5
    val = IOMUX_GPIO_P00_SEL(0xA)
        | IOMUX_GPIO_P01_SEL(0xA)
        | IOMUX_GPIO_P02_SEL(0xA)
        | IOMUX_GPIO_P03_SEL(0xA)
        | IOMUX_GPIO_P04_SEL(0xA)
        | IOMUX_GPIO_P05_SEL(0xA)
        | IOMUX_GPIO_P06_SEL(0x1)
        | IOMUX_GPIO_P07_SEL(0x1);
    iomux->REG_004 = val;

    // ANA TEST DIR
    iomux->REG_014 = IOMUX_ANA_TEST_DIR(0xFF);
    // ANA TEST SEL
    iomux->REG_018 = IOMUX_ANA_TEST_SEL(0x0B);
}
