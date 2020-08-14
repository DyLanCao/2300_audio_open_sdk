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
#include "bt_drv_2300p_internal.h"
#include "bt_drv_interface.h"
#include "bt_drv.h"
#include "hal_sysfreq.h"
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
    return false;
}

void bt_drv_reg_op_ld_sniffer_env_monitored_dev_state_set(bool state)
{
}

void bt_drv_reg_ibrt_env_reset(void)
{
    struct ld_device_info * mobile_device_info;
    BT_DRV_REG_OP_ENTER();
    bool status = false;
    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0)
    {
        mobile_device_info = (struct ld_device_info *)0xc000635c;
        status = true;
    }

    if(status)
    {
        mobile_device_info->link_id = 3;
        mobile_device_info->state = 0;
    }
    BT_DRV_REG_OP_EXIT();
}

int bt_drv_reg_op_currentfreeaclbuf_get(void)
{
    BT_DRV_REG_OP_ENTER();
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
    {
        return (*(volatile uint16_t *)0xc00067c8);
    }
    else
    {
        return 0;
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
    //  BT_DRV_REG_OP_ENTER();

//    BT_DRIVER_PUT_U8_REG_VAL(0xc0000054, 1);
//   BT_DRV_REG_OP_EXIT();
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
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
        {
            memcpy((uint8_t *)0xc0006394, addr, 6);
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
    for(i=0; i<60; i++)
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
    uint8_t buf_count=0, i=0;
    uint16_t em_buf ;
    uint32_t acl_pkt_sent = *(uint16_t *)buf_ptr;
    uint32_t aclRxPacketsLeft = hci_current_left_rx_packets_left();
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
    {
        buf_ptr = (uint32_t *)0xc0005a10; //bt_util_buf_env.acl_rx_free c0005784
    }

    buf_count = 0;
    while(*buf_ptr)
    {
        buf_count++;
        buf_ptr = (uint32_t *)(*buf_ptr);
    }
    for(i=0; i<4; i++)
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
    uint8_t buf_count=0, i=0;
    uint16_t em_buf ;

    BT_DRV_REG_OP_ENTER();

    //bt_drv_reg_op_connection_checker();

    //TRACE_LVL(HAL_TRACE_LEVEL_ENV, "tx flow=%x",*(uint32_t *)0xc0003f9c);
    //TRACE_LVL(HAL_TRACE_LEVEL_ENV, "remote_tx flow=%x",*(uint32_t *)0xc0003fa2);


    //TRACE_LVL(HAL_TRACE_LEVEL_ENV, "tx buff = %x %x",*(uint16_t *)0xc0005d48,hci_current_left_tx_packets_left());
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0 )
    {
        buf_ptr = (uint32_t *)0xc0005a10; //bt_util_buf_env.acl_rx_free c0005784
    }

    buf_count = 0;
    while(*buf_ptr)
    {
        buf_count++;
        buf_ptr = (uint32_t *)(*buf_ptr);
    }
    for(i=0; i<4; i++)
    {
        em_buf = BT_DRIVER_GET_U16_REG_VAL(EM_BT_RXACLBUFPTR_ADDR + i*14);
        if(em_buf != 0)
            buf_count++;
    }
    TRACE("rxbuff = %x %x , host rx buff in controller = %x",buf_count,hci_current_left_rx_packets_left(),BT_DRIVER_GET_U16_REG_VAL(0xc0006340));

    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
    {
        buf_ptr = (uint32_t *)0xc0005a24; //bt_util_buf_env.acl_tx_free c0005784
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
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
    {
        TRACE("BT metal id=%d", hal_get_chip_metal_id());
        //first move R3 to R9, lost R9
        TRACE("BT controller BusFault_Handler:\nREG:[LR] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE));
        TRACE("REG:[R0] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +120));
        TRACE("REG:[R1] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +124));
        TRACE("REG:[R2] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +128));
        TRACE("REG:[R3] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +132));
        TRACE("REG:[R4] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +4));
        TRACE("REG:[R5] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +8));
        TRACE("REG:[R6] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +12));
        TRACE("REG:[R7] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +16));
        TRACE("REG:[R8] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +20));
        //TRACE("REG:[R9] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +24));
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
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
        {
            buff_addr = 0xc00007cc;
        }
        TRACE("task msg buff:");
        for(uint8_t j=0; j<5; j++)
        {
            DUMP8("%02x ", (uint8_t *)(buff_addr+j*20), 20);
        }
        TRACE("");
        TRACE("lmp buff:");
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
        {
            buff_addr = 0xc0000834;
        }
        for(uint8_t j=0; j<10; j++)
        {
            DUMP8("%02x ",(uint8_t *)(buff_addr+j*20), 20);
        }

        uint8_t link_id = 0;
        uint32_t evt_ptr = 0;
        uint32_t acl_par_ptr = 0;
        for(link_id=0; link_id<3; link_id++)
        {
            TRACE("acl_par: link id %d",link_id);
            if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
            {
                evt_ptr = *(uint32_t *)(0xc000098c+link_id*4);
            }

            acl_par_ptr = evt_ptr;
            TRACE("acl_par: acl_par_ptr 0x%x, clk off 0x%x, bit off 0x%x, last sync clk off 0x%x, last sync bit off 0x%x",
                  acl_par_ptr, *(uint32_t *)(acl_par_ptr+140),*(uint16_t *)(acl_par_ptr+150),
                  *(uint32_t *)(acl_par_ptr+136),((*(uint32_t *)(acl_par_ptr+150))&0xFFFF0000)>>16);
        }
        if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
        {
            evt_ptr = *(uint32_t *)(0xc0000980);
        }

        TRACE("esco linkid :%d",*(uint8_t *)(evt_ptr+70));
        for(link_id=0; link_id<3; link_id++)
        {
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
    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_0)
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
    return;
}



void bt_drv_reg_op_set_swagc_mode(uint8_t mode)
{
    return;
}

void bt_drv_reg_op_set_reboot_pairing_mode(uint8_t mode)
{
    return;
}

void bt_drv_reg_op_force_retrans(bool enalbe)
{
    return;

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
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
    {
        env0 = *(uint32_t *)0xc000098c;
        env1 = *(uint32_t *)0xc0000990;
        env2 = *(uint32_t *)0xc0000994;
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

struct rx_monitor
{
    int8_t rssi;
    uint8_t rxgain;
};

int8_t bt_drv_read_rssi_in_dbm(uint16_t connHandle)
{
    int8_t rssi = 0;
#ifdef BT_RSSI_MONITOR
    BT_DRV_REG_OP_ENTER();

    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_0)
    {
        uint8_t idx = btdrv_conhdl_to_linkid(connHandle);
        /// Accumulated RSSI (to compute an average value)
        int16_t rssi_acc = 0;
        /// Counter of received packets used in RSSI average
        uint8_t rssi_avg_cnt = 5;
        struct rx_monitor * rx_monitor_env;
        if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0)
        {
            rx_monitor_env = (struct rx_monitor *)0xc00058fc;
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
}

void bt_drv_reg_op_lock_sniffer_sco_resync(void)
{

}

void bt_drv_reg_op_unlock_sniffer_sco_resync(void)
{

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
    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_0)
    {
        sco_evt_ptr = *(volatile uint32_t *)0xc0000980;
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
            }
            while(0);
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
}

void bt_drv_reg_op_modify_bitoff_timer(uint16_t time_out)
{

}


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

}

bool  bt_drv_reg_op_check_esco_acl_sniff_conflict(uint16_t hciHandle)
{
    return false;
}

void bt_drv_reg_op_esco_acl_sniff_delay_cal(uint16_t hciHandle,bool enable)
{
}




uint8_t  bt_drv_reg_op_get_role(uint8_t linkid)
{
    uint32_t lc_evt_ptr=0;
    uint32_t role_ptr = 0;
    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_0)
    {
        lc_evt_ptr = *(volatile uint32_t *)(0xc0005c70+linkid*4);//lc_env
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
    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_0)
    {
        acl_evt_ptr=*(volatile uint32_t *)(0xc000098c+linkid*4);//ld_acl_env
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
    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_0 )
    {
        *(uint32_t *)0xC0006808 = seed;
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
    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_0)
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
    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_0)
    {
        *(uint8_t *)0xc0005864 = link_id;
    }
}

void bt_drv_set_music_link_duration_extra(uint8_t slot)
{
    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_0)
    {
        *(uint32_t *)0xc0005868 = slot*625;
    }
}



void bt_drv_set_hwagc_read_en(uint8_t en)
{

}


extern "C" void bt_drv_sleep(void)
{

}


extern "C" void bt_drv_wakeup(void)
{

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
        bt_drv_set_music_link_duration_extra(11);
        if(inactive_role == 0)
        {
            bt_drv_reg_op_set_tpoll(inactive_link-0x80,0x40);
        }
    }
}

bool bt_drv_reg_op_get_control_state(void)
{
    bool ret=true;
    BT_DRV_REG_OP_ENTER();
    if(hal_sysfreq_get() <= HAL_CMU_FREQ_32K)
        return ret;

    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_0)
    {
        if((BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +0)==0x00)
           &&(BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +8)==0xc0005920))
            ret = true;
        else
            ret = false;
    }

    BT_DRV_REG_OP_EXIT();
    return ret;
}

void bt_drv_reg_piconet_clk_offset_get(uint16_t connHandle, int32_t *clock_offset, uint16_t *bit_offset)
{
    uint8_t index = 0;
    uint32_t clock_offset_raw = 0;

    BT_DRV_REG_OP_ENTER();
    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_0)
    {
        if (connHandle)
        {
            index = btdrv_conhdl_to_linkid(connHandle);
            *bit_offset = em_bt_bitoff_getf(index);
            clock_offset_raw = (em_bt_clkoff1_getf(index) << 16) | em_bt_clkoff0_getf(index);
            *clock_offset = clock_offset_raw;
            *clock_offset = (*clock_offset << 5) >> 5;
        }
        else
        {
            *bit_offset = 0;
            *clock_offset = 0;
        }
    }
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_afh_env_reset(void)
{
    BT_DRV_REG_OP_ENTER();
    int ret = -1;
    uint32_t ld_afh_env_addr = 0x0;

    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_0)
    {
        ld_afh_env_addr = 0xc0005ecc;
        ret = 0;
    }

    if(ret == 0)
    {
        struct ld_afh_env* ld_afh_env = (struct ld_afh_env*)ld_afh_env_addr;
        if(ld_afh_env)
        {
            TRACE("BT_DRV reset ld_afh_env");
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
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_dma_tc_clkcnt_get(uint32_t *btclk, uint16_t *btcnt)
{
    BT_DRV_REG_OP_ENTER();
    *btclk = *((volatile uint32_t*)0xd02201fc);
    *btcnt = *((volatile uint32_t*)0xd02201f8);
    BT_DRV_REG_OP_EXIT();
}

void em_bt_bt_ext1_tx_silence_setf(int elt_idx, uint8_t txsilence)
{
    ASSERT_ERR((((uint16_t)txsilence << 15) & ~((uint16_t)0x00008000)) == 0);
    BTDIGITAL_EM_BT_WR(EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE,
                       (BTDIGITAL_BT_EM(EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE) & ~((uint16_t)0x00008000)) | ((uint16_t)txsilence << 15));
}

void em_bt_bt_ext1_sco_tx_silence_setf(int elt_idx, uint8_t scotxsilence)
{
    ASSERT_ERR((((uint16_t)scotxsilence << 13) & ~((uint16_t)0x00002000)) == 0);
    BTDIGITAL_EM_BT_WR(EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE,
                       (BTDIGITAL_BT_EM(EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE) & ~((uint16_t)0x00002000)) | ((uint16_t)scotxsilence << 13));
}

void bt_drv_acl_tx_silence(uint16_t connHandle, uint8_t on)
{
    return;
    BT_DRV_REG_OP_ENTER();
    uint8_t idx = btdrv_conhdl_to_linkid(connHandle);
    TRACE("BT ACL tx silence idx=%d,on=%d\n",idx,on);

    if(TX_SILENCE_ON == on)
    {
        //config ACL silence cs
        em_bt_bt_ext1_tx_silence_setf(idx, TX_SILENCE_ON);
        if(hal_get_chip_metal_id()>= HAL_CHIP_METAL_ID_1)
        {
            //config SCO silence cs
            em_bt_bt_ext1_sco_tx_silence_setf(idx, TX_SILENCE_ON);
            //select ACL silence
            BTDIGITAL_REG_SET_FIELD(0xd02204a8,3,2,2);
        }
    }
    else if(TX_SILENCE_OFF == on)
    {
        //config ACL silence cs
        em_bt_bt_ext1_tx_silence_setf(idx, TX_SILENCE_OFF);
        if(hal_get_chip_metal_id()>= HAL_CHIP_METAL_ID_1)
        {
            //config SCO silence cs
            em_bt_bt_ext1_sco_tx_silence_setf(idx, TX_SILENCE_OFF);
            //select ACL silence
            BTDIGITAL_REG_SET_FIELD(0xd02204a8,3,2,3);
        }
    }
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reset_sw_seq_filter(uint16_t connHandle)
{
}

void bt_drv_pcm_set(uint8_t en)
{
}

void bt_drv_clear_skip_flag()
{
}


void bt_drv_clean_flags_of_ble_and_sco(void)
{
#if 0
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_0)
    {
        //0xc0006a18,////sco_find_sync_flag
        //0xc000699c,///ble_sco_need_move_flag
        //0xc000693c,///prev_conflict_type_record
        //0xc000099f,///anchor_resume_flag
        ASSERT(*(uint32_t *)0xc0006a14 ==0xc0006a18,"ERROR 2300pt1 sco_find_sync_flag!");
        ASSERT(*(uint32_t *)0xc0006998 ==0xc000699c,"ERROR 2300pt1 ble_sco_need_move_flag!");
        ASSERT(*(uint32_t *)0xc0006938 ==0xc000693c,"ERROR 2300pt1 prev_conflict_type_record!");
        //TRACE("0xc0006a18=%x,0xc000699c=%x,0xc000693c=%x,0xc000099f=%x\n",*(volatile uint32_t *)0xc0006a18,*(volatile uint32_t *)0xc000699c,*(volatile uint32_t *)0xc000693c,*(volatile uint32_t *)0xc000099f);
        *(volatile uint32_t *)0xc0006a18 = 0;
        *(volatile uint32_t *)0xc0006a88 = 0;
        *(volatile uint32_t *)0xc000693c = 0;
        *(volatile uint32_t *)0xc000099f = 0;
    }
#endif
}

void bt_drv_sco_txfifo_reset(uint16_t codec_id)
{
}

#if defined(IBRT)
void bt_drv_reg_op_clean_ibrt_sco_flags(void)
{
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_0)
    {
        ASSERT(*(uint32_t *)0xc0006c5c ==0xc0006C60,"ERROR [2300pt1 patch 18]:first_handshack_flag!");
        ASSERT(*(uint32_t *)0xc0006b90 ==0xc0006b94,"ERROR [2300pt1 patch 18]:first_sco_recored!");

        TRACE("BT_reg_op:first_handshack_flag=%x,first_sco_recored=%x",*(volatile uint32_t *)0xc0006C60,*(volatile uint32_t *)0xc0006b94);
        *(volatile uint32_t *)0xc0006C60 = 0x00000000;
        *(volatile uint32_t *)0xc0006b94 = 0x07ffffff;

    }
}

void bt_drv_hack_start_snoop_in_sco(void)
{
    if(hal_get_chip_metal_id()==HAL_CHIP_METAL_ID_0)
    {
        ASSERT(*(uint32_t *)0xc0006AC4 ==0xc0006AC8,"ERROR [2300p t1 patch 10]");

        *(volatile uint32_t *)0xc0006AC8 = 0x1;
        TRACE("BT_reg_op: set start snoop falg",*(volatile uint32_t *)0xc0006AC8);
    }
}

/*****************************************************************************
 Prototype    : btdrv_set_tws_acl_poll_interval
 Description  : in ibrt mode, set tws acl poll interval
 Input        : uint16_t poll_interval
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/19
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_set_tws_acl_poll_interval(uint16_t poll_interval)
{
    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0)
    {
        //dbg_bt_setting.acl_interv_in_snoop_mode
        *(volatile uint16_t *)0xC0005894 = poll_interval;
    }
    else
    {
        //fill in when tape out
    }
}

void btdrv_reg_op_set_tws_link_duration(uint8_t slot_num)
{
    BT_DRV_REG_OP_ENTER();
    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0)
    {
        //uint16_t acl_slot = dbg_setting->acl_slot_in_snoop_mode;
        uint16_t val = *(volatile uint16_t *)(0xc0005854+0x44);
        val&=0xff00;
        val|= slot_num;
        *(volatile uint16_t *)(0xc0005854+0x44) = val;
        TRACE("SET tws_link_duration,val=%x",*((volatile uint16_t*)(0xc0005854+0x44)));
    }
    else
    {
        //fill in when tape out
    }
    BT_DRV_REG_OP_EXIT();
}

void btdrv_reg_op_enable_fix_tws_poll_interval(bool enable)
{
    BT_DRV_REG_OP_ENTER();
    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0)
    {
        //sniffer_env.acl_switch_flag.in_process
        uint16_t val = *((volatile uint16_t*)(0xc0006358+0x7e)); 
        val&=0xff;
        val|=1<<8;
        *((volatile uint16_t*)(0xc0006358+0x7e)) = val;
        TRACE("Enable FIX tws interval,val=%x",*((volatile uint16_t*)(0xc0006358+0x7e)));
    }
    BT_DRV_REG_OP_EXIT();
}

void btdrv_decrease_tx_pwr_when_reconnect(bool enable)
{
    if(enable)
    {
        TRACE("Decrese tx pwr");
        //drease defualt TX pwr
        BTDIGITAL_REG(0xd0350300) = 0x33;
    }
    else
    {
        TRACE("Increase tx pwr");
        //resume defualt TX pwr
        BTDIGITAL_REG(0xd0350300) = 0x11;
    }
}
#endif
