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
#include "cmsis.h"
#include "bt_drv_reg_op.h"
#include "bt_drv_internal.h"
#include "bt_drv_2300_internal.h"
#include "bt_drv_interface.h"
#include "bt_drv.h"
#include "hal_chipid.h"
#include "hal_trace.h"
#include "hal_iomux.h"
#include <string.h>

//define for DEBUG
#define EM_BT_CLKOFF0_MASK   ((uint16_t)0x0000FFFF)
#define ASSERT_ERR(cond)                             { if (!(cond)) { TRACE("line is %d file is %s", __LINE__, __FILE__); } }
#define abs(x)  (x>0?x:(-x))

#define BT_DRIVER_GET_U8_REG_VAL(regAddr)       (*(uint8_t *)(regAddr))
#define BT_DRIVER_GET_U16_REG_VAL(regAddr)      (*(uint16_t *)(regAddr))
#define BT_DRIVER_GET_U32_REG_VAL(regAddr)      (*(uint32_t *)(regAddr))

#define BT_DRIVER_PUT_U8_REG_VAL(regAddr, val)      *(uint8_t *)(regAddr) = (val)
#define BT_DRIVER_PUT_U16_REG_VAL(regAddr, val)     *(uint16_t *)(regAddr) = (val)
#define BT_DRIVER_PUT_U32_REG_VAL(regAddr, val)     *(uint32_t *)(regAddr) = (val)

void bt_drv_reg_op_rssi_set(uint16_t rssi)
{
}

void bt_drv_reg_op_scan_intv_set(uint32_t scan_intv)
{
}

void bt_drv_reg_op_encryptchange_errcode_reset(uint16_t hci_handle)
{

}

void bt_drv_reg_op_sco_sniffer_checker(void)
{
}

void bt_drv_reg_op_trigger_time_checker(void)
{
    BT_DRV_REG_OP_ENTER();
    TRACE("0xd02201f0 = %x",*(volatile uint32_t *)0xd02201f0);
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_tws_output_power_fix_separate(uint16_t hci_handle, uint16_t pwr)
{
}

#define SNIFF_IN_SCO    2
///BD Address structure
struct bd_addr
{
    ///6-byte array address value
    uint8_t  addr[6];
};
///device info structure
struct ld_device_info
{
    struct bd_addr bd_addr;
    uint8_t link_id;
    uint8_t state;
};

bool bt_drv_reg_op_ld_sniffer_env_monitored_dev_state_get(void)
{
    bool nRet = false;
    struct ld_device_info * mobile_device_info;
    BT_DRV_REG_OP_ENTER();
    if(hal_get_chip_metal_id() > HAL_CHIP_METAL_ID_1 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        mobile_device_info = (struct ld_device_info *)0xc00061cc;
    }
    else
    {
        mobile_device_info = (struct ld_device_info *)0xc00062b0;
    }

    
    if (mobile_device_info->state & SNIFF_IN_SCO){
        nRet = true;
    }else{
        nRet = false;
    }
    //sniffer sco active (virtual)address 0xc00061ca
    TRACE("monitored device in sco state,ret=%d", nRet);

    BT_DRV_REG_OP_EXIT();

    return nRet;
}

void bt_drv_reg_op_ld_sniffer_env_monitored_dev_state_set(bool state)
{
    struct ld_device_info * mobile_device_info;
    BT_DRV_REG_OP_ENTER();

    if(hal_get_chip_metal_id() > HAL_CHIP_METAL_ID_1 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        mobile_device_info = (struct ld_device_info *)0xc00061cc;
    }
    else
    {
        mobile_device_info = (struct ld_device_info *)0xc00062b0;
    }

    uint8_t val = mobile_device_info->state;
    TRACE("monitored_dev_state %d",mobile_device_info->state);
    if (state){        
        val |= (uint8_t)(SNIFF_IN_SCO);
    }else{
        val &= (uint8_t)(~SNIFF_IN_SCO);
    }
    
    mobile_device_info->state = val;
    TRACE("monitored_dev_state %d",mobile_device_info->state);
    BT_DRV_REG_OP_EXIT();
}

int bt_drv_reg_op_currentfreeaclbuf_get(void)
{
    BT_DRV_REG_OP_ENTER();

    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0 || hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1)
    {
        return (*(volatile uint16_t *)0xC0006338);
    }
    else if(hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_4)
    {
        return (*(volatile uint16_t *)0xC0006638);//hci_fc_env.curr_pkt_nb
    }
    else
    {
        return (*(volatile uint16_t *)0xC000671c);//hci_fc_env.curr_pkt_nb
    }
    
    BT_DRV_REG_OP_EXIT();
}
static uint16_t app_tws_original_mobile_airpath_info;
void bt_drv_reg_op_save_mobile_airpath_info(uint16_t hciHandle)
{
    BT_DRV_REG_OP_ENTER();

    uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);
    
    app_tws_original_mobile_airpath_info = BT_DRIVER_GET_U16_REG_VAL(EM_BT_BTADDR1_ADDR + BT_EM_SIZE*link_id);
    TRACE("org mobile air info 0x%x ", app_tws_original_mobile_airpath_info);
    
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_block_xfer_with_mobile(uint16_t hciHandle)
{
    BT_DRV_REG_OP_ENTER();

    uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);
    
    BT_DRIVER_PUT_U16_REG_VAL((EM_BT_BTADDR1_ADDR + BT_EM_SIZE*link_id), app_tws_original_mobile_airpath_info^0x8000);

    TRACE("block_xfer_with_mobile,hci handle 0x%x as 0x%x", hciHandle, app_tws_original_mobile_airpath_info^0x8000);

    BT_DRV_REG_OP_EXIT();    
}

void bt_drv_reg_op_resume_xfer_with_mobile(uint16_t hciHandle)
{
    BT_DRV_REG_OP_ENTER();
    uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);

    BT_DRIVER_PUT_U16_REG_VAL((EM_BT_BTADDR1_ADDR + BT_EM_SIZE*link_id), app_tws_original_mobile_airpath_info);
    TRACE("resume xfer with mobile, hci handle 0x%x as 0x%x", hciHandle, app_tws_original_mobile_airpath_info);

    BT_DRV_REG_OP_EXIT();
}

int bt_drv_reg_op_packet_type_checker(uint16_t hciHandle)
{
    return 0;
}

void bt_drv_reg_op_max_slot_setting_checker(uint16_t hciHandle)
{}

void bt_drv_reg_op_force_task_dbg_idle(void)
{
    BT_DRV_REG_OP_ENTER();

    BT_DRIVER_PUT_U8_REG_VAL(0xc0000054, 1);
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_afh_follow_mobile_mobileidx_set(uint16_t hciHandle)
{}


void bt_drv_reg_op_afh_follow_mobile_twsidx_set(uint16_t hciHandle)
{}

void bt_drv_reg_op_afh_bak_reset(void)
{}

void bt_drv_reg_op_afh_bak_save(uint8_t role, uint16_t mobileHciHandle)
{}

void bt_drv_reg_op_connection_checker(void)
{}

void bt_drv_reg_op_sco_status_store(void)
{}

void bt_drv_reg_op_sco_status_restore(void)
{}

void bt_drv_reg_op_afh_set_default(void)
{
}

void bt_drv_reg_op_ld_sniffer_master_addr_set(uint8_t * addr)
{
    BT_DRV_REG_OP_ENTER();

    if(addr != NULL)
    {
        if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0 || hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1)
        {
            memcpy((uint8_t *)0xc0005f08, addr, 6);
        }
        else if(hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_4)
        {
            memcpy((uint8_t *)0xc0006204, addr, 6);
        }
        else
        {
            memcpy((uint8_t *)0xc00062e8, addr, 6);
        }
    }
    else
    {
        TRACE("ERROR Master address");
    }

    BT_DRV_REG_OP_EXIT();    
}

uint8_t msbc_find_tx_sync(uint8_t *buff)
{
    BT_DRV_REG_OP_ENTER();

    uint8_t i;
    for(i=0;i<60;i++)
    {
        if(buff[i]==0x1 && buff[(i+2)%60] == 0xad)
        {
        //    TRACE("MSBC tx sync find =%d",i);
            return i;
        }
    }
    TRACE("TX No pACKET");

    BT_DRV_REG_OP_EXIT();    
    return 0;
}

bool bt_drv_reg_op_sco_tx_buf_restore(uint8_t *trigger_test)
{
    uint8_t offset;
    bool nRet = false;

    BT_DRV_REG_OP_ENTER();
    
    offset = msbc_find_tx_sync((uint8_t *)EM_BT_AUDIOBUF_OFF);
    if(offset !=0)
     {
#ifndef APB_PCM     
        *trigger_test = (((BTDIGITAL_REG(0xd022045c) & 0x3f)) +(60-offset))%64;
#endif
        TRACE("TX BUF ERROR trigger_test=%x,offset=%x", trigger_test,offset);
     DUMP8("%02x ",(uint8_t *)EM_BT_AUDIOBUF_OFF,10);
//   TRACE("pcm reg=%x %x",*(uint32_t *)0xd0220468,*(uint32_t *)0x400000f0);
        nRet = true;
    }

    BT_DRV_REG_OP_EXIT();    

    return nRet;
}

extern "C" uint32_t hci_current_left_tx_packets_left(void);
extern "C" uint32_t hci_current_left_rx_packets_left(void);
extern "C" uint32_t hci_current_rx_packet_complete(void);
extern "C" uint8_t hci_current_rx_aclfreelist_cnt(void);
void bt_drv_reg_op_bt_packet_checker(void)
{
    uint32_t *buf_ptr = (uint32_t *)0xc0006640;//hci_fc_env
    uint8_t buf_count=0 , i=0; 
    uint16_t em_buf ;
    uint32_t acl_pkt_sent = *(uint16_t *)buf_ptr;
    uint32_t aclRxPacketsLeft = hci_current_left_rx_packets_left();
    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0 || hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1)
    {    
        buf_ptr = (uint32_t *)0xc0005798; //bt_util_buf_env.acl_rx_free c0005784
    }
    else if(hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_4)
    {
        buf_ptr = (uint32_t *)0xc00058e8; //bt_util_buf_env.acl_rx_free c00058d4
    }
    else
    {
        buf_ptr = (uint32_t *)0xc000596c; //bt_util_buf_env.acl_rx_free 0xc0005958
    }
    buf_count = 0;
    while(*buf_ptr)
    {
        buf_count++;
        buf_ptr = (uint32_t *)(*buf_ptr);
    }
    for(i=0;i<4;i++)
    {
        em_buf = BT_DRIVER_GET_U16_REG_VAL(EM_BT_RXACLBUFPTR_ADDR + i*14);
        if(em_buf != 0)
            buf_count++;
    }
    TRACE("ctrl acl_pkt_sent %d,aclRxPacketsLeft %d,buf_count %d,curr rx comp %d,curr aclfreelst %d",acl_pkt_sent,aclRxPacketsLeft,buf_count,hci_current_rx_packet_complete(),hci_current_rx_aclfreelist_cnt());
    ASSERT_ERR(buf_count>3);
}

void bt_drv_reg_op_bt_info_checker(void)
{
    uint32_t *buf_ptr;
    uint8_t buf_count=0 , i=0; 
    uint16_t em_buf ;

    BT_DRV_REG_OP_ENTER();

    //bt_drv_reg_op_connection_checker();

    //TRACE_LVL(HAL_TRACE_LEVEL_ENV, "tx flow=%x",*(uint32_t *)0xc0003f9c);
    //TRACE_LVL(HAL_TRACE_LEVEL_ENV, "remote_tx flow=%x",*(uint32_t *)0xc0003fa2);
    
    
    //TRACE_LVL(HAL_TRACE_LEVEL_ENV, "tx buff = %x %x",*(uint16_t *)0xc0005d48,hci_current_left_tx_packets_left());
    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0 || hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1)
    {    
        buf_ptr = (uint32_t *)0xc0005798; //bt_util_buf_env.acl_rx_free c0005784
    }
    else if(hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_4)
    {
        buf_ptr = (uint32_t *)0xc00058e8; //bt_util_buf_env.acl_rx_free c00058d4
    }
    else
    {
        buf_ptr = (uint32_t *)0xc000596c; //bt_util_buf_env.acl_rx_free 0xc0005958
    }
    buf_count = 0;
    while(*buf_ptr)
    {
        buf_count++;
        buf_ptr = (uint32_t *)(*buf_ptr);
    }
    for(i=0;i<4;i++)
    {
        em_buf = BT_DRIVER_GET_U16_REG_VAL(EM_BT_RXACLBUFPTR_ADDR + i*14);
        if(em_buf != 0)
            buf_count++;
    }
    TRACE("rxbuff = %x %x , host rx buff in controller = %x",buf_count,hci_current_left_rx_packets_left(),BT_DRIVER_GET_U16_REG_VAL(0xc0006340));

    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0 || hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1)
    {    
        buf_ptr = (uint32_t *)0xc00057ac; //bt_util_buf_env.acl_tx_free c0005784
    }
    else if(hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_4)
    {
        buf_ptr = (uint32_t *)0xc00058fc; //bt_util_buf_env.acl_tx_free c00058d4
    }
    else
    {
        buf_ptr = (uint32_t *)0xc0005980; //bt_util_buf_env.acl_tx_free 0xc0005958
    }
   // buf_ptr = (uint32_t *)0xc00057ac; ////bt_util_buf_env.acl_tx_free
    buf_count = 0;
    while(*buf_ptr)
    {
        buf_count++;
        buf_ptr = (uint32_t *)(*buf_ptr);
    }

    TRACE("acl tx buffer debug controller txbuff = %x host acl left txbuffer = %x ",buf_count,hci_current_left_tx_packets_left());

    BT_DRV_REG_OP_EXIT();
}

uint32_t *rxbuff;
uint16_t *send_count;
uint16_t *free_count;
uint16_t *bt_send_count;

void bt_drv_reg_op_ble_buffer_cleanup(void)
{}

struct ke_timer
{
    /// next ke timer
    struct ke_timer *next;
    /// message identifier
    uint16_t     id;
    /// task identifier
    uint16_t    task;
    /// time value
    uint32_t    time;
};

struct co_list_hdr
{
    /// Pointer to next co_list_hdr
    struct co_list_hdr *next;
};

/// structure of a list
struct co_list_con
{
    /// pointer to first element of the list
    struct co_list_hdr *first;
    /// pointer to the last element
    struct co_list_hdr *last;

    /// number of element in the list
    uint32_t cnt;
    /// max number of element in the list
    uint32_t maxcnt;
    /// min number of element in the list
    uint32_t mincnt;
};

struct mblock_free
{
    /// Next free block pointer
    struct mblock_free* next;
    /// Previous free block pointer
    struct mblock_free* previous;
    /// Size of the current free block (including delimiter)
    uint16_t free_size;
    /// Used to check if memory block has been corrupted or not
    uint16_t corrupt_check;
};

void bt_drv_reg_op_crash_dump(void)
{
    if(hal_get_chip_metal_id() > HAL_CHIP_METAL_ID_1)
    {
        TRACE("BT controller BusFault_Handler:\nREG:[LR] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE));
        TRACE("REG:[R4] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +4));
        TRACE("REG:[R5] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +8));
        TRACE("REG:[R6] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +12));
        TRACE("REG:[R7] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +16));
        TRACE("REG:[R8] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +20));
        TRACE("REG:[R9] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +24));
        TRACE("REG:[sl] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +28));
        TRACE("REG:[fp] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +32));
        TRACE("REG:[ip] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +36));
        TRACE("REG:[SP,#0] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +40));
        TRACE("REG:[SP,#4] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +44));
        TRACE("REG:[SP,#8] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +48));
        TRACE("REG:[SP,#12] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +52));
        TRACE("REG:[SP,#16] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +56));
        TRACE("REG:[SP,#20] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +60));
        TRACE("REG:[SP,#24] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +64));
        TRACE("REG:[SP,#28] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +68));
        TRACE("REG:[SP,#32] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +72));
        TRACE("REG:[SP,#36] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +76));
        TRACE("REG:[SP,#40] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +80));
        TRACE("REG:[SP,#44] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +84));
        TRACE("REG:[SP,#48] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +88));
        TRACE("REG:[SP,#52] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +92));
        TRACE("REG:[SP,#56] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +96));
        TRACE("REG:[SP,#60] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +100));
        TRACE("REG:[SP,#64] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +104));
        TRACE("REG:[cfsr] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +108));
        TRACE("REG:[bfar] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +112));
        TRACE("REG:[hfsr] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +116));

        uint32_t buff_addr;
        if(hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
        {
            buff_addr = 0xc0000738;
        }
        else
        {
            buff_addr = 0xc000079c;
        }
        TRACE("task msg buff:");
        for(uint8_t j=0;j<5;j++)
        {
            DUMP8("%02x ", (uint8_t *)(buff_addr+j*20), 20);
        }
        TRACE("");
        TRACE("lmp buff:");
        if(hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
        {
            buff_addr = 0xc00007a0;
        }
        else
        {
            buff_addr = 0xc0000804;
        }
        for(uint8_t j=0;j<10;j++)
        {
            DUMP8("%02x ",(uint8_t *)(buff_addr+j*20), 20);
        }

        uint8_t link_id = 0;
        uint32_t evt_ptr = 0;
        uint32_t acl_par_ptr = 0;
        for(link_id=0;link_id<3;link_id++){
            TRACE("acl_par: link id %d",link_id);
            if(hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
            {
                evt_ptr = *(uint32_t *)(0xc00008fc+link_id*4);
            }
            else
            {
                evt_ptr = *(uint32_t *)(0xc000095c+link_id*4);
            }
            acl_par_ptr = evt_ptr;
            TRACE("acl_par: acl_par_ptr 0x%x, clk off 0x%x, bit off 0x%x, last sync clk off 0x%x, last sync bit off 0x%x",
                acl_par_ptr, *(uint32_t *)(acl_par_ptr+140),*(uint16_t *)(acl_par_ptr+150),
                *(uint32_t *)(acl_par_ptr+136),((*(uint32_t *)(acl_par_ptr+150))&0xFFFF0000)>>16);
        }
       if(hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)                    
       {
        evt_ptr = *(uint32_t *)(0xc00008ec); 
        }
    else
       {
           evt_ptr = *(uint32_t *)(0xc000094c); 
       }
    TRACE("esco linkid :%d",*(uint8_t *)(evt_ptr+70));
        for(link_id=0;link_id<3;link_id++){
            TRACE("bt_linkcntl_linklbl 0x%x: link id %d",*(uint16_t *)(EM_BT_LINKCNTL_ADDR+link_id*BT_EM_SIZE),link_id);
            TRACE("rxcount :%x",*(uint16_t *)(EM_BT_RXDESCCNT_ADDR+link_id*BT_EM_SIZE));
        }
     
    }
}

uint8_t bt_drv_reg_op_get_tx_pwr(uint16_t connHandle)
{   
    uint8_t idx;
    uint16_t localVal;
    uint8_t tx_pwr;
    
    idx = btdrv_conhdl_to_linkid(connHandle);
    localVal = BT_DRIVER_GET_U16_REG_VAL(EM_BT_PWRCNTL_ADDR + idx * BT_EM_SIZE);
    tx_pwr = ((localVal & ((uint16_t)0x000000FF)) >> 0);

    return tx_pwr;
}

void bt_drv_reg_op_set_tx_pwr(uint16_t connHandle, uint8_t txpwr)
{
    BT_DRV_REG_OP_ENTER();

    uint8_t idx = btdrv_conhdl_to_linkid(connHandle);
    
    BT_DRIVER_PUT_U16_REG_VAL(EM_BT_PWRCNTL_ADDR + idx * BT_EM_SIZE, 
        (BT_DRIVER_GET_U16_REG_VAL(EM_BT_PWRCNTL_ADDR + idx * BT_EM_SIZE) & ~((uint16_t)0x000000FF)) | ((uint16_t)txpwr << 0));

    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_fix_tx_pwr(uint16_t connHandle)
{
    BT_DRV_REG_OP_ENTER();
    bt_drv_reg_op_set_tx_pwr(connHandle, LBRT_TX_PWR_FIX);
    BT_DRV_REG_OP_EXIT();
}


static void bt_drv_reg_op_set_emsack_cs(uint16_t connHandle, uint8_t master)
{
    ////by luofei
    uint8_t idx = btdrv_conhdl_to_linkid(connHandle);

    BT_DRV_REG_OP_ENTER();

    if(master)
    {
        BT_DRIVER_PUT_U16_REG_VAL(EM_BT_BT_EXT1_ADDR + idx * BT_EM_SIZE, 
            (BT_DRIVER_GET_U16_REG_VAL(EM_BT_BT_EXT1_ADDR + idx * BT_EM_SIZE) & ~((uint16_t)0x00000600)) | ((uint16_t)1 << 10));
    }
    else
    {
        BT_DRIVER_PUT_U16_REG_VAL(EM_BT_BT_EXT1_ADDR + idx * BT_EM_SIZE, 
            (BT_DRIVER_GET_U16_REG_VAL(EM_BT_BT_EXT1_ADDR + idx * BT_EM_SIZE) & ~((uint16_t)0x00000600)) | ((uint16_t)1 << 9));
    }
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_enable_emsack_mode(uint16_t connHandle, uint8_t master, uint8_t enable)
{
    BT_DRV_REG_OP_ENTER();

    ////by luofei
    if(hal_get_chip_metal_id()>HAL_CHIP_METAL_ID_1)
    {
        if(enable)
        {
            //EMSACK cs config
            bt_drv_reg_op_set_emsack_cs(connHandle, master);
            //EMSACK toggle
            *(volatile uint32_t *)0xd0220468 |= (1<<20 | 1<<21);
        }
        else
        {
            *(volatile uint32_t *)0xd0220468 &= ~(1<<20 | 1<<21);
        }
    }
    BT_DRV_REG_OP_EXIT();
}

#ifndef __HW_AGC__
#define SWAGC_IDX (7)
static uint8_t swagc_set=0;
#endif

//#define __ACCESS_MODE_ADJUST_GAIN__
//#define __SWAGC_MODE_ADJUST_GAIN__
#define __REBOOT_PAIRING_MODE_ADJUST_GAIN__

void bt_drv_reg_op_set_accessible_mode(uint8_t mode)
{
    static uint8_t current_accessible_mode = 0xff;
    enum HAL_IOMUX_ISPI_ACCESS_T access;
    uint32_t lock;
    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_4)
    {
        return;
    }

#if !defined(__ACCESS_MODE_ADJUST_GAIN__)
    return;
#endif
#ifndef __HW_AGC__ 
    if(swagc_set==1)
    {
        return;
    }
#endif
    if (current_accessible_mode == mode){
        return;
    }

    BT_DRV_REG_OP_ENTER();

    current_accessible_mode = mode;

    lock = int_lock();

    access = hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_MCU_RF);

#if defined(__HW_AGC__)
#if defined(__ACCESS_MODE_ADJUST_GAIN__)
    if(mode)
    {        
        btdrv_rf_rx_gain_adjust_req(1, true);
    }
    else
    {        
        btdrv_rf_rx_gain_adjust_req(1, false);
    }
#endif
#else
    if(mode)
    {
        *(volatile uint8_t *)0xc0004137 = SWAGC_IDX;
        TRACE("0xc0004137 1 = %x",*(volatile uint8_t *)0xc0004137);
    }
    else
    {
        *(volatile uint8_t *)0xc0004137 = 0xff;
        TRACE("0xc0004137 2= %x",*(volatile uint8_t *)0xc0004137);
    }
#endif
    if ((access & HAL_IOMUX_ISPI_MCU_RF) == 0) {
        hal_iomux_ispi_access_disable(HAL_IOMUX_ISPI_MCU_RF);
    }

    int_unlock(lock);

    BT_DRV_REG_OP_EXIT();
}



void bt_drv_reg_op_set_swagc_mode(uint8_t mode)
{
    static uint8_t current_swagc_mode = 0xff;

    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_4)
    {
        return;
    }

#if !defined(__SWAGC_MODE_ADJUST_GAIN__)
    return;
#endif

    if (current_swagc_mode == mode){
        return;
    }

    BT_DRV_REG_OP_ENTER();
    
    current_swagc_mode = mode;

#if !defined(__HW_AGC__)
    TRACE("bt_drv_reg_op_set_swagc_mode %d", mode);
    if(mode)
    {
        *(volatile uint8_t *)0xc0004137 = SWAGC_IDX;
        TRACE("0xc0004137 3= %x",*(volatile uint8_t *)0xc0004137);
        swagc_set = 1;
    }
    else
    {
    
        *(volatile uint8_t *)0xc0004137 = 0xff;
        TRACE("0xc0004137 4= %x",*(volatile uint8_t *)0xc0004137);
        swagc_set = 0;
        
    }
#elif defined(__SWAGC_MODE_ADJUST_GAIN__)
{
    enum HAL_IOMUX_ISPI_ACCESS_T access;
    uint32_t lock;

    lock = int_lock();

    access = hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_MCU_RF);
    if(mode)
    {        
//        btdrv_rf_rx_gain_adjust_req(0, true);
        btdrv_rf_blerx_gain_adjust_lowgain();
    }
    else
    {
//        btdrv_rf_rx_gain_adjust_req(0, false);
        btdrv_rf_blerx_gain_adjust_default();
    }    
    if ((access & HAL_IOMUX_ISPI_MCU_RF) == 0) {
        hal_iomux_ispi_access_disable(HAL_IOMUX_ISPI_MCU_RF);
    }
    
    int_unlock(lock);
}
#endif

    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_set_reboot_pairing_mode(uint8_t mode)
{
    static uint8_t reboot_pairing_mode = 0;

    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_4)
    {
        return;
    }

#if !defined(__REBOOT_PAIRING_MODE_ADJUST_GAIN__)
    return;
#endif
    if (reboot_pairing_mode == mode){
        return;
    }

    BT_DRV_REG_OP_ENTER();
    
    reboot_pairing_mode = mode;
#if !defined(__HW_AGC__)
    TRACE("bt_drv_reg_op_set_swagc_mode %d", mode);
    if(mode)
    {
        *(volatile uint8_t *)0xc0004137 = SWAGC_IDX;
        TRACE("0xc0004137 3= %x",*(volatile uint8_t *)0xc0004137);
        reboot_pairing_mode = 1;
    }
    else
    {
    
        *(volatile uint8_t *)0xc0004137 = 0xff;
        TRACE("0xc0004137 4= %x",*(volatile uint8_t *)0xc0004137);
        reboot_pairing_mode = 0;
        
    }
#elif defined(__REBOOT_PAIRING_MODE_ADJUST_GAIN__)
{
    enum HAL_IOMUX_ISPI_ACCESS_T access;
    uint32_t lock;

    lock = int_lock();

    access = hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_MCU_RF);
    if(mode)
    {        
        btdrv_rf_rx_gain_adjust_req(3, true);
    }
    else
    {        
        btdrv_rf_rx_gain_adjust_req(3, false);
    }    
    if ((access & HAL_IOMUX_ISPI_MCU_RF) == 0) {
        hal_iomux_ispi_access_disable(HAL_IOMUX_ISPI_MCU_RF);
    }
    
    int_unlock(lock);
}
#endif

    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_force_retrans(bool enalbe)
{
    return;

    BT_DRV_REG_OP_ENTER();

    if (enalbe){
        BTDIGITAL_REG_SET_FIELD(0xd0220468,3,23,3);
    }else{
        BTDIGITAL_REG_SET_FIELD(0xd0220468,3,23,0);
    }
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_enable_pcm_tx_hw_cal(void)
{
    BT_DRV_REG_OP_ENTER();
    *(volatile uint32_t *)0xd0220468 |= 1<<22;
    *(volatile uint32_t *)0x400000f0 |= 1;
    BT_DRV_REG_OP_EXIT();
}



void bt_drv_reg_monitor_clk(void)
{
    BT_DRV_REG_OP_ENTER();
    uint32_t env0;  
    uint32_t env1;
    uint32_t env2;
    if(hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_4)
    {
        env0 = *(uint32_t *)0xc00008fc;
        env1 = *(uint32_t *)0xc0000900;
        env2 = *(uint32_t *)0xc0000904;
    }
    else
    {
        env0 = *(uint32_t *)0xc000095c;
        env1 = *(uint32_t *)0xc0000960;
        env2 = *(uint32_t *)0xc0000964;        
    }
    if(env0 & 0xc0000000)
    {
        env0 +=0x8c;
        TRACE("env0 clk off=%x %x",*(uint32_t *)env0,*(uint16_t *)EM_BT_CLKOFF0_ADDR | (*(uint16_t *)EM_BT_CLKOFF1_ADDR <<16));
    }
    if(env1 & 0xc0000000)
    {
        env1 +=0x8c;
        TRACE("env1 clk off=%x %x",*(uint32_t *)env1,*(uint16_t *)(EM_BT_CLKOFF0_ADDR+BT_EM_SIZE) | (*(uint16_t *)(EM_BT_CLKOFF1_ADDR+BT_EM_SIZE) <<16));
    }
    if(env2 & 0xc0000000)
    {
        env2 +=0x8c;
        TRACE("env2 clk off=%x %x",*(uint32_t *)env2,*(uint16_t *)(EM_BT_CLKOFF0_ADDR+BT_EM_SIZE*2)| (*(uint16_t *)(EM_BT_CLKOFF1_ADDR+BT_EM_SIZE*2) <<16));
    }

    BT_DRV_REG_OP_EXIT();
}

struct rx_monitor{
    int8_t rssi;
    uint8_t rxgain;
};

int8_t bt_drv_read_rssi_in_dbm(uint16_t connHandle)
{
    int8_t rssi = 0;
#ifdef BT_RSSI_MONITOR
    BT_DRV_REG_OP_ENTER();
 
    if(hal_get_chip_metal_id()>HAL_CHIP_METAL_ID_1)
    {    
        uint8_t idx = btdrv_conhdl_to_linkid(connHandle);
        /// Accumulated RSSI (to compute an average value)
        int16_t rssi_acc = 0;
        /// Counter of received packets used in RSSI average
        uint8_t rssi_avg_cnt = 5;
        struct rx_monitor * rx_monitor_env;
        if(hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_4)
        {
        rx_monitor_env = (struct rx_monitor *)0xc0005810;        
        }
        else
        {
            rx_monitor_env = (struct rx_monitor *)0xc0005894;   
        }

        for(int i=0; i< rssi_avg_cnt; i++)
        {
            rssi_acc += rx_monitor_env[idx].rssi;  
        }
        
        rssi = rssi_acc / rssi_avg_cnt;
        TRACE("read_rssi_in_dbm =%d",rssi);
    }

    BT_DRV_REG_OP_EXIT();
#else
    rssi = -90;
#endif
    return rssi;
}


void bt_drv_reg_op_acl_silence(uint16_t connHandle, uint8_t silence)
{
    BT_DRV_REG_OP_ENTER();

    uint8_t idx = btdrv_conhdl_to_linkid(connHandle);

    BT_DRIVER_PUT_U16_REG_VAL(EM_BT_BT_EXT1_ADDR + idx * BT_EM_SIZE, 
        (BT_DRIVER_GET_U16_REG_VAL(EM_BT_BT_EXT1_ADDR + idx * BT_EM_SIZE) & ~((uint16_t)0x00008000)) | ((uint16_t)silence << 15));
    
    BT_DRV_REG_OP_EXIT();
}

 ///sniffer connect information environment structure
struct ld_sniffer_connect_env
{
    /// 
    uint16_t bitoff;
    ///
    uint32_t clk_offset;
    ///
    uint8_t map[10];    
    ///
    uint32_t afh_instant;
    ///
    uint8_t afh_mode;
    ///
    uint8_t enc_mode;
    ///
    uint8_t  ltk[16];
    ///
    uint8_t  role;
};

void bt_drv_reg_op_call_monitor(uint16_t connHandle, uint8_t tws_role)
{
    uint8_t idx = btdrv_conhdl_to_linkid(connHandle);

    BT_DRV_REG_OP_ENTER();

    if(hal_get_chip_metal_id()>HAL_CHIP_METAL_ID_1)
    {
        struct ld_sniffer_connect_env * connect_env = (struct ld_sniffer_connect_env *)0xc00061d4;  
        //bitoffset 10 bit
        uint16_t bitoffset_in_cs = (BTDIGITAL_BT_EM(BT_EM_ADDR_BASE +0x2 +  idx * BT_EM_SIZE)&0x3ff);
        //clock offset 32 bit
        uint32_t clockoffset_in_cs = (BTDIGITAL_REG(BT_EM_ADDR_BASE +0x4 +  idx * BT_EM_SIZE)&0x7ffffff);     
        //rx clkn 32 bit
        uint32_t rxclkn_in_cs = (BTDIGITAL_REG(BT_EM_ADDR_BASE +0x52 +  idx * BT_EM_SIZE)&0x7ffffff);        
        //rx bit 10bit
        uint16_t rxbit_in_cs = (BTDIGITAL_BT_EM(BT_EM_ADDR_BASE +0x56 +  idx * BT_EM_SIZE)&0x3ff);
            
        TRACE("TWS=%d,[env]:bitoff=0x%x,clkoff=0x%x,[CS]:bitoff =0x%x,clkoff=0x%x,clkn=0x%x,bit=0x%x",tws_role,connect_env->bitoff,connect_env->clk_offset,
            bitoffset_in_cs,clockoffset_in_cs,rxclkn_in_cs,rxbit_in_cs);
        
        uint8_t cs_channel_map[10];
        for(uint8_t i=0;i<3;i++)
        {
            uint32_t map_cs_addr = BT_EM_ADDR_BASE + 0x1E + i*BT_EM_SIZE;
            memcpy(cs_channel_map,(uint8_t*)map_cs_addr,10);
            TRACE("[CS map %d]:",i);
            DUMP8("%02x ",cs_channel_map,10);                  
        }
        
        TRACE("[env map]:");
        DUMP8("%02x ",connect_env->map,10);   
        
        
    }
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_lock_sniffer_sco_resync(void)
{
    BT_DRV_REG_OP_ENTER();
    
    int ret = -1;
    uint32_t sco_to_threshold_addr = 0x0;
    // TODO: [dbg_bt_setting.sco_to_threshold] address based on CHIP id
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 ||hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
    {
        sco_to_threshold_addr = 0xc00057b8;
        ret = 0;
    }
    else if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_5)
    {
        sco_to_threshold_addr = 0xc0005824;
        ret = 0;    
    }

    if(ret == 0)
    {
        *(volatile uint16_t *)sco_to_threshold_addr = 0xffff;
    }
    
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_unlock_sniffer_sco_resync(void)
{
    BT_DRV_REG_OP_ENTER();
    
    int ret = -1;
    uint32_t sco_to_threshold_addr = 0x0;
    // TODO: [dbg_bt_setting.sco_to_threshold] address based on CHIP id
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 || hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
    {
        sco_to_threshold_addr = 0xc00057b8;
        ret = 0;
    }
    else if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_5)
    {
        sco_to_threshold_addr = 0xc0005824;
        ret = 0;    
    }
    if(ret == 0)
    {
        *(volatile uint16_t *)sco_to_threshold_addr = 0x0640;
    }
    
    BT_DRV_REG_OP_EXIT();
}


extern uint8_t esco_retx_nb;
static uint8_t retx_att_nb = 0;
uint8_t bt_drv_reg_op_retx_att_nb_manager(uint8_t mode)
{
    BT_DRV_REG_OP_ENTER();
    //modify by luofei
    int ret = -1;
    uint32_t sco_evt_ptr = 0x0;
    // TODO: [ld_sco_env address] based on CHIP id
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 || hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
    {
        sco_evt_ptr = *(volatile uint32_t *)0xc00008ec;
        ret = 0;
    }
    else if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_5)
    {
        sco_evt_ptr = *(volatile uint32_t *)0xc000094c;
        ret = 0;    
    }    
    if(ret == 0)           
    {
        uint32_t retx_ptr=0x0;         
        if(sco_evt_ptr !=0)
        {
            //offsetof(struct ea_elt_tag, env) + sizeof(struct ld_sco_evt_params)
            retx_ptr =sco_evt_ptr+0x44;
        }
        else
        {
            TRACE("Error, ld_sco_env[0].evt ==NULL");
            ret = -2;
        }

        if(ret == 0)
        {
            do
            {
                if(mode != RETX_NEGO)
                {
                    TRACE("ZERO retx_att_nb =%x,retx ptr=%x",*(volatile uint8_t *)retx_ptr,retx_ptr);
                    if(*(volatile uint8_t *)retx_ptr == 0)
                    {
                        TRACE("Get retx_att_nb error ");
                        ret = -3;
                        retx_att_nb = esco_retx_nb;
                        TRACE("Get retx_att_nb use master share:%d\n ",retx_att_nb);
                        break;  
                    }
                    //record the number of sco retx 
                    retx_att_nb = *(volatile uint8_t *)retx_ptr;
                    *(volatile uint8_t *)retx_ptr = mode;
                }
                else 
                {
                    if((retx_att_nb == 0) ||( esco_retx_nb>retx_att_nb))
                    {
                        TRACE("Set retx_att_nb error ");
                        ret = -4;
                        retx_att_nb = esco_retx_nb;
                        TRACE("Set retx_att_nb use master share:%d\n ",retx_att_nb);
                    }
                    //resume the number of sco retx after op
                    *(volatile uint8_t *)retx_ptr = retx_att_nb;
                    retx_att_nb =0;
                    TRACE("resume retx_att_nb =%x,retx ptr=%x",*(volatile uint8_t *)retx_ptr,retx_ptr);
                }
            }while(0);
        }
    }
    TRACE("%s,ret=%d",__func__,ret);
    BT_DRV_REG_OP_EXIT();
    return retx_att_nb;
}


uint16_t em_bt_bitoff_getf(int elt_idx)
{
    uint16_t localVal = BTDIGITAL_BT_EM(EM_BT_BITOFF_ADDR + elt_idx * BT_EM_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x000003FF)) == 0);
    return (localVal >> 0);
}

void em_bt_bitoff_setf(int elt_idx, uint16_t bitoff)
{
    ASSERT_ERR((((uint16_t)bitoff << 0) & ~((uint16_t)0x000003FF)) == 0);
    BTDIGITAL_EM_BT_WR(EM_BT_BITOFF_ADDR + elt_idx * BT_EM_SIZE, (uint16_t)bitoff << 0);
}

void em_bt_clkoff0_setf(int elt_idx, uint16_t clkoff0)
{
    ASSERT_ERR((((uint16_t)clkoff0 << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    BTDIGITAL_EM_BT_WR(EM_BT_CLKOFF0_ADDR + elt_idx * BT_EM_SIZE, (uint16_t)clkoff0 << 0);
}

uint16_t em_bt_clkoff0_getf(int elt_idx)
{
    uint16_t localVal = BTDIGITAL_BT_EM(EM_BT_CLKOFF0_ADDR + elt_idx * BT_EM_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}
void em_bt_clkoff1_setf(int elt_idx, uint16_t clkoff1)
{
    ASSERT_ERR((((uint16_t)clkoff1 << 0) & ~((uint16_t)0x000007FF)) == 0);
    BTDIGITAL_EM_BT_WR(EM_BT_CLKOFF1_ADDR + elt_idx * BT_EM_SIZE, (uint16_t)clkoff1 << 0);
}

uint16_t em_bt_clkoff1_getf(int elt_idx)
{
    uint16_t localVal = BTDIGITAL_BT_EM(EM_BT_CLKOFF1_ADDR + elt_idx * BT_EM_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x000007FF)) == 0);
    return (localVal >> 0);
}

void em_bt_wincntl_pack(int elt_idx, uint8_t rxwide, uint16_t rxwinsz)
{
    ASSERT_ERR((((uint16_t)rxwide << 15) & ~((uint16_t)0x00008000)) == 0);
    ASSERT_ERR((((uint16_t)rxwinsz << 0) & ~((uint16_t)0x00000FFF)) == 0);
    BTDIGITAL_EM_BT_WR(EM_BT_WINCNTL_ADDR + elt_idx * BT_EM_SIZE,  ((uint16_t)rxwide << 15) | ((uint16_t)rxwinsz << 0));
}

void bt_drv_reg_op_update_sniffer_bitoffset(uint16_t mobile_conhdl,uint16_t master_conhdl)
{
#if 0
    // only slave can come in, please config CS cautiously!!!
    BT_DRV_REG_OP_ENTER();
    int ret = -1;
    uint32_t connect_env_addr = 0x0;
    uint32_t acl_par_addr = 0x0;
    uint32_t acl_par_ptr =0;
    short diff_bitoff = 0;
    // TODO: [connect_env] address based on CHIP id
    
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3)
    {
        connect_env_addr = 0xc00061d4;
        acl_par_addr = 0xc00008fc;
        ret = 0;
    }
    
    if(ret == 0)           
    {
        uint8_t mobile_idx = btdrv_conhdl_to_linkid(mobile_conhdl);
        uint8_t master_idx = btdrv_conhdl_to_linkid(master_conhdl);

        acl_par_ptr = *(uint32_t *)(acl_par_addr + mobile_idx*4);

        if(acl_par_ptr == 0)
        {
            TRACE("Error, acl_par_ptr == NULL");
            return;
        }
        uint32_t rx_win_size =  *(uint32_t *)(acl_par_ptr+104);
        
        struct ld_sniffer_connect_env * connect_env = (struct ld_sniffer_connect_env *)connect_env_addr;  
        //bitoffset 10 bit
        uint16_t mobile_bitoffset = em_bt_bitoff_getf(mobile_idx);
        //clock offset 32 bit
        uint32_t mobile_clockoffset = (em_bt_clkoff1_getf(mobile_idx) << 16) | em_bt_clkoff0_getf(mobile_idx);  
        //bitoffset 10 bit
        uint16_t master_bitoffset = em_bt_bitoff_getf(master_idx);
        //clock offset 32 bit
        uint32_t master_clockoffset = (em_bt_clkoff1_getf(master_idx) << 16) | em_bt_clkoff0_getf(master_idx);  

        uint32_t temp_clock_off = (connect_env->clk_offset +master_clockoffset)& MAX_SLOT_CLOCK;
        
        uint16_t temp_bit_off = connect_env->bitoff + master_bitoffset;

        //increase the ACL sync window 
        temp_bit_off -= (rx_win_size/2);
        if(temp_bit_off < 0)
        {
            temp_bit_off += SLOT_SIZE;
            temp_clock_off +=1;
        }    
        if(temp_bit_off > SLOT_SIZE)
        {
            temp_bit_off -=SLOT_SIZE;
            temp_clock_off -=1;
        }        
        //check if need to update the bt clock of mobile link
        diff_bitoff = abs((short)(temp_bit_off - mobile_bitoffset));
        
        if(diff_bitoff >5)
        {
            ret = 1;
        }

        if(ret == 1)
        {
            TRACE("%s 1,m_bit=%x,m_clk=%x,cal clk=%x,bit=%x,diff =%d",__func__,
                mobile_bitoffset,mobile_clockoffset,temp_clock_off,temp_bit_off,diff_bitoff);
                TRACE("env clk= 0x%x, bit off 0x%x",connect_env->clk_offset,connect_env->bitoff);
            //update all BT info
                TRACE("clk off2 0x%x, bit off 0x%x, last clk off 0x%x, last bit off 0x%x",
             *(uint32_t *)(acl_par_ptr+140),*(uint16_t *)(acl_par_ptr+150),
            *(uint32_t *)(acl_par_ptr+136),((*(uint32_t *)(acl_par_ptr+150))&0xFFFF0000)>>16);
                        
            //*(uint32_t *)(acl_par_ptr+140) = temp_clock_off& MAX_SLOT_CLOCK;
            //*(uint16_t *)(acl_par_ptr+150) = temp_bit_off;
            *(uint32_t *)(acl_par_ptr+136) =  temp_clock_off& MAX_SLOT_CLOCK;
            *(uint16_t *)(acl_par_ptr+152) = temp_bit_off;

            //increase the rx window
            //em_bt_wincntl_pack(mobile_idx,0,100);
            // update new bit offset and clock offset
           // em_bt_bitoff_setf(mobile_idx, temp_bit_off);
          //  em_bt_clkoff0_setf(mobile_idx,temp_clock_off & EM_BT_CLKOFF0_MASK);
         //   em_bt_clkoff1_setf(mobile_idx,(temp_clock_off>> 16)); 
            
            TRACE("clk off 3 0x%x, bit off 0x%x, last clk off 0x%x, last bit off 0x%x",
             *(uint32_t *)(acl_par_ptr+140),*(uint16_t *)(acl_par_ptr+150),
                *(uint32_t *)(acl_par_ptr+136),((*(uint32_t *)(acl_par_ptr+150))&0xFFFF0000)>>16);
        }
    }
    BT_DRV_REG_OP_EXIT();
#endif        
}

void bt_drv_reg_op_modify_bitoff_timer(uint16_t time_out)
{
    BT_DRV_REG_OP_ENTER();
    
    int ret = -1;
    uint32_t sco_to_threshold_addr = 0x0;
    // TODO: [dbg_bt_setting.send_connect_info_to] address based on CHIP id
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 || hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
    {
        sco_to_threshold_addr = 0xc00057b6;
        ret = 0;
    }
    else if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_5)
    {
        sco_to_threshold_addr = 0xc0005822;
        ret = 0;    
    }  
    if(ret == 0)
    {
        *(volatile uint16_t *)sco_to_threshold_addr = time_out;
    }
    TRACE("%s,ret=%d,timer =%d",__func__,ret,time_out);
    BT_DRV_REG_OP_EXIT();
}

#ifdef __LARGE_SYNC_WIN__
void bt_drv_reg_op_enlarge_sync_winsize(uint16_t conhdl, bool on)
{
  
    // modify the winsize ,ld_acl_rx_sync2: 62us,ld_acl_rx_no_sync_120us
    // To follow the phone's PCA clock dragging
    
    BT_DRV_REG_OP_ENTER();
    // TODO: fuck patch address
    int ret = -1;
    uint32_t winsize_addr = 0x0;

    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 || hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
    {
        winsize_addr = 0xc0006bfc;
        ret = 0;
    }
////t6  should check!    
    if(ret == 0)
    {
        uint8_t sniffer_sco_idx = btdrv_conhdl_to_linkid(conhdl);
        if(on)
        {
            *(volatile uint32_t *)winsize_addr = sniffer_sco_idx;
        }
        else
        {
            *(volatile uint32_t *)winsize_addr = 0xff;
        }
    }
    TRACE("%s,ret=%d,handle =%x,on=%d",__func__,ret,conhdl,on);
    BT_DRV_REG_OP_EXIT();
}
#endif

void bt_drv_reg_cs_monitor(void)
{
    uint32_t addr;
    addr = BT_EM_ADDR_BASE+0x1e;
    TRACE("AFH 0:");
    DUMP8("%02x ",(uint8_t *)addr,10);
    addr = BT_EM_ADDR_BASE + 0x1e + 110;
    TRACE("AFH 1:");
    DUMP8("%02x ",(uint8_t *)addr,10);
    addr = BT_EM_ADDR_BASE + 0x1e + 220;
    TRACE("AFH 2:");
    DUMP8("%02x ",(uint8_t *)addr,10);  
    uint32_t tmp1,tmp2,tmp3;
    tmp1 = BT_EM_ADDR_BASE+0x8;
    tmp2 = BT_EM_ADDR_BASE+0x8+110;
    tmp3 = BT_EM_ADDR_BASE+0x8+220;
    TRACE("AFH EN:%x %x %x ",*(uint16_t *)tmp1,*(uint16_t *)tmp2,*(uint16_t *)tmp3);
    tmp1 = BT_EM_ADDR_BASE+0x28;
    tmp2 = BT_EM_ADDR_BASE+0x28+110;
    tmp3 = BT_EM_ADDR_BASE+0x28+220;    
    TRACE("AFH ch num:%x %x %x ",*(uint16_t *)tmp1,*(uint16_t *)tmp2,*(uint16_t *)tmp3);    

    tmp1 = BT_EM_ADDR_BASE+0x4;
    tmp2 = BT_EM_ADDR_BASE+0x4 + 110;
    tmp3 = BT_EM_ADDR_BASE+0x4 + 220;   
    TRACE("clk off:%x %x %x ",*(uint32_t *)tmp1,*(uint32_t *)tmp2,*(uint32_t *)tmp3);   

    tmp1 = BT_EM_ADDR_BASE+0x2;
    tmp2 = BT_EM_ADDR_BASE+0x2+110;
    tmp3 = BT_EM_ADDR_BASE+0x2+220; 
    TRACE("bitoff:%x %x %x ",*(uint16_t *)tmp1,*(uint16_t *)tmp2,*(uint16_t *)tmp3);    

}

void bt_drv_reg_op_ble_llm_substate_hacker(void)
{
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 || hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
    {
    TRACE("llm_le_env.task_substate 0x%x",*(volatile uint32_t*)(0xc0006260+0x84));
    *(volatile uint32_t*)(0xc0006260+0x84) &= ~0x1;
    TRACE("After op,task_substate 0x%x",*(volatile uint32_t*)(0xc0006260+0x84));
    }
    else if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_5)
    {
        TRACE("llm_le_env.task_substate 0x%x",*(volatile uint32_t*)(0xc0006344+0x84));
        *(volatile uint32_t*)(0xc0006344+0x84) &= ~0x1;
        TRACE("After op,task_substate 0x%x",*(volatile uint32_t*)(0xc0006344+0x84));
        
    }
}

bool  bt_drv_reg_op_check_esco_acl_sniff_conflict(uint16_t hciHandle)
{
      bool check_res=false;
       int ret = -1;
       //acl
       uint32_t acl_evt_ptr = 0x0;
       uint32_t acl_sniff_anchor_point=0;
       uint32_t acl_sniff_anchor_offset=0;
       uint32_t acl_linkid =btdrv_conhdl_to_linkid(hciHandle);
       uint32_t  acl_sniff_anchor_point_addr=0x0;

    //sco
       uint32_t sco_evt_ptr = 0x0;
       uint32_t sco_anchor_point=0;
       uint32_t t_esco=12;
       uint32_t sco_anchor_point_addr=0x0; 
       uint32_t sco_tesco_point_addr=0x0;
       
       // TODO: [ld_sco_env address] based on CHIP id
       if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 || hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
       {
           sco_evt_ptr = *(volatile uint32_t *)0xc00008ec;//ld_sco_env
           acl_evt_ptr=*(volatile uint32_t *)(0xc00008fc+acl_linkid*4);//ld_acl_env
           ret = 0;
       }
        else if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_5)
        {
           sco_evt_ptr = *(volatile uint32_t *)0xc000094c;//ld_sco_env
           acl_evt_ptr=*(volatile uint32_t *)(0xc000095c+acl_linkid*4);//ld_acl_env
           ret = 0;            
        }      
       
       if(ret == 0)           
       {
            if(sco_evt_ptr !=0)
            {
                sco_anchor_point_addr =sco_evt_ptr+0x34;//sco anchor_point 
                sco_tesco_point_addr=sco_evt_ptr+0x42;//tesco
            }
            else
            {
                ret = -1;
            }
  
            if(acl_evt_ptr !=0)
            {
                acl_sniff_anchor_point_addr =acl_evt_ptr+0xd4;//acl anchor_point 
            }
            else
            {
                ret = -2;
            }
        } 
        if(ret==0)
        {
            do{
                     //sco anchor point get
                    if(*(volatile uint32_t *)sco_anchor_point_addr == 0)
                    {
                        ret = -3;
                        break;  
                    }
                    //sco tesco get
                    if(*(volatile uint8_t *)sco_tesco_point_addr == 0)
                    {
                        ret = -4;
                        break;  
                    }
                    
                    //record the number of sco anchor point
                    sco_anchor_point = *(volatile uint32_t *)sco_anchor_point_addr;
                    t_esco = *(volatile uint8_t *)sco_tesco_point_addr;

                    //acl sniff anchor point get
                    if(*(volatile uint32_t *)acl_sniff_anchor_point_addr == 0)
                    {
                        ret = -5;
                        break;  
                    }
                    //record the number of sco anchor point
                    acl_sniff_anchor_point = *(volatile uint32_t *)acl_sniff_anchor_point_addr;
                      if(acl_sniff_anchor_point>= sco_anchor_point)
                     { 
                         acl_sniff_anchor_offset =((acl_sniff_anchor_point -sco_anchor_point)%t_esco);
                      }
                    else
                    {
                        acl_sniff_anchor_offset =((sco_anchor_point-acl_sniff_anchor_point )%t_esco);
                    }
                           
                   if((acl_sniff_anchor_offset<4)||(acl_sniff_anchor_offset>8))
                        check_res = true;
                }while(0);
     }
        
    static uint8_t trace_counter = 0;
    if(++trace_counter>100)
    {
        TRACE("%s,ret=%d res:%d offset:%x\n",__func__,ret,check_res,acl_sniff_anchor_offset);
        trace_counter = 0;
    }
    return check_res;
}

void bt_drv_reg_op_esco_acl_sniff_delay_cal(uint16_t hciHandle,bool enable)
{
    int ret = -1;
    //acl
    uint32_t acl_evt_ptr = 0x0;
    uint32_t acl_sniff_anchor_point=0;
    uint32_t acl_sniff_anchor_offset=0;
    uint32_t acl_linkid =btdrv_conhdl_to_linkid(hciHandle);
    uint32_t last_sync_clk_off;
    uint32_t master_clock;
    uint32_t last_sync_clk_off_addr=0x0;
    uint32_t clock=(btdrv_syn_get_curr_ticks()+1)>>1;//321.5us--625
    int diff;
    //lc
    uint32_t lc_evt_ptr = 0;
    uint32_t host_set_dely_addr = 0;
    //sco
    uint32_t sco_evt_ptr = 0x0;
    uint32_t sco_par_anchor_point=0;
    uint32_t t_esco=12;
    uint32_t interval=60,delay=0;
    uint32_t anchor_point=0x0;     // sco anchor_point  
    uint32_t sco_tesco_point_addr=0x0;   
  
    // TODO: [ld_sco_env address] based on CHIP id
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 || hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
    {
        sco_evt_ptr = *(volatile uint32_t *)0xc00008ec;//ld_sco_env
        acl_evt_ptr=*(volatile uint32_t *)(0xc00008fc+acl_linkid*4);//ld_acl_env
        lc_evt_ptr = *(volatile uint32_t *)(0xc0005b48+acl_linkid*4);//lc_env
        ret = 0;
    }
    else if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_5)
    {
       sco_evt_ptr = *(volatile uint32_t *)0xc000094c;//ld_sco_env
       acl_evt_ptr=*(volatile uint32_t *)(0xc000095c+acl_linkid*4);//ld_acl_env
       lc_evt_ptr = *(volatile uint32_t *)(0xc0005bcc+acl_linkid*4);//lc_env
       ret = 0;            
    }       
    if(enable==false)
    {
        if(lc_evt_ptr !=0)
        {
            host_set_dely_addr =lc_evt_ptr+0x67;//lc
            *(volatile uint8_t *)host_set_dely_addr=0;
        }
        return;
    }
    
    if(ret == 0)           
    {
        if(sco_evt_ptr !=0)
        {
            anchor_point =sco_evt_ptr+0x34;//sco anchor_point 
            sco_tesco_point_addr=sco_evt_ptr+0x42;//tesco
        }
        else
        {
            ret = -1;
        }

        if(acl_evt_ptr !=0)
        {
            last_sync_clk_off_addr =acl_evt_ptr+0x88;
        }
        else
        {
            ret = -2;
        }
        
         if(lc_evt_ptr !=0)
        {
            host_set_dely_addr =lc_evt_ptr+0x67;//lc
        }
        else
        {
            ret = -3;
        }

        
        if(ret == 0)
        {
            do
            {
                //sco anchor point get
                if(*(volatile uint32_t *)anchor_point == 0)
                {
                    ret = -4;
                    break;  
                }

                //sco tesco get
                if(*(volatile uint8_t *)sco_tesco_point_addr == 0)
                {
                    ret = -5;
                    break;  
                }
                //record the number of sco anchor point
                sco_par_anchor_point = *(volatile uint32_t *)anchor_point;
                t_esco = *(volatile uint8_t *)sco_tesco_point_addr;

                //last_sync_clk_off get
                last_sync_clk_off =*(volatile uint32_t *)last_sync_clk_off_addr;

                master_clock =(clock+last_sync_clk_off) &0x07FFFFFF;
                diff = 0- ((master_clock ^ (1 << 26)) %  interval); 
                while (diff <= 0)
                {
                    diff += interval;
                }
                diff -= interval;
                acl_sniff_anchor_point = (clock+diff)&0x07FFFFFF;
                TRACE("acl_sniff_anchor_point:%x %x \n",acl_sniff_anchor_point,sco_par_anchor_point );

                 if(acl_sniff_anchor_point >= sco_par_anchor_point )
                { 
                        acl_sniff_anchor_offset =((acl_sniff_anchor_point-sco_par_anchor_point)%t_esco);
                        delay =t_esco-acl_sniff_anchor_offset+2;
                        TRACE("offset0:%x delay:%x\n",acl_sniff_anchor_offset,delay);
                }
                else
                {
                        acl_sniff_anchor_offset =((sco_par_anchor_point-acl_sniff_anchor_point )%t_esco);
                        delay =acl_sniff_anchor_offset+2;
                        TRACE("offset1:%x delay:%x\n",acl_sniff_anchor_offset,delay);
                }
                TRACE("Delay:%x  \n",delay);
                if(delay%2)/*must even*/
                    delay+=1;

                *(volatile uint8_t *)host_set_dely_addr=1;
                *(volatile uint8_t *)(host_set_dely_addr+1) = delay ;
                TRACE("host_set_dely_addr =%x,value=%x",host_set_dely_addr,*(volatile uint32_t *)host_set_dely_addr);
            }while(0);
        }
    }
    TRACE("%s,ret=%d",__func__,ret);
}




uint8_t  bt_drv_reg_op_get_role(uint8_t linkid)
{
    uint32_t lc_evt_ptr=0;
    uint32_t role_ptr = 0;
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 || hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
    {
        lc_evt_ptr = *(volatile uint32_t *)(0xc0005b48+linkid*4);//lc_env
    }
    else if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_5)
    {
       lc_evt_ptr = *(volatile uint32_t *)(0xc0005bcc+linkid*4);//lc_env
    }
    if(lc_evt_ptr !=0)
    {
        role_ptr = lc_evt_ptr+0x40;
    }
    else
    {
        TRACE("ERROR LINKID =%x",linkid);
        return 0xFF;
    }
    return *(uint8_t *)role_ptr;
}

void bt_drv_reg_op_set_tpoll(uint8_t linkid,uint16_t poll_interval)
{
  //  return;
    uint32_t acl_evt_ptr = 0x0;
    uint32_t poll_addr;
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 || hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
    {
        acl_evt_ptr=*(volatile uint32_t *)(0xc00008fc+linkid*4);//ld_acl_env
    }
    else
    {
        acl_evt_ptr=*(volatile uint32_t *)(0xc000095c+linkid*4);//ld_acl_env
    }
    if(acl_evt_ptr !=0)
    {
        poll_addr = acl_evt_ptr+0xb8;
        *(uint16_t *)poll_addr = poll_interval;
    }
    else
    {
        TRACE("ERROR LINK ID FOR TPOLL %x",linkid);
    }
}



void bt_drv_reg_set_rssi_seed(uint32_t seed)
{
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_3 || hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_4)
    {
    *(uint32_t *)0xC0006660 = seed;
    }
    else if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_5)
    {
        *(uint32_t *)0xC0006750 = seed;
    }
}

/***********************************************************************************
* function: bt_drv_rssi_correction
* rssi<=-94, rssi=127(invalid)
* rssi<=-45, rssi
* rssi<=-30, rssi-3
* rssi<=-14, rssi-7
* rssi<= 27, rssi-5
* rssi>  27, rssi=127(invalid)
***********************************************************************************/
int8_t  bt_drv_rssi_correction(int8_t rssi)
{
#ifdef __HW_AGC__
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_5)
    {
        if(rssi <= -94)
            rssi = 127;
        else if(rssi <= -45)
            rssi = rssi;
        else if(rssi <= -30)
            rssi -= 3;
        else if(rssi <= 0)
            rssi -= 7;
        else if(rssi <= 27)
            rssi -= 5;
        else
            rssi = 127;
    }
#endif

    return rssi;
}



void bt_drv_set_music_link(uint8_t link_id)
{
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_5)
    {
        *(uint8_t *)0xc000582c = link_id;
    }
}

void bt_drv_set_music_link_duration_extra(uint8_t slot)
{
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_5)
    {
        *(uint32_t *)0xc0005830 = slot*625;
    }
}



void bt_drv_set_hwagc_read_en(uint8_t en)
{
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_5)
    {

    *(uint8_t *)0xC000419f = en;
    }
}


extern "C" void bt_drv_sleep(void)
{
#ifdef BT_RSSI_MONITOR
    bt_drv_set_hwagc_read_en(0);
#endif
}


extern "C" void bt_drv_wakeup(void)
{
#ifdef BT_RSSI_MONITOR
    bt_drv_set_hwagc_read_en(1);
#endif
}



void bt_dev_music_link_config(uint16_t active_link,uint8_t active_role,uint16_t inactive_link,uint8_t inactive_role)
{
    TRACE("bt_dev_music_link_config %x %d %x %d",active_link,active_role,inactive_link,inactive_role);
    if(active_role == 0)//MASTER
    {
        bt_drv_reg_op_set_tpoll(active_link-0x80,0x10);
        if(inactive_role == 0)
        {
            bt_drv_reg_op_set_tpoll(inactive_link-0x80,0x40);
        }
    }
    else
    {
        bt_drv_set_music_link(active_link-0x80);
        
        if(inactive_role == 0)
        {
            bt_drv_set_music_link_duration_extra(11);
            bt_drv_reg_op_set_tpoll(inactive_link-0x80,0x40);
        }
        else//dual_slave
        {
            bt_drv_set_music_link_duration_extra(25);
        }  
    }
}

void bt_drv_reg_afh_env_reset(void)
{
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_5)
    {
        struct ld_afh_env* ld_afh_env = (struct ld_afh_env*)0xc0005e28;
        ld_afh_env->rf_rssi_interf_thr = BT_AFH_RSSI_INTERF_THR;
        ld_afh_env->afh_update_period = BT_AFH_UPDATE_PERIOD;
        ld_afh_env->afh_access_valid_to = BT_AFH_ASSESS_VALID_TO;
        ld_afh_env->afh_reaccess_to = BT_AFH_REASSESS_TO;
        ld_afh_env->afh_access_count_max = BT_AFH_ASSESS_COUNT_MAX;
        ld_afh_env->afh_access_count_thr_good = BT_AFH_ASSESS_COUNT_THR_GOOD;
        ld_afh_env->afh_access_count_thr_bad = BT_AFH_ASSESS_COUNT_THR_BAD;
        ld_afh_env->afh_access_count_min = BT_AFH_ASSESS_COUNT_MIN;
    }
}

void bt_drv_pcm_set(uint8_t en)
{
}

void bt_drv_clear_skip_flag()
{
}
void bt_drv_disable_sco_avoid_ble()
{
    *(volatile uint32_t *)(0xC0006054)&=0xffff00ff;
}
const uint8_t msbc_mute_patten[]= {
    0x01,0x38,
    0xad, 0x0,  0x0,  0xc5, 0x0,  0x0, 0x0, 0x0,
    0x77, 0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7,
    0x76, 0xdb, 0x6d, 0xdd, 0xb6, 0xdb, 0x77,
    0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76,
    0xdb, 0x6d, 0xdd, 0xb6, 0xdb, 0x77, 0x6d,
    0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb,
    0x6d, 0xdd, 0xb6, 0xdb, 0x77, 0x6d, 0xb6,
    0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6c,
    0x00
};
#define SCO_TX_FIFO_BASE (0xd0210000)
void bt_drv_sco_txfifo_reset(uint16_t codec_id)
{
    uint32_t reg_val = BTDIGITAL_REG(0xd0220148);
    uint32_t reg_offset0, reg_offset1;    
    uint16_t *patten_p = (uint16_t *)msbc_mute_patten;

    reg_offset0 = (reg_val & 0xffff);
    reg_offset1 = (reg_val >> 16) & 0xffff;

    for (uint8_t i=0; i<60; i+=2){
		if (codec_id == 2){
		    BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE+reg_offset0+i) = *patten_p;
		    BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE+reg_offset1+i) = *patten_p;
		}else{
		    BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE+reg_offset0+i) = 0x5555;
		    BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE+reg_offset1+i) = 0x5555;		
		}
        patten_p++;
    }
}


