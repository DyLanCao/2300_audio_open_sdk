#include "plat_addr_map.h"
#include "cmsis_nvic.h"
#include "hal_dma.h"
#include "hal_sensor_eng.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "reg_timer.h"
#include "string.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif

#define MAX_RX_FRAME_NUM (5)

// 003C
#define SENSOR_ENG_FSM_SHIFT                8
#define SENSOR_ENG_FSM_MASK                 (0xF << SENSOR_ENG_FSM_SHIFT)
#define SENSOR_ENG_FSM(n)                   BITFIELD_VAL(SENSOR_ENG_FSM, n)
#define SENSOR3_BUSY                        (1 << 7)
#define SENSOR2_BUSY                        (1 << 6)
#define SENSOR1_BUSY                        (1 << 5)
#define SENSOR0_BUSY                        (1 << 4)

struct SENSOR_ENG_T {
    __IO uint32_t SLV0_CONFIG_REG;  // 0000
    __IO uint32_t SLV1_CONFIG_REG;  // 0004
    __IO uint32_t SLV2_CONFIG_REG;  // 0008
    __IO uint32_t SLV3_CONFIG_REG;  // 000C
    __IO uint32_t GLOBAL_CFG0_REG;  // 0010
    __IO uint32_t GLOBAL_CFG1_REG;  // 0014
    __IO uint32_t GLOBAL_CFG2_REG;  // 0018
    __IO uint32_t GLOBAL_CFG3_REG;  // 001C
    __IO uint32_t SLV0_INTR_MASK;   // 0020
    __IO uint32_t SLV1_INTR_MASK;   // 0024
    __IO uint32_t SLV2_INTR_MASK;   // 0028
    __IO uint32_t SLV3_INTR_MASK;   // 002C
    __IO uint32_t SENSOR_INTR_CLR;  // 0030
    __I  uint32_t SENSOR_INTR_STS;  // 0034
    __IO uint32_t RESERVED;         // 0038
    __IO uint32_t SENSOR_STATUS;    // 003C
};

static struct SENSOR_ENG_T * const sensor_eng[] = {
    (struct SENSOR_ENG_T *)SENSOR_ENG_BASE,
};

static struct DUAL_TIMER_T * const sensor_eng_timer = (struct DUAL_TIMER_T *)MCU_TIMER2_BASE;

static struct HAL_DMA_DESC_T tx_dma_desc[2];
static struct HAL_DMA_DESC_T rx_dma_desc[MAX_RX_FRAME_NUM];

static HAL_SENSOR_ENG_HANDLER_T sensor_eng_handler;

static uint8_t dma_rx_chan;

static uint8_t *dma_rx_buf;
static uint32_t dma_rx_len;
static uint32_t prev_rx_pos;

static uint32_t clr_irq_val;

void hal_sensor_process_rx_buffer(void)
{
    if (dma_rx_len == 0) {
        return;
    }

    uint8_t *dma_buf_active = (uint8_t *)hal_dma_get_cur_dst_addr(dma_rx_chan);
    uint32_t result_len = 0;
    uint8_t *curr_pos;

    curr_pos = dma_rx_buf + prev_rx_pos;

    //TRACE("dma_buf_active:%08x curr_pos:%08x", dma_buf_active, curr_pos);

    if (dma_buf_active < curr_pos) {
        result_len = dma_rx_buf + dma_rx_len - curr_pos;
        //TRACE("1:result_len:%08x, curr_pos:%08x", result_len, curr_pos);
        //DUMP8("%02x ", curr_pos, result_len);
        if (sensor_eng_handler){
            sensor_eng_handler(HAL_SENSOR_ENGINE_ID_0, HAL_SENSOR_ENGINE_DEVICE_I2C0, curr_pos, result_len);
        }
        curr_pos = dma_rx_buf;
    }

    if (curr_pos < dma_buf_active) {
        result_len = dma_buf_active - curr_pos;
        //TRACE("2:result_len:%08x, curr_pos:%08x", result_len, curr_pos);
        //DUMP8("%02x ", curr_pos, result_len);
        if (sensor_eng_handler){
            sensor_eng_handler(HAL_SENSOR_ENGINE_ID_0, HAL_SENSOR_ENGINE_DEVICE_I2C0, curr_pos, result_len);
        }
        curr_pos = dma_buf_active;
    }

    prev_rx_pos = curr_pos - dma_rx_buf;
}

static void hal_sensor_eng_irq_handler(void)
{
    //TRACE("%s: irq=0x%08x", __func__, sensor_eng[0]->SENSOR_INTR_STS);

    sensor_eng[0]->SENSOR_INTR_CLR = 0x00000001;

    hal_sensor_process_rx_buffer();
}

static void hal_sensor_eng_irq_enable(void)
{
    NVIC_SetVector(SENSOR_WKUP_IRQn, (uint32_t)hal_sensor_eng_irq_handler);
    NVIC_SetPriority(SENSOR_WKUP_IRQn, IRQ_PRIORITY_HIGH);
    NVIC_ClearPendingIRQ(SENSOR_WKUP_IRQn);
    NVIC_EnableIRQ(SENSOR_WKUP_IRQn);
}

static void hal_sensor_eng_irq_disable(void)
{
    NVIC_DisableIRQ(SENSOR_WKUP_IRQn);
}

static void hal_sensor_eng_timer_stop(void)
{
    sensor_eng_timer->timer[0].Control = 0;
    sensor_eng_timer->timer[0].IntClr = 1;
}

static void hal_sensor_eng_timer_open(uint32_t ticks)
{
    if (ticks > 1) {
        ticks -= 1;
    } else {
        ticks = 1;
    }
    sensor_eng_timer->timer[0].Load = ticks;
    sensor_eng_timer->timer[0].Control = TIMER_CTRL_MODE_PERIODIC | TIMER_CTRL_INTEN | TIMER_CTRL_PRESCALE_DIV_1 | TIMER_CTRL_SIZE_32_BIT;
}

static void hal_sensor_eng_timer_start(void)
{
    sensor_eng_timer->timer[0].Control |= TIMER_CTRL_EN;
}

int hal_sensor_engine_open(const struct HAL_SENSOR_ENGINE_CFG_T *cfg)
{
    struct HAL_DMA_CH_CFG_T rx_dma_cfg;
    struct HAL_DMA_CH_CFG_T clr_dma_cfg;
    enum HAL_DMA_RET_T dret;
    uint32_t i;

    ASSERT(cfg->id == HAL_SENSOR_ENGINE_ID_0, "Bad sensor engine id: %d", cfg->id);
    ASSERT(cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C0, "Bad sensor engine device: %d", cfg->device);
    ASSERT(cfg->rx_burst_cnt <= MAX_RX_FRAME_NUM, "Bad rx_burst_cnt: %d", cfg->rx_burst_cnt);
    ASSERT(cfg->tx_dma_cfg, "Bad tx_dma_cfg");

    sensor_eng_handler = cfg->handler;
    dma_rx_chan = cfg->rx_dma_cfg->ch;
    dma_rx_buf = (uint8_t *)cfg->rx_dma_cfg->dst;
    dma_rx_len = cfg->rx_burst_len * cfg->rx_burst_cnt;
    prev_rx_pos = 0;

    dret = hal_dma_init_desc(&tx_dma_desc[0], cfg->tx_dma_cfg, &tx_dma_desc[1], 0);
    ASSERT(dret == HAL_DMA_OK, "SensorEng: Failed to init tx desc 0");

    clr_irq_val = 0x1;
    clr_dma_cfg = *cfg->tx_dma_cfg;
    clr_dma_cfg.dst_bsize = HAL_DMA_BSIZE_1;
    clr_dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
    clr_dma_cfg.src_bsize = HAL_DMA_BSIZE_1;
    clr_dma_cfg.src_tsize = 1;
    clr_dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
    clr_dma_cfg.try_burst = 0;
    if (cfg->trigger_type == HAL_SENSOR_ENGINE_TRIGGER_GPIO) {
        dret = hal_dma_init_desc(&tx_dma_desc[1], &clr_dma_cfg, (struct HAL_DMA_DESC_T *)((uint32_t)&tx_dma_desc[0] | (1 << 1)), 0);
    } else {
        dret = hal_dma_init_desc(&tx_dma_desc[1], &clr_dma_cfg, &tx_dma_desc[0], 0);
    }
    ASSERT(dret == HAL_DMA_OK, "SensorEng: Failed to init tx desc 1");
    tx_dma_desc[1].src = (uint32_t)&clr_irq_val;
    tx_dma_desc[1].dst = (uint32_t)&sensor_eng[0]->SENSOR_INTR_CLR;

    dret = hal_gpdma_sg_start(&tx_dma_desc[0], cfg->tx_dma_cfg);
    ASSERT(dret == HAL_DMA_OK, "SensorEng: Failed to start tx dma");

    if (dma_rx_len) {
        ASSERT(cfg->rx_dma_cfg, "Bad rx_dma_cfg");

        rx_dma_cfg = *cfg->rx_dma_cfg;
        for (i = 0; i < cfg->rx_burst_cnt; i++) {
            rx_dma_cfg.dst = (uint32_t)(dma_rx_buf + cfg->rx_burst_len * i);
            if (i + 1 == cfg->rx_burst_cnt) {
                dret = hal_dma_init_desc(&rx_dma_desc[i], &rx_dma_cfg, &rx_dma_desc[0], 0);
            } else {
                dret = hal_dma_init_desc(&rx_dma_desc[i], &rx_dma_cfg, &rx_dma_desc[i + 1], 0);
            }
            ASSERT(dret == HAL_DMA_OK, "SensorEng: Failed to init rx desc %d", i);
        }
        hal_gpdma_sg_start(&rx_dma_desc[0], cfg->rx_dma_cfg);
        ASSERT(dret == HAL_DMA_OK, "SensorEng: Failed to start rx dma");
    }

    hal_cmu_clock_enable(HAL_CMU_MOD_H_SENSOR_HUB);
    hal_cmu_reset_clear(HAL_CMU_MOD_H_SENSOR_HUB);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_TIMER2);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_TIMER2);

    sensor_eng[0]->SLV0_CONFIG_REG = ((cfg->device_address & 0x3FF) << 1) | 1;
    sensor_eng[0]->GLOBAL_CFG1_REG &= ~0xf;
    if (dma_rx_len) {
        //RX LLI = 1
        sensor_eng[0]->GLOBAL_CFG1_REG |= 1;
    }
    //TX LLI = 2
    sensor_eng[0]->GLOBAL_CFG3_REG &= ~0xf;
    if (cfg->trigger_type == HAL_SENSOR_ENGINE_TRIGGER_TIMER) {
        sensor_eng[0]->GLOBAL_CFG3_REG |= 2;
    } else if (cfg->trigger_type == HAL_SENSOR_ENGINE_TRIGGER_GPIO) {
        sensor_eng[0]->GLOBAL_CFG3_REG |= 1;
        sensor_eng[0]->SLV0_INTR_MASK = ~(1 << cfg->trigger_gpio);
    }

    sensor_eng[0]->GLOBAL_CFG0_REG = (cfg->trigger_type == HAL_SENSOR_ENGINE_TRIGGER_GPIO ? (1<<0) : (0<<0)) |
                                      (cfg->device == HAL_SENSOR_ENGINE_DEVICE_I2C0 ? (0<<2) : (1<<2))|
                                      (1<<3);

    if (cfg->trigger_type == HAL_SENSOR_ENGINE_TRIGGER_TIMER) {
        hal_sensor_eng_timer_open(US_TO_TICKS(cfg->period_us));
        hal_sensor_eng_timer_start();
    }

    hal_sensor_eng_irq_enable();

    return 0;
}

int hal_sensor_engine_close(enum HAL_SENSOR_ENGINE_ID_T id)
{
    uint32_t cnt;

    hal_sensor_eng_irq_disable();

    hal_sensor_eng_timer_stop();

    sensor_eng[0]->GLOBAL_CFG0_REG &= ~(1<<3);

    // Wait until sensor engine becomes idle
    cnt = 0;
    while ((sensor_eng[0]->SENSOR_STATUS & SENSOR_ENG_FSM_MASK) && cnt < 10) {
        osDelay(1);
        cnt++;
    }
    if (sensor_eng[0]->SENSOR_STATUS & SENSOR_ENG_FSM_MASK) {
        ASSERT(false, "%s: Sensor engine cannot become idle: 0x%08X", __func__, sensor_eng[0]->SENSOR_STATUS);
    }

    sensor_eng[0]->SLV0_INTR_MASK = ~0UL;

    dma_rx_len = 0;

    hal_cmu_reset_set(HAL_CMU_MOD_O_TIMER2);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_TIMER2);
    hal_cmu_reset_set(HAL_CMU_MOD_H_SENSOR_HUB);
    hal_cmu_clock_disable(HAL_CMU_MOD_H_SENSOR_HUB);

    return 0;
}

#if 0
#include "hal_i2c.h"

#define READ_BUF_BURST_CNT 5
#define TRIGGER_TIMER_US 500000
#define LOCK_INTR_CNT 3
#define TRIGGER_GPIO_PIN HAL_GPIO_PIN_P0_0

static void hal_sensor_eng_external_input_pin_irqhandler(enum HAL_GPIO_PIN_T pin)
{
    bool gpio_lvl = 0;
    gpio_lvl = hal_gpio_pin_get_val(pin);
    TRACE("hal_sensor_eng_external_irqhandler io=%d val=%d", pin, gpio_lvl);
}

static void sensor_timer_test_handler(enum HAL_I2C_ID_T id, const uint8_t *buf, uint32_t len)
{
    TRACE("%s: id=%d len=%u %02X-%02X-%02X-%02x", __FUNCTION__, id, len, buf[0], buf[1], buf[2], buf[3]);
}

static void sensor_gpio_test_handler(enum HAL_I2C_ID_T id, const uint8_t *buf, uint32_t len)
{
    TRACE("%s: id=%d len=%u %02X-%02X-%02X-%02x", __FUNCTION__, id, len, buf[0], buf[1], buf[2], buf[3]);
}

static int sensor_eng_test(enum HAL_SENSOR_ENGINE_TRIGGER_T trigger_type)
{
    static uint8_t write_buf[3];
    static uint8_t read_buf[READ_BUF_BURST_CNT][sizeof(write_buf) * 2];

    int ret;
    struct HAL_I2C_CONFIG_T cfg;
    struct HAL_I2C_SENSOR_ENGINE_CONFIG_T se_cfg;
    enum HAL_I2C_ID_T id;
    uint32_t i;

    id = HAL_I2C_ID_0;
    if (id == HAL_I2C_ID_0) {
        hal_iomux_set_i2c0();
    } else {
        hal_iomux_set_i2c1();
    }

    for (i = 0; i < sizeof(write_buf); i++) {
        write_buf[i] = i;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.mode = HAL_I2C_API_MODE_SENSOR_ENGINE;
    cfg.as_master = 1;
    cfg.use_dma = 1;
    cfg.use_sync = 1;
    cfg.speed = 100000;
    ret = hal_i2c_open(id, &cfg);
    if (ret) {
        TRACE("%s: i2c open failed %d", __func__, ret);
        return ret;
    }

    memset(&se_cfg, 0, sizeof(se_cfg));
    se_cfg.id = HAL_SENSOR_ENGINE_ID_0;
    se_cfg.trigger_type = trigger_type;
    if (trigger_type == HAL_SENSOR_ENGINE_TRIGGER_TIMER) {
        se_cfg.period_us = TRIGGER_TIMER_US;
        se_cfg.handler = sensor_timer_test_handler;
    } else {
        se_cfg.trigger_gpio = TRIGGER_GPIO_PIN;
        se_cfg.handler = sensor_gpio_test_handler;
    }
    se_cfg.target_addr = 0x17;
    se_cfg.write_buf = &write_buf[0];
    se_cfg.write_txn_len = 1;
    se_cfg.read_buf = &read_buf[0][0];
    se_cfg.read_txn_len = 2;
    se_cfg.txn_cnt = sizeof(write_buf);
    se_cfg.read_burst_cnt = READ_BUF_BURST_CNT;

    hal_i2c_sensor_engine_start(id, &se_cfg);

    uint32_t lock = int_lock();
    while (1) {
        hal_sys_timer_delay(US_TO_TICKS(TRIGGER_TIMER_US) * LOCK_INTR_CNT);
        TRACE_IMM("%s: delay %u ms", __func__, (TRIGGER_TIMER_US / 1000 * LOCK_INTR_CNT));
        hal_sensor_process_rx_buffer();
    }
    int_unlock(lock);

    return 0;
}

int hal_sensor_eng_periodic_timer_test(void)
{
    sensor_eng_test(HAL_SENSOR_ENGINE_TRIGGER_TIMER);
    return 0;
}

int hal_sensor_eng_external_input_test(void)
{
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP ext_pin[] = {
        {TRIGGER_GPIO_PIN, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };
    struct HAL_GPIO_IRQ_CFG_T gpiocfg;

    sensor_eng_test(HAL_SENSOR_ENGINE_TRIGGER_GPIO);

    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)ext_pin, sizeof(ext_pin)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
    hal_gpio_pin_set_dir(TRIGGER_GPIO_PIN, HAL_GPIO_DIR_IN, 1);

    gpiocfg.irq_enable = true;
    gpiocfg.irq_debounce = false;
    gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_HIGH_RISING;
    gpiocfg.irq_handler = hal_sensor_eng_external_input_pin_irqhandler;
    gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;

    hal_gpio_setup_irq(TRIGGER_GPIO_PIN, &gpiocfg);

    TRACE("SLV0:%08x CFG1:0x%08x CFG3:0x%08x CFG0:0x%08x INTR_MASK:0x%08x", 
        sensor_eng[0]->SLV0_CONFIG_REG,
        sensor_eng[0]->GLOBAL_CFG1_REG, 
        sensor_eng[0]->GLOBAL_CFG3_REG, 
        sensor_eng[0]->GLOBAL_CFG0_REG, 
        sensor_eng[0]->SLV0_INTR_MASK);
    return 0;
}
#endif

