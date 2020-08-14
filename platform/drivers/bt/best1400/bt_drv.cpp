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
#include <stdio.h>
#include "string.h"
#include "plat_types.h"
#include "plat_addr_map.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "bt_drv.h"
#include "bt_drv_internal.h"
#include "bt_drv_1400_internal.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_interface.h"
#include "hal_timer.h"
#include "hal_intersys.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_sysfreq.h"
#include "hal_chipid.h"
#include "hal_iomux.h"
#include "hal_gpio.h"
#include "pmu.h"
#include "nvrecord_dev.h"

bool btdrv_dut_mode_enable = false;
static bool hci_has_opened = false;
static volatile uint32_t btdrv_tx_flag = 1;/*only one packet can be sent before callback*/
static uint32_t dut_connect_status = DUT_CONNECT_STATUS_DISCONNECTED;

/*****************************************************************************
 Prototype    : btdrv_tx
 Description  : mcu tx done callback function
 Input        : const unsigned char *data
                unsigned int len
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/11/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_tx(const unsigned char *data, unsigned int len)
{
    DRV_PRINT("tx");
    btdrv_tx_flag = 1;
}
/*****************************************************************************
 Prototype    : btdrv_dut_accessible_mode_manager
 Description  : DUT mode signaling test rx hook function
 Input        : const unsigned char *data
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/11/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_dut_accessible_mode_manager(const unsigned char *data)
{
    if(btdrv_dut_mode_enable)
    {
        if(data[0]==0x04&&data[1]==0x03&&data[2]==0x0b&&data[3]==0x00)
        {
            bt_drv_reg_op_set_accessible_mode(0);
            btdrv_disable_scan();
            dut_connect_status = DUT_CONNECT_STATUS_CONNECTED;
        }
        else if(data[0]==0x04&&data[1]==0x05&&data[2]==0x04&&data[3]==0x00)
        {
            btdrv_enable_dut();
            dut_connect_status = DUT_CONNECT_STATUS_DISCONNECTED;
        }
    }
}
/*****************************************************************************
 Prototype    : btdrv_rx
 Description  : mcu rx callback function
 Input        : const unsigned char *data
                unsigned int len
 Output       : None
 Return Value : static
 Calls        :
 Called By    :

 History        :
 Date         : 2018/11/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
static unsigned int btdrv_rx(const unsigned char *data, unsigned int len)
{
    hal_intersys_stop_recv(HAL_INTERSYS_ID_0);

    DRV_PRINT("%s len:%d", __func__, len);
    DRV_DUMP("%02x ", data, len>7?7:len);
    btdrv_dut_accessible_mode_manager(data);
    hal_intersys_start_recv(HAL_INTERSYS_ID_0);

    return len;
}
/*****************************************************************************
 Prototype    : btdrv_enable_jtag
 Description  : bes1400 Jtag enable function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/11/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_enable_jtag(void)
{
    *(uint32_t*)0x400000F8 &= 0x7FFFFFFF;//clear bit31

    hal_iomux_set_jtag();
    hal_cmu_jtag_enable();
    hal_cmu_jtag_clock_enable();
}
/*****************************************************************************
 Prototype    : btdrv_SendData
 Description  : btdrv send data api
 Input        : const uint8_t *buff
                uint8_t len
 Output       : None
 Return Value : extern
 Calls        :
 Called By    :

 History        :
 Date         : 2018/11/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
extern "C" void btdrv_SendData(const uint8_t *buff,uint8_t len)
{
    btdrv_tx_flag = 0;

    hal_intersys_send(HAL_INTERSYS_ID_0, HAL_INTERSYS_MSG_HCI, buff, len);
    DRV_PRINT("%s", __func__);
    DRV_DUMP("%02x ", buff, len);
    while( (btdrv_dut_mode_enable==0) && btdrv_tx_flag == 0);
}
/*****************************************************************************
 Prototype    : btdrv_hciopen
 Description  : btdrv hci open function, intersys open
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/11/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_hciopen(void)
{
    int ret = 0;

    if (hci_has_opened)
    {
        return;
    }

    hci_has_opened = true;

    if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0)
    {
        hal_intersys_peer_irq_auto_clear(true);
    }

    ret = hal_intersys_open(HAL_INTERSYS_ID_0, HAL_INTERSYS_MSG_HCI, btdrv_rx, btdrv_tx, false);

    if (ret)
    {
        DRV_PRINT("Failed to open intersys");
        return;
    }

    hal_intersys_start_recv(HAL_INTERSYS_ID_0);
}
/*****************************************************************************
 Prototype    : btdrv_hcioff
 Description  : btdrv hci off function, intersys close
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/11/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_hcioff(void)
{
    if (!hci_has_opened)
    {
        return;
    }
    hci_has_opened = false;

    hal_intersys_close(HAL_INTERSYS_ID_0,HAL_INTERSYS_MSG_HCI);
}
/*****************************************************************************
 Prototype    : btdrv_poweron
 Description  : bt controller power on function
 Input        : uint8_t en
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/11/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_poweron(uint8_t en)
{
    //power on bt controller
    if(en)
    {
        hal_cmu_bt_clock_enable();
        hal_cmu_bt_reset_clear();
        hal_cmu_bt_module_init();
        btdrv_delay(10);
        if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1)
        {
            BTDIGITAL_REG(0xc0000050)=0x42;
            btdrv_delay(100);
        }
    }
    else
    {
        btdrv_delay(10);
        hal_cmu_bt_reset_set();
        hal_cmu_bt_clock_disable();
    }
}
/*****************************************************************************
 Prototype    : btdrv_hwagc_1400_dccal
 Description  : btdrv dc calibration function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/12/3
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_hwagc_1400_dccal(void)
{
//    btdrv_write_rf_reg(0xce,0x3001);//filter pdt reset dr=1
    btdrv_write_rf_reg(0x41,0x9908);//LNA_att, i2v_gain dr=1
    btdrv_write_rf_reg(0x4f,0x010f);//LNA_gain dr=1
    btdrv_write_rf_reg(0x42,0x7e01);//filter_gain dr=1

    btdrv_write_rf_reg(0x27,0x0030);//rx input pull down
    btdrv_write_rf_reg(0x7,0x4124);//rx input pull down

    BTDIGITAL_REG(0xd02201e4) = 0x00000000;//rx continue
    btdrv_delay(10);
    BTDIGITAL_REG(0xd02201e4) = 0x000a0080;
    btdrv_delay(10);

    for (uint8_t dc_cal_counter = 0; dc_cal_counter < 3; dc_cal_counter++)
    {
        short value_signed = 0;
        switch(dc_cal_counter)
        {
            case 0:
                btdrv_write_rf_reg(0xcf,0x0864);//select LNA pdt
                btdrv_write_rf_reg(0xd0,0x5504);//select LNA pdt
                break;
            case 1:
                btdrv_write_rf_reg(0xcf,0x0f64);//select i2v pdt
                btdrv_write_rf_reg(0xd0,0x5404);//select i2v pdt
                break;
            case 2:
                btdrv_write_rf_reg(0xcf,0x0b64);//select filter pdt
                btdrv_write_rf_reg(0xd0,0x5404);//select filter pdt
                break;
            default:
                break;
        }

        for(uint8_t i = 0; i < 10; i++)
        {
            unsigned short first_val;
            btdrv_write_rf_reg(0xc6,0x3cc1);//sample DC value
            btdrv_delay(1);
            btdrv_read_rf_reg(0xc7,&first_val);//read DC value
            TRACE("btdrv_hwagc_1400_dccal first_val = 0x%x\n",first_val);
            btdrv_write_rf_reg(0xc6,0x1cc1);//reset sample bit
            btdrv_delay(1);
        }

        for(uint8_t i = 0; i < 10; i++)
        {
            unsigned short value_tmp;
            btdrv_write_rf_reg(0xc6,0x3cc1);//sample DC value
            btdrv_delay(1);
            btdrv_read_rf_reg(0xc7,&value_tmp);//read DC value
            TRACE("btdrv_hwagc_1400_dccal value = 0x%x\n",(uint8_t)value_tmp&0xFF);
            btdrv_write_rf_reg(0xc6,0x1cc1);//reset sample bit
            btdrv_delay(1);
            value_signed += (int8_t)(value_tmp&0xFF);
        }

        value_signed /= 10;
        TRACE("btdrv_hwagc_1400_dccal dc_cal_counter = %d, average value_after = %x\n",dc_cal_counter, (uint8_t)value_signed);

        switch(dc_cal_counter)
        {
            case 0:
                btdrv_write_rf_reg(0xc8,value_signed);//write LNA DC value
                break;
            case 1:
                btdrv_write_rf_reg(0xc9,value_signed);//write I2V DC value
                break;
            case 2:
                btdrv_write_rf_reg(0xca,value_signed);//write filter DC value
                break;
            default:
                break;
        }
    }

    BTDIGITAL_REG(0xd02201e4) = 0x00000000;//rx close

    btdrv_write_rf_reg(0xce,0x1001);//filter pdt reset dr=0
    btdrv_write_rf_reg(0x41,0x50c7);//LNA_att, i2v_gain dr=0
    btdrv_write_rf_reg(0x4f,0x010e);//LNA_gain dr=0
    btdrv_write_rf_reg(0x42,0x6c09);//filter_gain dr=0

    btdrv_write_rf_reg(0x27,0x0000);//rx input dr=0
    btdrv_write_rf_reg(0x7,0x4124);//rx input dr=0
    btdrv_write_rf_reg(0xcf,0x0124);//restore
}
/*****************************************************************************
 Prototype    : btdrv_1400_rccal
 Description  : 1400 rc calibration
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/11/30
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_1400_rccal(void)
{
    unsigned short value;
    unsigned short value_tmp;
    unsigned short val_tmp1;
    unsigned short val_tmp2;
    unsigned short val_rf_0xaa;

    btdrv_read_rf_reg(0xaa,&val_rf_0xaa);//backup rf 0xaa value
    btdrv_write_rf_reg(0x2b,0x0300);//pwup rcosc

    btdrv_delay(1);

    BTDIGITAL_REG(0xd02201e4) = 0x00000000;//rx on
    btdrv_delay(10);
    BTDIGITAL_REG(0xd02201e4) = 0x000a0080;
    btdrv_delay(10);

    btdrv_write_rf_reg(0xaa,(val_rf_0xaa | 0x0200));//enable clk counter

    btdrv_delay(10);

    btdrv_read_rf_reg(0xb0,&value);
    TRACE("btdrv_rccal 0xb0 value:%x\n",value);

    btdrv_read_rf_reg(0x03,&val_tmp1);
    TRACE("0x8b val_tmp1=%x\n",val_tmp1);

    btdrv_read_rf_reg(0x05,&val_tmp2);
    TRACE("0x8d val_tmp2=%x\n",val_tmp2);

    value_tmp = value & 0x0fff;
    if((value_tmp < 0x0ff0)&&(value_tmp > 0x0200)&&((value|0xefff) == 0xffff))
    {
        TRACE("0xb0 0x200 < value < 0xff0 done \n");
        btdrv_write_rf_reg(0x03,(((( 0x7e7 * 1000 / (value & 0x0fff)) * 0x8f / 1000) << 6) | (val_tmp1 & 0x003f)));
        TRACE("0x03:%x\n",((((( 0x7e7 * 1000 / (value & 0x0fff)) * 0x8f / 1000) << 6) << 6) | (val_tmp1 & 0x003f)));
        btdrv_read_rf_reg(0x03,&val_tmp1);
        TRACE("chk 0x03 val_tmp1=%x\n",val_tmp1);
        btdrv_write_rf_reg(0x05,((((0x7e7 * 1000 / (value & 0x0fff)) *  0x28 / 1000) << 10) | (val_tmp2 & 0x03ff)));
        TRACE("0x05:%x\n",((((0x7e7 * 1000 / (value & 0x0fff)) *  0x28 / 1000) << 10) | (val_tmp2 & 0x03ff)));
        btdrv_read_rf_reg(0x8d,&val_tmp2);
        TRACE("chk 0x05 val_tmp2=%x\n",val_tmp2);
    }
    else
    {
        btdrv_write_rf_reg(0x03,((0x8f << 6) | (val_tmp1 & 0x003f)));
        TRACE("0x03:%x\n",((0x8f << 6) | (val_tmp1 & 0x003f)));
        btdrv_write_rf_reg(0x05,((0x28 << 10) | (val_tmp2 & 0x03ff)));
        TRACE("0x05:%x\n",((0x28 << 10) | (val_tmp2 & 0x03ff)));
    }

    BTDIGITAL_REG(0xd02201e4) = 0x00000000;

    btdrv_write_rf_reg(0x2b,0x0000);//[9:8]=00,pwdown rcosc
}
/*****************************************************************************
 Prototype    : btdrv_rxbb_1400_dccal
 Description  : btdrv rx_bb calibration function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/12/3
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_rxbb_1400_dccal(void)
{
    unsigned short rfae_val, rfaf_val;

    uint16_t dccal_flt_q[8]= {0};
    uint16_t dccal_flt_i[8]= {0};

    uint16_t dccal_i2v_q[6]= {0};
    uint16_t dccal_i2v_i[6]= {0};

    BTDIGITAL_REG(0xd02201e4) = 0x00000000;//rx on
    btdrv_delay(10);
    BTDIGITAL_REG(0xd02201e4) = 0x000a0080;
    btdrv_delay(10);

    btdrv_write_rf_reg(0x27,0x0030);//bit4~5 set
    btdrv_write_rf_reg(0x41,0x57C7);//bit8~10 set
    btdrv_write_rf_reg(0x33,0xFF03);//bit0~1 set
    btdrv_write_rf_reg(0x2a,0x0300);//bit8~9 set
    btdrv_write_rf_reg(0x34,0x1081);//bit12 bit0 set
    btdrv_write_rf_reg(0xaa,0xFF40);//bit5 bit7 clear

    for(uint8_t counter=0; counter<8; counter++)
    {
        btdrv_write_rf_reg(0x41, 0x58C7);//bit11 set
        btdrv_write_rf_reg(0x4f, 0x010F);//bit0 set
        btdrv_write_rf_reg(0x42, 0x6E09);//bit9 set
        switch(counter)
        {
            case 0:
                btdrv_write_rf_reg(0x41, 0x00C7);//step1 bit12~15 0000
                btdrv_write_rf_reg(0x4f, 0x0108);//step1 bit1~3 001
                btdrv_write_rf_reg(0x42, 0x6009);//step1 bit10~12 000
                break;

            case 1:
                btdrv_write_rf_reg(0x41, 0x20C7);//step2 bit12~15 0100
                btdrv_write_rf_reg(0x4f, 0x010C);//step2 bit1~3 011
                btdrv_write_rf_reg(0x42, 0x7009);//step2 bit10~12 001
                break;

            case 2:
                btdrv_write_rf_reg(0x41, 0x40C7);//step3 bit12~15 0010
                btdrv_write_rf_reg(0x4f, 0x010A);//step3 bit1~3 101
                btdrv_write_rf_reg(0x42, 0x6809);//step3 bit10~12 010
                break;

            case 3:
                btdrv_write_rf_reg(0x41, 0x6C07);//step4 bit12~15 0110
                btdrv_write_rf_reg(0x4f, 0x0108);//step1 bit1~3 001
                btdrv_write_rf_reg(0x42, 0x7809);//step1 bit10~12 011
                break;

            case 4:
                btdrv_write_rf_reg(0x41, 0x10C7);//step4 bit12~15 1000
                btdrv_write_rf_reg(0x4f, 0x010C);//step1 bit1~3 011
                btdrv_write_rf_reg(0x42, 0x6409);//step1 bit10~12 100
                break;

            case 5:
                btdrv_write_rf_reg(0x41, 0x90C7);//step4 bit12~15 1001
                btdrv_write_rf_reg(0x4f, 0x010A);//step1 bit1~3 101
                btdrv_write_rf_reg(0x42, 0x7409);//step1 bit10~12 101
                break;

            case 6:
                btdrv_write_rf_reg(0x42, 0x6C09);//step1 bit10~12 110
                break;

            case 7:
                btdrv_write_rf_reg(0x42, 0x7C09);//step1 bit10~12 111
                break;
            default:
                break;
        }

        btdrv_write_rf_reg(0x30,0x0018);//bit8 clear, no action?
        btdrv_delay(1);
        btdrv_write_rf_reg(0x30,0x0118);//bit8 set
        btdrv_delay(5);

        btdrv_read_rf_reg(0xae,&rfae_val);
        if (rfae_val&0x1000)//bit12 is set
        {
            dccal_flt_q[counter] = rfae_val&0x03e0;//bit5~9
            dccal_flt_i[counter] = rfae_val&0x001f;//bit0~4
        }

        if (counter<6)
        {
            btdrv_read_rf_reg(0xaf,&rfaf_val);
            if (rfaf_val&0x1000)//bit12 is set
            {
                dccal_i2v_q[counter] = rfaf_val&0x0fc0;//bit6~11
                dccal_i2v_i[counter] = rfaf_val&0x003f;//bit0~5
            }
        }
        TRACE("rfae_val%d value = %x, rfaf_val%d = %x",counter,rfae_val,counter,rfaf_val);
    }
    btdrv_write_rf_reg(0x34,0x0C80);//bit10~11 11
    btdrv_write_rf_reg(0x35,0x3068);//bit5~6 11

    for (uint8_t result_counter=0; result_counter<8; result_counter++)
    {
        uint16_t i2v_reg;
        uint16_t flt_reg = dccal_flt_i[result_counter] | dccal_flt_q[result_counter];

        TRACE("flt_reg[%d]=%x",result_counter,flt_reg);
        if (result_counter<6)
        {
            i2v_reg = dccal_i2v_i[result_counter] | dccal_i2v_q[result_counter];
            TRACE("i2v_reg[%d] = %x",result_counter,i2v_reg);
        }
        switch(result_counter)
        {
            case 0:
                btdrv_write_rf_reg(0x47, i2v_reg);
                btdrv_write_rf_reg(0x50, flt_reg);
                break;

            case 1:
                btdrv_write_rf_reg(0x48, i2v_reg);
                btdrv_write_rf_reg(0x51, flt_reg);
                break;

            case 2:
                btdrv_write_rf_reg(0x49, i2v_reg);
                btdrv_write_rf_reg(0x52, flt_reg);
                break;

            case 3:
                btdrv_write_rf_reg(0x4a, i2v_reg);
                btdrv_write_rf_reg(0x53, flt_reg);
                break;

            case 4:
                btdrv_write_rf_reg(0x4b, i2v_reg);
                btdrv_write_rf_reg(0x54, flt_reg);
                break;

            case 5:
                btdrv_write_rf_reg(0x4c, i2v_reg);
                btdrv_write_rf_reg(0x55, flt_reg);
                break;

            case 6:
                btdrv_write_rf_reg(0x56, flt_reg);
                break;

            case 7:
                btdrv_write_rf_reg(0x57, flt_reg);
                break;

            default:
                break;
        }
    }

    btdrv_write_rf_reg(0x34, 0x0280);//bit0 bit12 clear,bit9 set
    btdrv_write_rf_reg(0x01, 0x9D3B);//bit10 set
}
/*****************************************************************************
 Prototype    : btdrv_start_bt
 Description  : btdrv main function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/11/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_start_bt(void)
{
    hal_sysfreq_req(HAL_SYSFREQ_USER_BT, HAL_CMU_FREQ_52M);

#ifndef NO_SLEEP
    pmu_sleep_en(0);
#endif

    /*power on BT*/
    btdrv_poweron(BT_POWERON);
    btdrv_delay(100);

    /*open hci interface*/
    btdrv_hciopen();

    /*download patch and enable patch*/
    btdrv_ins_patch_init();

    /*initialise rf and digital registers*/
    btdrv_config_init();
    btdrv_rf_init();

    btdrv_rf_init_ext();

    /*bes1400 rxbb dc calibration*/
    //btdrv_rxbb_1400_dccal();

    /*bes1400 rc calibration*/
    btdrv_1400_rccal();

    /*bes1400 hwagc dc calibration*/
    btdrv_hwagc_1400_dccal();

    /* rf tx cal */
    btdrv_rf_txcal();

    /*close hci interface*/
    btdrv_hcioff();

    /*reg controller crash dump*/
    hal_trace_crash_dump_register(bt_drv_reg_op_crash_dump);

#ifndef NO_SLEEP
    pmu_sleep_en(1);
#endif

    hal_sysfreq_req(HAL_SYSFREQ_USER_BT, HAL_CMU_FREQ_32K);
}

#undef DEFAULT_XTAL_FCAP
#define DEFAULT_XTAL_FCAP 0x8100

void btdrv_rf_init_ext(void)
{
    unsigned int xtal_fcap;

    if (!nvrec_dev_get_xtal_fcap(&xtal_fcap))
    {
        btdrv_rf_init_xtal_fcap(xtal_fcap);
        btdrv_delay(1);
        TRACE("%s xtal_fcap=0x%x", __func__, xtal_fcap);
    }
    else
    {
        btdrv_rf_init_xtal_fcap(DEFAULT_XTAL_FCAP);
        TRACE("%s failed, use default %04x", __func__, DEFAULT_XTAL_FCAP);
    }
}

void tx_ramp_new(void)
{
    return;

    BTDIGITAL_REG(0xd0220080) = 0x18421656;
    BTDIGITAL_REG(0xd0350360) = 0x00000120;
    BTDIGITAL_REG(0xd0350300) = 0x00000005;
    BTDIGITAL_REG(0xd0350340) = 0x00000005;
}

void bt_drv_extra_config_after_init(void)
{
    //BTDIGITAL_BT_EM(0xD021028A) = 0x00000000;//tx pwr init
    //ld_afh_env init
    bt_drv_reg_afh_env_reset();
    return;
}

const uint8_t hci_cmd_enable_dut[] =
{
    0x01,0x03, 0x18, 0x00
};
const uint8_t hci_cmd_enable_allscan[] =
{
    0x01, 0x1a, 0x0c, 0x01, 0x03
};
const uint8_t hci_cmd_disable_scan[] =
{
    0x01, 0x1a, 0x0c, 0x01, 0x00
};
const uint8_t hci_cmd_enable_pagescan[] =
{
    0x01, 0x1a, 0x0c, 0x01, 0x02
};
const uint8_t hci_cmd_autoaccept_connect[] =
{
    0x01,0x05, 0x0c, 0x03, 0x02, 0x00, 0x02
};
const uint8_t hci_cmd_hci_reset[] =
{
    0x01,0x03,0x0c,0x00
};


const uint8_t hci_cmd_nonsig_tx_dh1_pn9[] =
{
    0x01, 0x87, 0xfc, 0x14, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x00, 0x04, 0x04, 0x1b, 0x00
};
const uint8_t hci_cmd_nonsig_tx_2dh1_pn9[] =
{
    0x01, 0x87, 0xfc, 0x14, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x01, 0x04, 0x04, 0x36, 0x00
};
const uint8_t hci_cmd_nonsig_tx_3dh1_pn9[] =
{
    0x01, 0x87, 0xfc, 0x14, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x01, 0x08, 0x04, 0x53, 0x00
};
const uint8_t hci_cmd_nonsig_tx_2dh3_pn9[] =
{
    0x01, 0x87, 0xfc, 0x14, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x01, 0x0a, 0x04, 0x6f, 0x01
};
const uint8_t hci_cmd_nonsig_tx_3dh3_pn9[] =
{
    0x01, 0x87, 0xfc, 0x14, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x01, 0x0b, 0x04, 0x28, 0x02
};

const uint8_t hci_cmd_nonsig_rx_dh1_pn9[] =
{
    0x01, 0x87, 0xfc, 0x14, 0x01, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x00, 0x04, 0x00, 0x1b, 0x00
};
const uint8_t hci_cmd_nonsig_rx_2dh1_pn9[] =
{
    0x01, 0x87, 0xfc, 0x14, 0x01, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x01, 0x04, 0x00, 0x36, 0x00
};
const uint8_t hci_cmd_nonsig_rx_3dh1_pn9[] =
{
    0x01, 0x87, 0xfc, 0x14, 0x01, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x01, 0x08, 0x00, 0x53, 0x00
};
const uint8_t hci_cmd_nonsig_rx_2dh3_pn9[] =
{
    0x01, 0x87, 0xfc, 0x14, 0x01, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x01, 0x0a, 0x00, 0x6f, 0x01
};
const uint8_t hci_cmd_nonsig_rx_3dh3_pn9[] =
{
    0x01, 0x87, 0xfc, 0x14, 0x01, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x01, 0x0b, 0x00, 0x28, 0x02
};

//vco test
const uint8_t hci_cmd_start_bt_vco_test[] =
{
    0x01, 0xaa, 0xfc, 0x02, 0x00,0x02
};
const uint8_t hci_cmd_stop_bt_vco_test[] =
{
    0x01, 0xaa, 0xfc, 0x02, 0x00,0x04
};
void btdrv_testmode_start(void)
{
    TRACE("%s\n", __func__);

    btdrv_start_bt();

    btdrv_hciopen();
}

void btdrv_write_localinfo(char *name, uint8_t len, uint8_t *addr)
{
    uint8_t hci_cmd_write_addr[5+6] =
    {
        0x01, 0x72, 0xfc, 0x07, 0x00
    };

    uint8_t hci_cmd_write_name[248+4] =
    {
        0x01, 0x13, 0x0c, 0xF8
    };
    memset(&hci_cmd_write_name[4], 0, sizeof(hci_cmd_write_name)-4);
    memcpy(&hci_cmd_write_name[4], name, len);
    btdrv_SendData(hci_cmd_write_name, sizeof(hci_cmd_write_name));
    btdrv_delay(50);
    memcpy(&hci_cmd_write_addr[5], addr, 6);
    btdrv_SendData(hci_cmd_write_addr, sizeof(hci_cmd_write_addr));
    btdrv_delay(20);
}

void btdrv_enable_dut(void)
{
    btdrv_SendData(hci_cmd_enable_dut, sizeof(hci_cmd_enable_dut));
    btdrv_delay(20);
    btdrv_SendData(hci_cmd_enable_allscan, sizeof(hci_cmd_enable_allscan));
    btdrv_delay(20);
    btdrv_SendData(hci_cmd_autoaccept_connect, sizeof(hci_cmd_autoaccept_connect));
    btdrv_delay(20);
    bt_drv_reg_op_set_accessible_mode(3);

    btdrv_dut_mode_enable = true;
}

void btdrv_disable_scan(void)
{
    btdrv_SendData(hci_cmd_disable_scan, sizeof(hci_cmd_disable_scan));
    btdrv_delay(20);
}

uint32_t btdrv_dut_get_connect_status(void)
{
    return dut_connect_status;
}

void btdrv_hci_reset(void)
{
    btdrv_SendData(hci_cmd_hci_reset, sizeof(hci_cmd_hci_reset));
    btdrv_delay(100);
}

void btdrv_enable_nonsig_tx(uint8_t index)
{
    TRACE("%s\n", __func__);

    if (index == 0)
        btdrv_SendData(hci_cmd_nonsig_tx_2dh1_pn9, sizeof(hci_cmd_nonsig_tx_2dh1_pn9));
    else if (index == 1)
        btdrv_SendData(hci_cmd_nonsig_tx_3dh1_pn9, sizeof(hci_cmd_nonsig_tx_3dh1_pn9));
    else if (index == 2)
        btdrv_SendData(hci_cmd_nonsig_tx_2dh3_pn9, sizeof(hci_cmd_nonsig_tx_2dh1_pn9));
    else if (index == 3)
        btdrv_SendData(hci_cmd_nonsig_tx_3dh3_pn9, sizeof(hci_cmd_nonsig_tx_3dh1_pn9));
    else
        btdrv_SendData(hci_cmd_nonsig_tx_dh1_pn9, sizeof(hci_cmd_nonsig_tx_dh1_pn9));
    btdrv_delay(20);

}

void btdrv_enable_nonsig_rx(uint8_t index)
{
    TRACE("%s\n", __func__);

    if (index == 0)
        btdrv_SendData(hci_cmd_nonsig_rx_2dh1_pn9, sizeof(hci_cmd_nonsig_rx_2dh1_pn9));
    else if (index == 1)
        btdrv_SendData(hci_cmd_nonsig_rx_3dh1_pn9, sizeof(hci_cmd_nonsig_rx_3dh1_pn9));
    else if (index == 2)
        btdrv_SendData(hci_cmd_nonsig_rx_2dh3_pn9, sizeof(hci_cmd_nonsig_rx_2dh1_pn9));
    else if (index == 3)
        btdrv_SendData(hci_cmd_nonsig_rx_3dh3_pn9, sizeof(hci_cmd_nonsig_rx_3dh1_pn9));
    else
        btdrv_SendData(hci_cmd_nonsig_rx_dh1_pn9, sizeof(hci_cmd_nonsig_rx_dh1_pn9));
    btdrv_delay(20);
}

static bool btdrv_vco_test_running = false;
static unsigned short vco_test_reg_val_b6 = 0;
static unsigned short vco_test_reg_val_1f3 = 0;
#ifdef VCO_TEST_TOOL
static unsigned short vco_test_hack_flag = 0;
static unsigned short vco_test_channel = 0xff;

extern "C" unsigned short btdrv_get_vco_test_process_flag(void)
{
    return vco_test_hack_flag;
}

extern "C" bool btdrv_vco_test_bridge_intsys_callback(const unsigned char *data)
{
    bool status = false;
    if(data[0]==0x01 &&data[1]==0xaa&&data[2]==0xfc &&data[3]==0x02)
    {
        status = true;
        vco_test_hack_flag = data[5];
        vco_test_channel = data[4];
    }

    return status;
}

extern "C" void btdrv_vco_test_process(uint8_t op)
{
    if(op == 0x02)//vco test start
    {
        if(vco_test_channel != 0xff)
            btdrv_vco_test_start(vco_test_channel);
    }
    else if(op ==0x04)//vco test stop
    {
        btdrv_vco_test_stop();
    }
    vco_test_channel =0xff;
    vco_test_hack_flag = 0;
}
#endif
void btdrv_vco_test_start(uint8_t chnl)
{
    if (!btdrv_vco_test_running)
    {
        btdrv_vco_test_running = true;
        hal_analogif_reg_read(0xb6, &vco_test_reg_val_b6);
        hal_analogif_reg_read(0x1f3, &vco_test_reg_val_1f3);
        hal_analogif_reg_write(0x1f3, 0);
        hal_analogif_reg_write(0xb6, vco_test_reg_val_b6|(0x03<<14));

        BTDIGITAL_REG(0xd02201e4) = (chnl & 0x7f) | 0xa0000;
        btdrv_delay(10);
        BTDIGITAL_REG(0xd02201e4) = 0;
        btdrv_delay(10);
        BTDIGITAL_REG(0xd0340020) &= (~0x7);
        BTDIGITAL_REG(0xd0340020) |= 6;
        btdrv_delay(10);
    }
}

void btdrv_vco_test_stop(void)
{
    if (btdrv_vco_test_running)
    {
        btdrv_vco_test_running = false;
        BTDIGITAL_REG(0xd02201bc) = 0;
        BTDIGITAL_REG(0xd0340020) &=(~0x7);
        if (vco_test_reg_val_b6 != 0)
        {
            hal_analogif_reg_write(0xb6, vco_test_reg_val_b6);
        }
        if (vco_test_reg_val_1f3 != 0)
        {
            hal_analogif_reg_write(0x1f3, vco_test_reg_val_1f3);
        }
        btdrv_delay(10);
    }
}


void btdrv_stop_bt(void)
{
    btdrv_poweron(BT_POWEROFF);

}

extern "C" void btdrv_write_memory(uint8_t wr_type,uint32_t address,const uint8_t *value,uint8_t length)
{
    uint8_t buff[256];
    if(length ==0 || length >128)
        return;
    buff[0] = 0x01;
    buff[1] = 0x02;
    buff[2] = 0xfc;
    buff[3] = length + 6;
    buff[4] = address & 0xff;
    buff[5] = (address &0xff00)>>8;
    buff[6] = (address &0xff0000)>>16;
    buff[7] = address>>24;
    buff[8] = wr_type;
    buff[9] = length;
    memcpy(&buff[10],value,length);
    btdrv_SendData(buff,length+10);
    btdrv_delay(2);


}

extern "C" void btdrv_send_cmd(uint16_t opcode,uint8_t cmdlen,const uint8_t *param)
{
    uint8_t buff[256];
    buff[0] = 0x01;
    buff[1] = opcode & 0xff;
    buff[2] = (opcode &0xff00)>>8;
    buff[3] = cmdlen;
    if(cmdlen>0)
        memcpy(&buff[4],param,cmdlen);
    btdrv_SendData(buff,cmdlen+4);
}

void btdrv_rxdpd_sample_init(void)
{
    return;

    //0x40000048,0x03ffffff; //release btcpu reset
    BTDIGITAL_REG(0xc00003ac)=0x00060020;//turn off bt sleep
    btdrv_delay(2000);
    //0xd0220050,0x0000b7b7;
    BTDIGITAL_REG(0xd0350350)=0x70810081;
    BTDIGITAL_REG(0xd0350208)=0x8;
    BTDIGITAL_REG(0xd0350228)=0x8;

    BTDIGITAL_REG(0xd035035c)=0x50000000; // enable dc cancel
    BTDIGITAL_REG(0xd02201bc)=0x800a00c4; //rx continue for dpdsample and freq 2470
    BTDIGITAL_REG(0xd0340020)=0x010E01C5;// open ana rxon for open adc clk
    BTDIGITAL_REG(0xd0330038)=0x950d; //dpd enable 4 open modem adcclk

    btdrv_write_rf_reg(0xe0,0x470d);//fix gain
    btdrv_write_rf_reg(0x0b,0x470d);//fix gain
    btdrv_write_rf_reg(0xc7,0xf0f8);//gen single tone inside
    btdrv_write_rf_reg(0x04,0x5255);//gen single tone inside

}

void btdrv_rxdpd_sample_deinit(void)
{
    return;

    //[switch to RX normal]
    BTDIGITAL_REG(0xd02201bc) = 0x0;
    BTDIGITAL_REG(0xd0340020) = 0x010E01C0;
    BTDIGITAL_REG(0xd0330038) = 0x850d;

    //[SET inside tone off]
    btdrv_write_rf_reg(0x04,0x5154);
    btdrv_write_rf_reg(0xc7,0xf008);
    btdrv_write_rf_reg(0xe0,0x470d);
}

#define BTTX_PATTEN (1)
#define BTTX_FREQ(freq) ((freq-2402)&0x7f)

void btdrv_rxdpd_sample_init_tx(void)
{
    return;

    //0x40000048,0x03ffffff; //release btcpu reset
    BTDIGITAL_REG(0xc00003ac)=0x00060020;//turn off bt sleep
    btdrv_delay(2000);
    //0xd0220050,0x0000b7b7;

    BTDIGITAL_REG(0xd0340020)=0x010E01C7;// open TRX path
#if BTTX_PATTEN
    BTDIGITAL_REG(0xd02201bc)=0x000aff00 | BTTX_FREQ(2479); //tx continue for dpdsample
#else
    BTDIGITAL_REG(0xd02201bc)=0x000a0000 | BTTX_FREQ(2479); //tx continue for dpdsample
#endif
#ifdef CHIP_BEST1000
    BTDIGITAL_REG(0x4001f034)=0x00001000; //open p1_4 for agpio
#endif
    BTDIGITAL_REG(0xd0330038)=0x950d; //dpd enable 4 open modem adcclk
#if 0
    //monitor adcclk from p2_6
    BTDIGITAL_REG(0x4001f004)=0x02000000;
    BTDIGITAL_REG(0x40000074)=0x00f000ff;
    BTDIGITAL_REG(0xd033002c)=0x00000060;
#endif

    btdrv_write_rf_reg(0x2d,0x7fa); //open ldo for ad/da
    btdrv_write_rf_reg(0x0e,0x700); //set dac gain
    btdrv_write_rf_reg(0x0f,0x2a18);//set dac gain dr
    btdrv_write_rf_reg(0x10,0x00be);//sel dac output

#if 0
    btdrv_write_rf_reg(0x12,0x1188);
    btdrv_write_rf_reg(0x04,0x52fe);
    btdrv_write_rf_reg(0x14,0x01fc);
    btdrv_write_rf_reg(0x0c,0x6d84);
    btdrv_write_rf_reg(0x07,0x02bb);
    btdrv_write_rf_reg(0x0a,0x0223);

    btdrv_write_rf_reg(0x02,0x2694);
    btdrv_write_rf_reg(0x0b,0x402d);
    btdrv_write_rf_reg(0x10,0x00b5);
#else
    //set ad/da loop
    BTDIGITAL_REG(0xd0350308)=0x05;// digital gain * 5
    BTDIGITAL_REG(0xd0350300)=0x04;// digital gain / 16
    btdrv_write_rf_reg(0x07,0x2bd); //
    btdrv_write_rf_reg(0x02,0x2294); //
    btdrv_write_rf_reg(0x0b,0x4028);//
    btdrv_write_rf_reg(0x0c,0x6584);//
#endif
}

void btdrv_rxdpd_sample_enable(uint8_t rxon, uint8_t txon)
{
}
extern "C" void btdrv_btcore_extwakeup_irq_enable(bool on)
{
    if (on)
    {
        *(volatile uint32_t *)(0xd033003c) |= (1<<14);
    }
    else
    {
        *(volatile uint32_t *)(0xd033003c) &= ~(1<<14);
    }
}
uint32_t btdrv_syn_get_curr_ticks(void)
{
    uint32_t value;

    value = BTDIGITAL_REG(0xd0220490) & 0x0fffffff;
    return value;
}
static int32_t btdrv_syn_get_offset_ticks(uint16_t conidx)
{
    int32_t offset;
    uint32_t local_offset;
    uint16_t offset0;
    uint16_t offset1;
    offset0 = BTDIGITAL_BT_EM(EM_BT_CLKOFF0_ADDR + conidx*110);
    offset1 = BTDIGITAL_BT_EM(EM_BT_CLKOFF1_ADDR + conidx*110);

    local_offset = (offset0 | offset1 << 16) & 0x07ffffff;
    offset = local_offset;
    offset = (offset << 5)>>5;

    if (offset)
    {
        return offset*2;
    }
    else
    {
        return 0;
    }

}

extern "C" void  btdrv_syn_clr_trigger(void)
{
    BTDIGITAL_REG(0xd02201f0) = BTDIGITAL_REG(0xd02201f0) | (1<<31);
}

static void btdrv_syn_set_tg_ticks(uint32_t num, uint8_t mode)
{
    if (mode == BT_TRIG_MASTER_ROLE)
    {
        BTDIGITAL_REG(0xd02204a4) = 0x80000006;
        BTDIGITAL_REG(0xd02201f0) = (BTDIGITAL_REG(0xd02201f0) & 0x70000000) | (num & 0x0fffffff) | 0x10000000;
        //TRACE("master mode d02201f0:0x%x\n",BTDIGITAL_REG(0xd02201f0));
    }
    else
    {
        BTDIGITAL_REG(0xd02204a4) = 0x80000006;
        BTDIGITAL_REG(0xd02201f0) = (BTDIGITAL_REG(0xd02201f0) & 0x60000000) | (num & 0x0fffffff);
        //TRACE("slave mode d02201f0:0x%x\n",BTDIGITAL_REG(0xd02201f0));
    }
}

extern "C" void btdrv_syn_trigger_codec_en(uint32_t v)
{
}


extern "C" uint32_t btdrv_get_syn_trigger_codec_en(void)
{

    return BTDIGITAL_REG(0xd02201f0);

}


extern "C" uint32_t btdrv_get_trigger_ticks(void)
{

    return BTDIGITAL_REG(0xd02201f0);

}


// Can be used by master or slave
// Ref: Master bt clk
extern "C" uint32_t bt_syn_get_curr_ticks(uint16_t conhdl)
{
    int32_t curr,offset;

    curr = btdrv_syn_get_curr_ticks();
    if(conhdl>=0x80)
        offset = btdrv_syn_get_offset_ticks(btdrv_conhdl_to_linkid(conhdl));
    else
        offset = 0;
    //TRACE("[%s] curr(%d) + offset(%d) = %d", __func__, curr , offset,curr + offset);
    return (curr + offset) & 0x0fffffff;
}

// Can be used by master or slave
// Ref: Master bt clk
extern "C" void bt_syn_set_tg_ticks(uint32_t val,uint16_t conhdl, uint8_t mode)
{
    int32_t offset;
    if(conhdl>=0x80)
        offset = btdrv_syn_get_offset_ticks(btdrv_conhdl_to_linkid(conhdl));
    else
        offset = 0;

    if(conhdl==0x80)
    {
        BTDIGITAL_REG(0xd0220498)=(BTDIGITAL_REG(0xd0220498)&0xfffffff0)|0x1;
    }
    else if(conhdl==0x81)
    {
        BTDIGITAL_REG(0xd0220498)=(BTDIGITAL_REG(0xd0220498)&0xfffffff0)|0x2;
    }
    else if(conhdl==0x82)
    {
        BTDIGITAL_REG(0xd0220498)=(BTDIGITAL_REG(0xd0220498)&0xfffffff0)|0x3;
    }

    if ((mode == BT_TRIG_MASTER_ROLE) && (offset !=0))
        TRACE("ERROR OFFSET !!");

    TRACE("bt_syn_set_tg_ticks val:%d num:%d mode:%d conhdl:%02x", val, val - offset, mode, conhdl);

    btdrv_syn_set_tg_ticks(val - offset, mode);
}

extern "C"  void btdrv_enable_playback_triggler(uint8_t triggle_mode)
{
    if(triggle_mode == ACL_TRIGGLE_MODE)
    {
        //clear SCO trigger
        BTDIGITAL_REG(0xd02201f0) &= (~0x40000000);
        //set ACL trigger
        BTDIGITAL_REG(0xd02201f0) |= 0x20000000;
    }
    else if(triggle_mode == SCO_TRIGGLE_MODE)
    {
        //clear ACL trigger
        BTDIGITAL_REG(0xd02201f0) &= (~0x20000000);
        //set SCO trigger
        BTDIGITAL_REG(0xd02201f0) |= 0x40000000;
    }
}

/*
* bit28  1:master  0:slave
* both master/ slave reference local clk
* slave role will add rxbit compensation
*/

extern "C"  void btdrv_set_tws_role_triggler(uint8_t tws_mode)
{
    TRACE("btdrv_set_tws_role_triggler tws_mode:%d", tws_mode);

    if(tws_mode == BT_TRIG_MASTER_ROLE)
    {
        BTDIGITAL_REG(0xd02201f0) |= 0x10000000;
    }
    else if(tws_mode == BT_TRIG_SLAVE_ROLE)
    {
        BTDIGITAL_REG(0xd02201f0) &= (~0x10000000);
    }

}

extern "C"  void btdrv_set_bt_pcm_triggler_en(uint8_t  en)
{
    if(en)
    {
        BTDIGITAL_REG(0xd022046c) &= (~0x1);
    }
    else
    {
        BTDIGITAL_REG(0xd022046c) |= 0x1;
    }
}

extern "C" void btdrv_set_bt_pcm_triggler_delay(uint8_t  delay)
{
    if(delay > 0x3f)
    {
        TRACE("delay is error value");
        return;
    }
    TRACE("0XD022045c=%x",BTDIGITAL_REG(0xd022045c));
    BTDIGITAL_REG(0xd022045c) &= ~0x7f;
    BTDIGITAL_REG(0xd022045c) |= (delay);
    TRACE("exit :0XD022045c=%x",BTDIGITAL_REG(0xd022045c));
}


extern "C" void btdrv_set_bt_pcm_en(uint8_t  en)
{
    if(en)
        BTDIGITAL_REG(0xd02201b0) |= 1;
    else
        BTDIGITAL_REG(0xd02201b0) &= (~1);
}


extern "C" void btdrv_set_bt_pcm_triggler_delay_reset(uint8_t  delay)
{
    return;

    if(delay > 0x3f)
    {
        TRACE("delay is error value");
        return;
    }
    TRACE("0XD022045c=%x",BTDIGITAL_REG(0xd0224024));
    BTDIGITAL_REG(0XD022045c) &= ~0x3f;
    BTDIGITAL_REG(0XD022045c) |= delay|1;
    //  BTDIGITAL_REG(0xd0224024) |= 6;  //bypass sco trig
    TRACE("exit :0xd022045c=%x",BTDIGITAL_REG(0xd022045c));
}

extern "C" void btdrv_set_pcm_data_ignore_crc(void)
{

    BTDIGITAL_REG(0xD0220144) &= ~0x800000;
}

extern "C" uint8_t btdrv_conhdl_to_linkid(uint16_t connect_hdl)
{
    if(connect_hdl<0x80||connect_hdl>0x83)
    {
        ASSERT(0, "ERROR Connect Handle=0x%x", connect_hdl);
        return 3;
    }

    return (connect_hdl - 0x80);
}

extern "C" void btdrv_linear_format_16bit_set(void)
{
    *(volatile uint32_t *)(0xd02201a0) |= 0x00300000;
}

extern "C" void btdrv_pcm_enable(void)
{
    *(volatile uint32_t *)(0xd02201b0) |= 0x01; //pcm enable
}

extern "C" void btdrv_pcm_disable(void)
{
    *(volatile uint32_t *)(0xd02201b0) &= 0xfffffffe; //pcm disable
}

// Trace tport
static const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_tport[] =
{
    {HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
};

int btdrv_tportopen(void)
{
    uint32_t i;

    for (i=0; i<sizeof(pinmux_tport)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP); i++)
    {
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&pinmux_tport[i], 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)pinmux_tport[i].pin, HAL_GPIO_DIR_OUT, 0);
    }
    return 0;
}

int btdrv_tportset(int port)
{
    hal_gpio_pin_set((enum HAL_GPIO_PIN_T)pinmux_tport[port].pin);
    return 0;
}

int btdrv_tportclr(int port)
{
    hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)pinmux_tport[port].pin);
    return 0;
}
void btdrv_set_powerctrl_rssi_low(uint16_t rssi)
{
}

extern void bt_drv_set_music_link(uint8_t link_id);

void btdrv_enable_dual_slave_configurable_slot_mode(bool isEnable,
        uint16_t activeDevHandle, uint8_t activeDevRole,
        uint16_t idleDevHandle, uint8_t idleDevRole)
{
    if(isEnable)
    {
        bt_drv_set_music_link(activeDevHandle-0x80);
    }
    else
    {
        bt_drv_set_music_link(0xff);
    }
}

#if defined(TX_RX_PCM_MASK)
uint8_t  btdrv_is_pcm_mask_enable(void)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1)
        return 1;
    else
        return 0;
}
#endif

#if defined(CVSD_BYPASS)
void btdrv_cvsd_bypass_enable(void)
{
    BTDIGITAL_REG(0xD0220144) |= 0x5555;
    // BTDIGITAL_REG(0xD02201E8) |= 0x04000000; //test sequecy
    BTDIGITAL_REG(0xD02201A0) &= ~(1<<7); //soft cvsd
    //BTDIGITAL_REG(0xD02201b8) |= (1<<31); //revert clk
}
#endif
