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
#include "hal_i2c.h"
#include "hal_uart.h"
#include "bt_drv.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "cmsis.h"
#include "hal_cmu.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "tgt_hardware.h"
#include "bt_drv_internal.h"
#include "bt_drv_2300p_internal.h"
#include "hal_chipid.h"
#include "bt_drv_interface.h"
#include "pmu.h"
#define AUTO_CAL                                0

#ifndef BT_RF_MAX_XTAL_TUNE_PPB
// Default 10 ppm/bit or 10000 ppb/bit
#define BT_RF_MAX_XTAL_TUNE_PPB                 10000
#endif

#ifndef BT_RF_XTAL_TUNE_FACTOR
// Default 0.2 ppm/bit or 200 ppb/bit
#define BT_RF_XTAL_TUNE_FACTOR                  200
#endif

#define DEFAULT_XTAL_FCAP                       0x8080

#define XTAL_FCAP_NORMAL_SHIFT                  0
#define XTAL_FCAP_NORMAL_MASK                   (0xFF << XTAL_FCAP_NORMAL_SHIFT)
#define XTAL_FCAP_NORMAL(n)                     BITFIELD_VAL(XTAL_FCAP_NORMAL, n)

#define BT_XTAL_CMOM_DR                         (1 << 13)

#define RF_REG_XTAL_FCAP                        0xE9
#define RF_REG_XTAL_CMOM_DR                     0xE8

/* ljh add for sync modify rf RF_REG_XTAL_FCAP value*/


#define SPI_TRIG_RX_NEG_TIMEOUT                 MS_TO_TICKS(3)

#define SPI_TRIG_NEG_TIMEOUT                    SPI_TRIG_RX_NEG_TIMEOUT



enum BIT_OFFSET_CMD_TYPE_T {
    BIT_OFFSET_CMD_STOP = 0,
    BIT_OFFSET_CMD_START,
    BIT_OFFSET_CMD_ACK,
};

static uint16_t xtal_fcap = DEFAULT_XTAL_FCAP;
static uint16_t init_xtal_fcap = DEFAULT_XTAL_FCAP;

#ifndef BTDRV_XTAL_FCAP_DIRECT_UPDATE
static uint32_t spi_trig_time;
#endif


//rampup start     1--->2->3 rampup ok
//rampdown start 3-->2-->1-->0  rampdown ok


//3a06=3b3f
//for pa@1.8v
struct bt_drv_tx_table_t{
    uint16_t tbl[16][3];
};


struct RF_SYS_INIT_ITEM {
    uint16_t reg;
    uint16_t set;
    uint16_t mask;
    uint16_t delay;
};

static const struct RF_SYS_INIT_ITEM rf_sys_init_tbl[] =
{
    {0xb5,0x8000,0x8000,0},
    {0xc2,0x7188,0x7fff,0},
#ifdef LBRT
    {0xc4,0x0043,0x0043,0},//enalbe lbrt adc clk[6]
#else
    {0xc4,0x0003,0x0003,0},
#endif    
};


const uint16_t rf_init_tbl_1[][3] =
{
    {0x88,0x8640,0},
    {0x8b,0x8a4a,0},//set rx flt cap,filter start up
#if defined(__FANG_HW_AGC_CFG_ADC__)    
    {0xd1,0x8401,0},//set ra adc gain -3db  
    {0x8e,0x0D28,0},//adc noise reduction
#else
    {0xd1,0x8403,0},//set gain  
    {0x8e,0x0128,0},//adc noise reduction
#endif   
    {0x90,0x8e1f,0},//enlarge txflt BW
    {0x91,0x05c0,0},//by walker 20180427
    //{0x92,0x688c,0},//update by  walker 2018.07.30
    {0x92,0x668c,0},//update by  luobin 2019/3/22
    {0x97,0x2523,0},//update by  luobin 2019.0523
    {0x98,0x1324,0},//update by  walker 2019.01.14, modify for yield
    {0x9a,0x4470,0},//div2 rc
    {0x9b,0xfd42,0},//update by  walker 2018.10.24
    {0x9c,0x180f,0},/////////luobin
    {0x9d,0x086c,0},//enhance xtal drv
    {0xa3,0x0789,0},
    {0xb0,0x0000,0},
    {0xb1,0x0000,0},
    {0xb3,0x0000,0},//
    {0xb4,0x883c,0},
    {0xb6,0x3156,0},
    {0xb7,0x183f,0},//update by  walker 2018.07.30
    {0xb9,0x8000,0},//cap3g6 off
    {0xba,0x104e,0},
#if defined(__FANG_LNA_CFG__)
    {0xac,0x080e,0},//pre-charge set to 5us
    {0x85,0x7f00,0},//NFMI RXADC LDO rise up
    {0xc3,0x0068,0},//add by xrz increase pll cal time 2018/10/20
#else
    {0xc3,0x0050,0},
#endif
    {0xc5,0x4b50,0},//vco ictrl dr
    {0xc9,0x3a08,0},//vco ictrl
    {0xd3,0xc1c1,0},
    {0xd4,0x000f,0},
    {0xd5,0x4000,0},
    {0xd6,0x7980,0},
    {0xe8,0xe000,0},
    {0xf3,0x0c41,0},//by fang
    {0x1a6,0x0600,0},
    {0x1ae,0x6800,0},//update by  walker 2019.03.23
    {0x1d4,0x0000,0},
    {0x1d7,0xc4ff,0},//update by  walker 2018.10.10
    {0x1de,0x2000,0},
    {0x1df,0x2087,0},
    {0x1f4,0x2241,0},//by walker 20180427
    {0x1fa,0x03df,0},//rst needed
    {0x1fa,0x83df,0},
    {0xeb,0x000c,0},//gain_idx:0
    {0x181,0x004f,0},
    {0xec,0x002d,0},//gain_idx:1
    {0x182,0x004f,0},
    {0xed,0x025,0},//gain_idx:2
    {0x183,0x004f,0},
    {0xee,0x0127,0},//gain_idx:3
    {0x184,0x004f,0},
    {0xef,0x0377,0},//gain_idx:4
    {0x185,0x004f,0},
    {0xf0,0x0387,0},//gain_idx:5
    {0x186,0x004f,0},
    {0xf1,0x03f7,0},//gain_idx:6
    {0x187,0x004d,0},
    {0xf2,0x03f7,0},//gain_idx:7
    {0x188,0x0049,0},
    {0x1d6,0x7e58,0},//set lbrt rxflt dccal_ready=1       
    {0x1c8,0xdfc0,0},//en rfpll bias in lbrt mode 
    {0x1c9,0xa0cf,0},//en log bias 
    {0x82,0x0caa,0}, // select rfpll and cplx_dir=0 and en rxflt test
    {0xf4,0x2181,0}, // set flt_cap to match flt_gain                
#if defined(__FANG_HW_AGC_CFG__)
    {0x8A,0x6EA4,0},//  use rfpll for nfmi adclk    
#elif defined(__BT_NEW_RF__)
    {0x8a,0x7ea4,0},//  use rfpll for nfmi adclk   
#else
    {0x8a,0x4ea4,0},//  use rfpll for nfmi adclk
#endif
    {0x86,0xdc99,0},//set nfmi tmix                         
    {0xdf,0x2006,0},//set nfmi tmix vcm   
#ifdef __HW_AGC__
    {0x1c7,0x007d,0},//open lbrt adc and vcm bias current
    {0xad,0xa04a,1},//hwagc en=1
#if defined(__FANG_HW_AGC_CFG__)
    {0xEB,0x033D,0},//
    {0xEC,0x003D,0},//
    {0xCD,0x0040,0},//
#else    
    {0xcd,0x00c0,0},//set pdt_pga_gain
    {0xeb,0x0037,0},//0=-27dB lna gain index
    {0xec,0x005d,0},//-19dB 1
#endif
    {0xed,0x000d,0},//-10dB 2
    {0xee,0x000d,0},//-10dB 3
    {0xef,0x000d,0},//-10dB 4
    {0xf0,0x000d,0},//-10dB 5
    {0xf1,0x000d,0},//-10dB 6
    {0xf2,0x000c,0},//0dB 7 lna gain index
#if 0 
    {0xa2,0x67c2,0},//open test buffer
    {0x1c4,0xffcf,0},//pu tst buf ibias at bt mode
    {0x1c5,0xf8ff,0},//pu tst buf ibias at bt mode
    {0x8c,0x5000,0},//select i2v output of bt rxflt
#endif    
    {0xcf,0x7f32,0},//lna gain dr=0
    {0xdf,0x2006,0},//lna gain dr=0
    {0xd0,0xe91f,0},//i2v flt gain dr=0
    {0xE9,0x8020,0},//FCAP
#endif //__HW_AGC__   

#ifndef LBRT
    {0x189,0x000a,0},//min tx gain
    {0x18a,0x0006,0},// update by  walker 2018.07.10
    {0x18b,0x0001,0},//max tx gain  luobin 2018.09.11
#else 
    //[below only lbrt ]
    {0xd0,0xe91f,0},//set nfmi pa ibit                               
    {0xcf,0x7f32,0},//set nfmi pa eff  
    {0x189,0x000f,0},//min tx gain
    {0x18a,0x000c,0},// update by  walker 2018.07.10
    {0x18b,0x0008,0},//max tx gain 
#endif //LBRT
    {0x18c,0x0000,0},//max tx gain lbrt
    //air @ LBRT 31.5m
    {0x1d8,0x9162,0},//lbrt tx freqword 
    {0x1d9,0x9162,0},//lbrt tx freqword
    {0x1da,0x9162,0},//lbrt tx freqword
    {0x1db,0x9162,0},//lbrt tx freqword
    {0x1dc,0x0000,0},//txlccap
    {0x1dd,0x0000,0},//txlccap
    {0x1ed,0x0000,0},//rxlccap
    {0x1ee,0x0000,0},//rxlccap
    {0x1e9,0x0a0a,0},//set rfpll divn in lbrt tx mode                 
    {0x1ea,0x0a0a,0},
    {0x1eb,0x0a0a,0},//set rfpll divn in lbrt rx mode                 
    {0x1ec,0x0a0a,0},  
    {0x1ef,0x8a76,0},//lbrt rx freqword
    {0x1f0,0x8a76,0},//lbrt rx freqword
    {0x1f1,0x8a76,0},//lbrt rx freqword
    {0x1f2,0x8a76,0},//lbrt rx freqword
    {0x87,0x9f00,0},//{0x87,0x9f40,0}
    {0x81,0x9207,0},
    {0xce,0xfc08,0},
    {0x8A,0x4EA4,0},//  use rfpll for nfmi adclk   
    //[below for ibrt]
    {0x8c,0x9100,0},
    {0xab,0x1c0c,0}, //2019.05.08 by luofei
    {0xa5,0x0807,0}, //2019.05.08 by luofei
    {0xa6,0x2D20,0}, //2019.05.23 by luobin
    {0xa8,0x2D22,0}, //2019.05.23 by luobin
    {0x1b7,0x2c00,0}, //2019.05.08 by luofei
    {0x1ad,0x9000,0}, // 2M tx filter baseband
    {0xbf,0x9400,0}, //x4 enable
};

void btdrv_rf_set_conidx(uint32_t conidx)
{
    return;

}


uint8_t btdrv_rf_get_conidx(void)
{
    return 0;//current_conidx;
}



///tws triggle
void btdrv_rf_trig_patch_enable(bool enable)
{
        return;

}

void btdrv_tws_trig_role(uint8_t role)
{
    return;
}


uint32_t btdrv_rf_get_max_xtal_tune_ppb(void)
{
    return BT_RF_MAX_XTAL_TUNE_PPB;
}


uint32_t btdrv_rf_get_xtal_tune_factor(void)
{
    return BT_RF_XTAL_TUNE_FACTOR;
}

  

void btdrv_rf_init_xtal_fcap(uint32_t fcap)
{
    xtal_fcap = SET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL, fcap);
    btdrv_write_rf_reg(RF_REG_XTAL_FCAP, xtal_fcap);
    init_xtal_fcap = xtal_fcap;
}

uint32_t btdrv_rf_get_init_xtal_fcap(void)
{
    return GET_BITFIELD(init_xtal_fcap, XTAL_FCAP_NORMAL);
}

uint32_t btdrv_rf_get_xtal_fcap(void)
{
    return GET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL);
}

void btdrv_rf_set_xtal_fcap(uint32_t fcap , uint8_t is_direct)
{
    xtal_fcap = SET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL, fcap);
    if(is_direct)
    {
    //    TRACE("0x40086044 = %x",*(uint32_t *)0x40086044);
        hal_iomux_ispi_access_disable(HAL_IOMUX_ISPI_BT_RF);
        hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_MCU_RF);
        btdrv_write_rf_reg(RF_REG_XTAL_FCAP, xtal_fcap);
        hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_BT_RF);
    }
    else
    {
      //  btdrv_write_rf_reg(RF_REG_XTAL_FCAP, xtal_fcap);
        uint32_t val;
        val = ((((xtal_fcap & 0xFF00) >> 8) | ((xtal_fcap & 0xFF) << 8))<<8)|RF_REG_XTAL_FCAP;
        btdrv_spi_trig_data_change(0,0,val);
        btdrv_spi_trig_data_set(0);
        btdrv_set_spi_trig_enable();
        spi_trig_time = hal_sys_timer_get();
    }
}


int btdrv_rf_xtal_fcap_busy(uint8_t is_direct)
{
    if(is_direct)
    {
        return 0;
    }
    else
    {
        if (btdrv_get_spi_trig_enable()) {
            if (hal_sys_timer_get() - spi_trig_time > SPI_TRIG_NEG_TIMEOUT) {
                btdrv_clear_spi_trig_enable();
    //            TRACE("btdrv_rf_xtal_fcap_busy: not busy");
                return 0;
            }
            else{
    //            TRACE("btdrv_rf_xtal_fcap_busy: busy");
                return 1;
            }
        }
    }
    return 0;
}

void btdrv_rf_bit_offset_track_enable(bool enable)
{
    return;

}

void btdrv_rf_bit_offset_track_force_disable(void)
{
}


uint32_t btdrv_rf_bit_offset_get(void)
{
    return 0;
}

void btdrv_xtal_rf_init(void)
{

}

void btdrv_rf_tx_init(int idx)
{

}


uint16_t btdrv_rf_bitoffset_get(uint8_t conidx)
{
    uint16_t bitoffset;
    bitoffset = BTDIGITAL_REG(EM_BT_BITOFF_ADDR+conidx*BT_EM_SIZE) & 0x3ff;
    return bitoffset;
}


uint16_t bt_drv_reg_op_get_bitoffset(uint16_t conhandle)
{
    //sanity check
    if(conhandle<0x80||conhandle>0x83){
        TRACE("ERROR, conhdle=%x\n",conhandle);
        return 0;
    }

    uint8_t link_id = btdrv_conhdl_to_linkid(conhandle);
    uint32_t acl_par_ptr = 0;
    uint16_t bitoffset = 0;
    
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0){
        acl_par_ptr = *(uint32_t *)(0xc000098c+link_id*4);
    }
    
    if(acl_par_ptr != 0){
        bitoffset = ((*(uint32_t *)(acl_par_ptr+150))&0xFFFF0000)>>16;
    }else{
        TRACE("ERROR, bitoffset address\n");
    }
    
    return bitoffset;
}

void btdrv_rf_log_delay_cal(void)
{
    unsigned short read_value;
    unsigned short write_value;
    TRACE("btdrv_rf_log_delay_cal\n");

    btdrv_write_rf_reg(0xa7, 0x0028); 

    btdrv_write_rf_reg(0xd4, 0x0000); 
    btdrv_write_rf_reg(0xd5, 0x4002); 
 //   *(volatile uint32_t*)(0xd02201e4) = 0x000a00b0;
    *(volatile uint32_t*)(0xd0340020) = 0x030e01c1;
    TRACE("0xd0340020 =%x\n",*(volatile uint32_t*)(0xd0340020) );

     btdrv_delay(1);

    btdrv_write_rf_reg(0xd2, 0x5003); 

    btdrv_delay(1);
    
    btdrv_read_rf_reg(0x1e2, &read_value);
    TRACE("0x1e2 read_value:%x\n",read_value);
    if(read_value == 0xff80){
        btdrv_write_rf_reg(0xd3, 0xffff); 
    }else{
        write_value = ((read_value>>7)&0x0001) | ((read_value & 0x007f)<<1) | ((read_value&0x8000)>>7) | ((read_value&0x7f00)<<1);
        TRACE("d3 write_value:%x\n",write_value);
        btdrv_write_rf_reg(0xd3, write_value); 
    }
    btdrv_delay(1);

//    *(volatile uint32_t*)(0xd02201e4) = 0x00000000;
    *(volatile uint32_t*)(0xd0340020) = 0x010e01c0;
    TRACE("0xd0340020 =%x\n",*(volatile uint32_t*)(0xd0340020) );


    btdrv_write_rf_reg(0xd4, 0x000f); 
    btdrv_write_rf_reg(0xd2, 0x1003); 
    btdrv_write_rf_reg(0xd5, 0x4000); 

}




void btdrv_rf_rx_gain_adjust_req(uint32_t user, bool lowgain)
{
    return;

}

bool btdrv_rf_rx_gain_adjust_getstatus(void)
{
        return true;
}

void btdrv_rf_rx_gain_adjust_init(void)
{

        return;

}


bool btdrv_rf_blerx_gain_adjust_getstatus(void)
{
        return true;
}

void btdrv_rf_blerx_gain_adjust_default(void)
{
        return;
}

void btdrv_rf_blerx_gain_adjust_lowgain(void)
{
        return;
}

//rf Image calib
void btdtv_rf_image_calib(void)
{

        uint16_t read_val;
        //read calibrated val from efuse 0x05 register         
        pmu_get_efuse(PMU_EFUSE_PAGE_SW_CFG, &read_val);
        //check if bit 11 has been set 
        uint8_t calb_done_flag = ((read_val &0x800)>>11);
        if(calb_done_flag)
        {
            TRACE("EFUSE REG[5]=%x",read_val);
        }
        else
        {
            TRACE("EFUSE REG[5] rf image has not been calibrated!");
            return;
        }
        //[bit 12] calib flag 
        uint8_t calib_val = ((read_val &0x1000)>>12);
        btdrv_read_rf_reg(0x9b,&read_val);
        read_val&=0xfcff;
        
        if(calib_val==0)
        {
            read_val|= 1<<8;
        }
        else if(calib_val== 1)
        {
            read_val|= 1<<9;
        }
        
        TRACE("write rf image calib val=%x in REG[0x9b]", read_val);
        btdrv_write_rf_reg(0x9b,read_val);
}

uint8_t btdrv_rf_init(void)
{
    uint16_t value;
    const uint16_t (*rf_init_tbl_p)[3];
    uint32_t tbl_size;
    //uint8_t ret;
    uint8_t i;

    for (i = 0; i < ARRAY_SIZE(rf_sys_init_tbl); i++) {
        btdrv_read_rf_reg(rf_sys_init_tbl[i].reg, &value);
        value = (value & ~rf_sys_init_tbl[i].mask) | (rf_sys_init_tbl[i].set & rf_sys_init_tbl[i].mask);
        if (rf_sys_init_tbl[i].delay) {
            btdrv_delay(rf_sys_init_tbl[i].delay);
        }
        btdrv_write_rf_reg(rf_sys_init_tbl[i].reg, value);
    }

    rf_init_tbl_p = &rf_init_tbl_1[0];
    tbl_size = sizeof(rf_init_tbl_1)/sizeof(rf_init_tbl_1[0]);
    
    for(i=0;i< tbl_size;i++)
    {
        btdrv_write_rf_reg(rf_init_tbl_p[i][0],rf_init_tbl_p[i][1]);
        if(rf_init_tbl_p[i][2] !=0)
            btdrv_delay(rf_init_tbl_p[i][2]);//delay
        btdrv_read_rf_reg(rf_init_tbl_p[i][0],&value);
        //TRACE("reg=%x,v=%x",rf_init_tbl_p[i][0],value);
    }

    //need before rf log delay cal 
    btdtv_rf_image_calib();



    btdrv_rf_log_delay_cal();
    
  
    btdrv_spi_trig_init();

    btdrv_rf_rx_gain_adjust_init();

    return 1;
}

void bt_drv_rf_reset(void)
{
    enum HAL_IOMUX_ISPI_ACCESS_T access;
    access = hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_MCU_RF);

    btdrv_write_rf_reg(0x80,0xcafe);

    btdrv_write_rf_reg(0x80,0x5fee);

    if ((access & HAL_IOMUX_ISPI_MCU_RF) == 0) {
        hal_iomux_ispi_access_disable(HAL_IOMUX_ISPI_MCU_RF);
    }
}
