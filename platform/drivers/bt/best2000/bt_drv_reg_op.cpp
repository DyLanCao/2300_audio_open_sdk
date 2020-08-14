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
#include "bt_drv_interface.h"
#include "bt_drv.h"
#include "hal_chipid.h"
#include "hal_trace.h"
#include "string.h"


#define BT_DRIVER_GET_U8_REG_VAL(regAddr)		(*(uint8_t *)(regAddr))
#define BT_DRIVER_GET_U16_REG_VAL(regAddr)		(*(uint16_t *)(regAddr))
#define BT_DRIVER_GET_U32_REG_VAL(regAddr)		(*(uint32_t *)(regAddr))

#define BT_DRIVER_PUT_U8_REG_VAL(regAddr, val)		*(uint8_t *)(regAddr) = (val)
#define BT_DRIVER_PUT_U16_REG_VAL(regAddr, val)		*(uint16_t *)(regAddr) = (val)
#define BT_DRIVER_PUT_U32_REG_VAL(regAddr, val)		*(uint32_t *)(regAddr) = (val)

void bt_drv_reg_op_rssi_set(uint16_t rssi)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        *(volatile uint16_t*)(0xc0003bec) = 0;         //rssi low
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        *(volatile uint16_t*)(0xc0003c7c) = 0;         //rssi low
    }
}

void bt_drv_reg_op_scan_intv_set(uint32_t scan_intv)
{

    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        *(volatile uint32_t *)0xc000566c = scan_intv;    
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        *(volatile uint32_t *)0xc0005710 = scan_intv;    
    }
}

void bt_drv_reg_op_encryptchange_errcode_reset(uint16_t hci_handle)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
       // uint32_t error_code_address = (0xc000531b +(hci_handle -0x80)*0x210);
        uint32_t error_code_address = (0xc0004edb +(hci_handle -0x80)*0x210);
        TRACE("EncryptChange error checker code= %x",*(volatile uint8_t *)(error_code_address));
        if(*(volatile uint8_t *)(error_code_address) != 0)
        {
            *(volatile uint8_t *)(error_code_address) = 0;
        }
    }
}

void bt_drv_reg_op_sco_sniffer_checker(void)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        TRACE("MASTER CHANMAP=:");
        DUMP8("%02x",(unsigned char *)0xc0004c10,10);
        TRACE("\n instant=%x,mode=%x,SCO silence REG=%x,%x",*(unsigned int *)0xc0004c1c,*(unsigned int *)0xc0004c20,*(volatile uint16_t *)(0xd0210214),*(volatile uint16_t *)(0xd0210216));
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
   {
        TRACE("MASTER CHANMAP=:");
        DUMP8("%02x",(unsigned char *)0xc0004ca4,10);
        TRACE("\n instant=%x,mode=%x,SCO silence REG=%x,%x",*(unsigned int *)0xc0004c1c,*(unsigned int *)0xc0004c20,*(volatile uint16_t *)(0xd0210214),*(volatile uint16_t *)(0xd0210216));
       
    }
}

void bt_drv_reg_op_trigger_time_checker(void)
{
    TRACE("0xd0224024 = %x,0xd022007c=%x",*(volatile uint32_t *)0xd0224024,*(volatile uint32_t *)0xd022007c);
    TRACE("0xd02201b4 = %x,0xd02201cc=%x",*(volatile uint32_t *)0xd02201b4,*(volatile uint32_t *)0xd02201cc);
}

void bt_drv_reg_op_tws_output_power_fix_separate(uint16_t hci_handle, uint16_t pwr)
{
    *(volatile uint32_t *)(0xd02101a4+btdrv_conhdl_to_linkid(hci_handle)) = pwr;
}


#define SNIFF_IN_SCO    2

bool bt_drv_reg_op_ld_sniffer_env_monitored_dev_state_get(void)
{
    uint8_t val = 0;
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        val = *(volatile uint8_t *)0xc0004bfe;
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        val = *(volatile uint8_t *)0xc0004c92;
    }
    bool nRet = false;
    
    if (val & SNIFF_IN_SCO){
        nRet = true;
    }else{
        nRet = false;
    }
    TRACE("monitored_dev_state %d", nRet);

    return nRet;
}

void bt_drv_reg_op_ld_sniffer_env_monitored_dev_state_set(bool state)
{
    uint8_t val = 0;
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        val = *(volatile uint8_t *)0xc0004bfe;
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        val = *(volatile uint8_t *)0xc0004c92;
    }

    if (state){        
        val |= (uint8_t)(SNIFF_IN_SCO);
    }else{
        val &= (uint8_t)(~SNIFF_IN_SCO);
    }
    *(volatile uint8_t *)0xc0004bfe = val;
}

void bt_drv_reg_op_ld_sniffer_master_addr_set(uint8_t * addr)
{

}

int bt_drv_reg_op_currentfreeaclbuf_get(void)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        return *(volatile uint16_t *)0xC0005D48;
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        return *(volatile uint16_t *)0xC0005DEC;
    }
    else
    {
    	return 0;
    }
}


static uint16_t app_tws_original_mobile_airpath_info;

void bt_drv_reg_op_save_mobile_airpath_info(uint16_t hciHandle)
{
    uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);
    app_tws_original_mobile_airpath_info = BT_DRIVER_GET_U16_REG_VAL(0xd0210192 + 96*link_id);
    TRACE("org mobile air info 0x%x ", app_tws_original_mobile_airpath_info);
}

void bt_drv_reg_op_block_xfer_with_mobile(uint16_t hciHandle)
{
    uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);
    BT_DRIVER_PUT_U16_REG_VAL((0xd0210192 + 96*link_id), app_tws_original_mobile_airpath_info^0x8000);

    TRACE("Set hci handle 0x%x as 0x%x", hciHandle, app_tws_original_mobile_airpath_info^0x8000);
}

void bt_drv_reg_op_resume_xfer_with_mobile(uint16_t hciHandle)
{
    uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);
    BT_DRIVER_PUT_U16_REG_VAL((0xd0210192 + 96*link_id), app_tws_original_mobile_airpath_info);
}

int bt_drv_reg_op_packet_type_checker(uint16_t hciHandle)
{
    uint32_t table_addr=0;
    uint32_t len_addr=0;
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        table_addr = 0xc0003df8;
        len_addr = 0xc0003e04;
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        table_addr = 0xc0003e88;
        len_addr = 0xc0003e94;        
    }
#if  IS_ENABLE_BT_DRIVER_REG_DEBUG_READING    
        bt_drv_reg_op_connection_checker();
#endif
        uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);
        uint32_t table = BT_DRIVER_GET_U32_REG_VAL(table_addr);
        uint32_t len = BT_DRIVER_GET_U32_REG_VAL(len_addr);
        uint16_t table_id = 0;
        
        if(link_id ==0){
            table_id = table & 0xffff;
        }else if(link_id==1){
            table_id = table >>16;
        }
        
        if(table_id == 0x8 || table_id ==0xa){
            TRACE("TX TABLE IS ONLY 1SLOT PACKET !!!");
            if(link_id ==0){
                len &= 0xffff0000;
                len |= 0x2a7;
                table |= 0x110a;
                BT_DRIVER_PUT_U32_REG_VAL(len_addr, len);
                BT_DRIVER_PUT_U32_REG_VAL(table_addr, table);
            }else if(link_id ==1){
                len &= 0xffff;
                len |= 0x2a70000;
                table |= 0x110a0000;
                BT_DRIVER_PUT_U32_REG_VAL(len_addr, len);
                BT_DRIVER_PUT_U32_REG_VAL(table_addr, table);        
            }
        }   

    return 0;
}

void bt_drv_reg_op_max_slot_setting_checker(uint16_t hciHandle)
{
#if IS_ENABLE_BT_DRIVER_REG_DEBUG_READING    
    bt_drv_reg_op_connection_checker();
#endif
    if (hciHandle<0x80 || hciHandle>0x83)
    {
        return;
    }
    uint32_t table_addr=0;
    uint32_t len_addr=0;
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        table_addr = 0xc0003df8;
        len_addr = 0xc0003e04;
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        table_addr = 0xc0003e88;
        len_addr = 0xc0003e94;        
    }



    uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle); 
    uint32_t table = BT_DRIVER_GET_U32_REG_VAL(table_addr);
    uint32_t len = BT_DRIVER_GET_U32_REG_VAL(len_addr);
    uint16_t table_id = 0xFFFF;
    if(link_id == 0)
    {
        table_id = table &0xffff;
    }
    else if(link_id == 1)
    {
        table_id = table >>16;
    }
    
    if(table_id == 0x8 || table_id ==0xa)
    {
        TRACE("TX TABLE IS ONLY 1 SLOT PACKET");
        if(link_id ==0)
        {
            len &= 0xffff0000;
            len |= 0x2a7;
            table |= 0x110a;
            BT_DRIVER_PUT_U32_REG_VAL(len_addr, len);
            BT_DRIVER_PUT_U32_REG_VAL(table_addr, table);
        }
        else if(link_id ==1)
        {
            len &= 0xffff;
            len |= 0x2a70000;
            table |= 0x110a0000;
            BT_DRIVER_PUT_U32_REG_VAL(len_addr, len);
            BT_DRIVER_PUT_U32_REG_VAL(table_addr, table);       
        }
    }
}

void bt_drv_reg_op_force_task_dbg_idle(void)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
    	if(*(uint32_t *)0xc00060f8 == 0xC00060FC)
            BT_DRIVER_PUT_U32_REG_VAL(0xC00060FC, 1);
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        *(uint32_t *)0xc0000048 = 1;
    }
}

void bt_drv_reg_op_afh_follow_mobile_mobileidx_set(uint16_t hciHandle)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
    	if(*(uint32_t *)0xc00062c8 == 0xc00062d4)
    	{
            if (hciHandle == 0xff){
                BT_DRIVER_PUT_U32_REG_VAL(0xc00062d4, 0xff);
            }else{
                BT_DRIVER_PUT_U32_REG_VAL(0xc00062d4, btdrv_conhdl_to_linkid(hciHandle));
            }
    	}
    }
}

void bt_drv_reg_op_afh_follow_mobile_twsidx_set(uint16_t hciHandle)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
    	if(*(uint32_t *)0xc00062c4 == 0xc00062d0)
    	{

            if (hciHandle == 0xff){
                BT_DRIVER_PUT_U32_REG_VAL(0xc00062d0, 0xff);
            }else{
                BT_DRIVER_PUT_U32_REG_VAL(0xc00062d0, btdrv_conhdl_to_linkid(hciHandle));
            }
        }
    }
}

uint32_t bt_drv_reg_sco_active_bak;
void bt_drv_reg_op_sco_status_store(void)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
         bt_drv_reg_sco_active_bak =  *(uint32_t *)0xc000490c;
         *(uint32_t *)0xc000490c = 0;
         TRACE("SCO TRIGGER TARGET TIME = %x", bt_drv_reg_sco_active_bak);     
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
         bt_drv_reg_sco_active_bak =  *(uint32_t *)0xc00049a0;
         *(uint32_t *)0xc00049a0 = 0;
         TRACE("SCO TRIGGER TARGET TIME = %x", bt_drv_reg_sco_active_bak);         
    }
}

void bt_drv_reg_op_sco_status_restore(void)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        *(uint32_t *)0xc000490c = bt_drv_reg_sco_active_bak;       
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        *(uint32_t *)0xc00049a0 = bt_drv_reg_sco_active_bak;   
    }
}

uint8_t msbc_find_tx_sync(uint8_t *buff)
{
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
    return 0;
}

bool bt_drv_reg_op_sco_tx_buf_restore(uint8_t *trigger_test)
{
    uint8_t offset;
    bool nRet = false;
    
    offset = msbc_find_tx_sync((uint8_t *)0xd0212a50);
    if(offset !=0)
        {
        *trigger_test = (((BTDIGITAL_REG(0xd0224024) & 0x3f00)>>8) +(60-offset))%64;
        TRACE("TX BUF ERROR trigger_test=%d", *trigger_test);
        nRet = true;
    }
    return nRet;
}

enum ld_afh_state
{
    AFH_ON = 0,
    AFH_OFF,
    AFH_WAIT_ON_ACK,
    AFH_WAIT_ON_HSSI,
    AFH_WAIT_OFF_HSSI
};



struct afh_chnl_map
{
    ///5-byte channel map array
    uint8_t map[10];
};

struct ld_afh_bak
{
    enum ld_afh_state state;
    ///AFH Instant
    uint32_t        instant;
    ///AFH Mode
    uint8_t         mode;
    ///AFH channel map
    struct afh_chnl_map map;
};

void bt_drv_reg_op_afh_bak_reset(void)
{
    struct ld_afh_bak *afh_bak = (struct ld_afh_bak *)0xc0003fb8;

    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
    	afh_bak = (struct ld_afh_bak *)0xc0003fb8;
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        afh_bak = (struct ld_afh_bak *)0xc000404c;
    }
		
	
    for (uint32_t index = 0;index < 2;index++)
    {
        memset(afh_bak[index].map.map, 0xFF, sizeof(struct afh_chnl_map));
    }
}

struct ld_sniffer_connect_env
{
    uint8_t connect_addr[6];
    uint16_t bitoff;
    uint32_t clk_offset;
    uint8_t  chan_map[10];
    uint32_t afh_instant;
    uint8_t afh_mode;
    uint8_t enc_mode;
    uint8_t enc_key[16];
    uint8_t role;
    
};


struct lc_sco_air_params
{
    uint8_t  tr_id;
    uint8_t  esco_hdl;
    uint8_t  esco_lt_addr;
    uint8_t  flags;
    uint8_t  d_esco;
    uint8_t  t_esco;
    uint8_t  w_esco;
    uint8_t  m2s_pkt_type;
    uint8_t  s2m_pkt_type;
    uint16_t m2s_pkt_len;
    uint16_t s2m_pkt_len;
    uint8_t  air_mode;
    uint8_t  negt_st;
};

struct lmp_sco_start_pdu
{
    uint8_t  link_id;
    uint8_t  tr_id;
    uint8_t sco_type;
    uint8_t t_sco;
    uint8_t wsco;
    uint8_t dsco;
    uint8_t timeflag;
    uint8_t packet_type;
    uint16_t packet_length;
    uint8_t airmode;
    uint8_t ltaddr;
};

struct ld_sniffer_env
{
    uint8_t sniffer_active;
    uint8_t sniffer_role;
    uint8_t monitored_dev_addr[6];
    uint8_t sniffer_dev_addr[6];
    uint8_t monitored_dev_state;
    uint8_t sniffer_dev_state;

    uint8_t monitored_linkid;
    uint8_t monitored_syncid;
    uint8_t monitored_sco_linkid;
    uint8_t sniffer_linkid;
    struct ld_sniffer_connect_env  peer_dev;
    struct lc_sco_air_params  sco_para;
    struct lmp_sco_start_pdu  sco_para_bak;
    uint32_t sniffer_acl_link_timeout;
    uint16_t sync_acl_link_super_to;
    uint16_t sync_sco_link_super_to;
    uint32_t last_sco_link_clk;
    
    uint8_t sniffer_acl_link_pending;
    uint8_t sniffer_sco_link_pending;
    
};

void bt_drv_reg_op_afh_bak_save(uint8_t role, uint16_t mobileHciHandle)
{

    uint32_t afh_bak_addr=0;
    uint32_t ld_sniffer_env_addr=0;
    uint32_t ld_afh_addr=0;
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        afh_bak_addr = 0xc0003fb8;
        ld_sniffer_env_addr = 0xc0004bf0;
	 ld_afh_addr = 0xc0004067;
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        afh_bak_addr = 0xc000404c;
        ld_sniffer_env_addr = 0xc0004c84;
	 ld_afh_addr = 0xc00040fb;    
    }
    
    struct ld_afh_bak *afh_bak = (struct ld_afh_bak *)afh_bak_addr;
    struct  ld_afh_bak *afh_bak_id = &afh_bak[btdrv_conhdl_to_linkid(mobileHciHandle)];
    struct ld_sniffer_env *ld_sniffer = (struct ld_sniffer_env *)ld_sniffer_env_addr;


    if(role == 1) //SLAVE ROLE
    {
        ld_sniffer->peer_dev.afh_instant = afh_bak_id->instant;
        ld_sniffer->peer_dev.afh_mode = afh_bak_id->mode;
        if( ld_sniffer->peer_dev.afh_mode == 0)
        {
            uint8_t default_map[]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f};
            memcpy(ld_sniffer->peer_dev.chan_map,default_map,10);
        }
        else
        {
            memcpy(ld_sniffer->peer_dev.chan_map,afh_bak_id->map.map,10);
        }
    }
    else 
    {
         ld_sniffer->peer_dev.afh_instant = 0;
         ld_sniffer->peer_dev.afh_mode = 1;
         memcpy(ld_sniffer->peer_dev.chan_map,(uint8_t *)ld_afh_addr,10);
    }
    TRACE("CHANMAP=:");
    DUMP8("%02x",ld_sniffer->peer_dev.chan_map,10);
    TRACE("\n instant=%x,mode=%x",ld_sniffer->peer_dev.afh_instant,ld_sniffer->peer_dev.afh_mode);           
}


void bt_drv_reg_op_afh_set_default(void)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        TRACE("2_0xc0004c20 = %x", *(unsigned int *)0xc0004c20);
        if (0 == (*(unsigned int *)0xc0004c20 & 0x1))
        {
            *(unsigned int *)0xc0004c1c = 0;
            *(unsigned int *)0xc0004c20 |= 1;
            *(unsigned int *)0xc0004c10 = 0xffffffff;
            *(unsigned int *)0xc0004c14 = 0xffffffff;
            *(unsigned int *)0xc0004c18 = 0x00007fff; 
        }
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        if (0 == (*(unsigned int *)0xc0004cb4 & 0x1))
        {
            *(unsigned int *)0xc0004cb0 = 0;
            *(unsigned int *)0xc0004cb4 |= 1;
            *(unsigned int *)0xc0004ca4 = 0xffffffff;
            *(unsigned int *)0xc0004ca8 = 0xffffffff;
            *(unsigned int *)0xc0004cac = 0x00007fff; 
        }        
    }
}

void bt_drv_reg_op_connection_checker(void)
{
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        TRACE( "MAX SLOT=%x", BT_DRIVER_GET_U32_REG_VAL(0xc0003df8));
        TRACE( "MAX LEN=%x", BT_DRIVER_GET_U32_REG_VAL(0xc0003e04));
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        TRACE( "MAX SLOT=%x", BT_DRIVER_GET_U32_REG_VAL(0xc0003e88));
        TRACE( "MAX LEN=%x", BT_DRIVER_GET_U32_REG_VAL(0xc0003e94));
    }
    TRACE( "hdr=%x", BT_DRIVER_GET_U32_REG_VAL(0xd02101b4));
    TRACE( "hdr1=%x", BT_DRIVER_GET_U32_REG_VAL(0xd02101b4+96));
}

extern "C" uint32_t hci_current_left_tx_packets_left(void);
extern "C" uint32_t hci_current_left_rx_packets_left(void);


void bt_drv_reg_op_set_swagc_mode(uint8_t mode)
{
    return;
}

void bt_drv_reg_op_bt_packet_checker(void)
{
    return;
}

void bt_drv_reg_op_bt_info_checker(void)
{
    uint32_t *buf_ptr;
    uint8_t buf_count=0;

    bt_drv_reg_op_connection_checker();

    uint32_t tx_flow_addr=0;
    uint32_t remote_tx_flow_addr=0;
    uint32_t tx_buf_addr=0;
    uint32_t rx_buff_addr=0;
    uint32_t air_buff_addr=0;

    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        tx_flow_addr = 0xc0003f9c;
        remote_tx_flow_addr = 0xc0003fa2;
	 tx_buf_addr = 0xc0005d48;
	 rx_buff_addr = 0xc0003af0;
	 air_buff_addr = 0xc0003af4;
	 
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        tx_flow_addr = 0xc0004030;
        remote_tx_flow_addr = 0xc0004036;
	 tx_buf_addr = 0xc0005DEC;
	 rx_buff_addr = 0xc0003b80;
	 air_buff_addr = 0xc0003b84; 
    }


    TRACE( "tx flow=%x",*(uint32_t *)tx_flow_addr);
    TRACE( "remote_tx flow=%x",*(uint32_t *)remote_tx_flow_addr);
    
    
    TRACE( "tx buff = %x %x",*(uint16_t *)tx_buf_addr,hci_current_left_tx_packets_left());
    buf_ptr = (uint32_t *)rx_buff_addr;
    buf_count = 0;
    while(*buf_ptr)
    {
        buf_count++;
        buf_ptr = (uint32_t *)(*buf_ptr);
    }
    
    TRACE( "rxbuff = %x %x",buf_count,hci_current_left_rx_packets_left());
    
    
    buf_ptr = (uint32_t *)air_buff_addr;
    buf_count = 0;
    while(*buf_ptr)
    {
        buf_count++;
        buf_ptr = (uint32_t *)(*buf_ptr);
    }
    TRACE( "air rxbuff = %x",buf_count);

}

uint32_t *rxbuff;
uint16_t *send_count;
uint16_t *free_count;
uint16_t *bt_send_count;

void bt_drv_reg_op_ble_buffer_cleanup(void)
{
    uint32_t lock;
#if 1
    uint8_t i;
    //if(param->reason == 8)
    {
        for(i=0;i<6;i++)
        {
             rxbuff = (uint32_t *)(0xd0211b64+270*i);
             TRACE("rxbuff=%x,rxd=%x",rxbuff,*rxbuff);
             if(*rxbuff & 0x8000)
             {
                *rxbuff &= (~0x8000);
                 TRACE("rxbuff=%x,rxd=%x",rxbuff,*rxbuff);
                
             }
        }
    }

    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        send_count = (uint16_t *)0xc0005d52;
        free_count = (uint16_t *)0xc0005d48;
        bt_send_count = (uint16_t *)0xC0005d4a;
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_5)
    {
        send_count = (uint16_t *)0xc0005df6;
        free_count = (uint16_t *)0xc0005dec;
        bt_send_count = (uint16_t *)0xc0005dee;        
    }
#if 0
    send_count = (uint16_t *)0xc00059ba;
    free_count = (uint16_t *)0xc00059b0;
#endif
    TRACE("send_count = %x %x %x %x %x",*send_count,*(send_count+1),*(send_count+2),*(send_count+3),*(send_count+4));
//    TRACE("bt_send_count = %x %x %x %x",*bt_send_count,*(bt_send_count+1),*(bt_send_count+2),*(bt_send_count+3),*(bt_send_count+4));
     lock = int_lock();
    *free_count += *send_count;
    *free_count += *(send_count+1);
    *free_count += *(send_count+2);
    *free_count += *(send_count+3);
    *free_count += *(send_count+4);

    *send_count = 0;
    *(send_count+1) = 0;
    *(send_count+2) = 0;
    *(send_count+3) = 0;
    *(send_count+4) = 0;
    int_unlock(lock);
    if(*free_count > 6)
    {
        TRACE("ERROR!! FREE COUNT");
    }
    
    TRACE("free count= %x",*free_count);
#endif
}

//#define BT_CONTROLLER_CRASH_DUMP_ADDR_BASE  (0xc00064cc)

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
#if 0
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_5)
    {
        TRACE_HIGHPRIO("BT controller BusFault_Handler:\nREG:[LR] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE));
        TRACE_HIGHPRIO("REG:[R4] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +4));
        TRACE_HIGHPRIO("REG:[R5] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +8));
        TRACE_HIGHPRIO("REG:[R6] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +12));
        TRACE_HIGHPRIO("REG:[R7] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +16));
        TRACE_HIGHPRIO("REG:[R8] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +20));
        TRACE_HIGHPRIO("REG:[R9] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +24));
        TRACE_HIGHPRIO("REG:[sl] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +28));
        TRACE_HIGHPRIO("REG:[fp] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +32));
        TRACE_HIGHPRIO("REG:[ip] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +36));
        TRACE_HIGHPRIO("REG:[SP,#0] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +40));
        TRACE_HIGHPRIO("REG:[SP,#4] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +44));
        TRACE_HIGHPRIO("REG:[SP,#8] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +48));
        TRACE_HIGHPRIO("REG:[SP,#12] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +52));
        TRACE_HIGHPRIO("REG:[SP,#16] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +56));
        TRACE_HIGHPRIO("REG:[SP,#20] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +60));
        TRACE_HIGHPRIO("REG:[SP,#24] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +64));
        TRACE_HIGHPRIO("REG:[SP,#28] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +68));
        TRACE_HIGHPRIO("REG:[SP,#32] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +72));
        TRACE_HIGHPRIO("REG:[SP,#36] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +76));
        TRACE_HIGHPRIO("REG:[SP,#40] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +80));
        TRACE_HIGHPRIO("REG:[SP,#44] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +84));
        TRACE_HIGHPRIO("REG:[SP,#48] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +88));
        TRACE_HIGHPRIO("REG:[SP,#52] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +92));
        TRACE_HIGHPRIO("REG:[SP,#56] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +96));
        TRACE_HIGHPRIO("REG:[SP,#60] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +100));
        TRACE_HIGHPRIO("REG:[SP,#64] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +104));
        TRACE_HIGHPRIO("REG:[cfsr] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +108));
        TRACE_HIGHPRIO("REG:[bfar] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +112));
        TRACE_HIGHPRIO("REG:[hfsr] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +116));

        struct co_list_con *queue_sent = (struct co_list_con*)(0xc0003844);
        struct co_list_con *queue_saved = (struct co_list_con*)(0xc0003844 + 1*sizeof(struct co_list_con));
        struct co_list_con *queue_timer = (struct co_list_con*)(0xc0003844 + 2*sizeof(struct co_list_con));
        
        //TRACE_HIGHPRIO("Plantform error =0x%x,defualt :0xabcd0000",*(volatile uint32_t *)(0xc000623c));

        //queue_sent
        TRACE_HIGHPRIO("queue_sent _addr=0x%x,fist=0x%x,cnt=%x",(0xc0003844 ),queue_sent->first,queue_sent->cnt);    
        if(queue_sent->first != NULL)
        {
            while(queue_sent->first != NULL)
            {
                struct ke_timer *timer = (struct ke_timer*) queue_sent->first;
                TRACE_HIGHPRIO("queue_sent = 0x%08x,Next_ptr:0x%08x, id:0x%04x time:0x%08x",timer, timer->next,timer->id,timer->task,timer->time);
                queue_sent->first = queue_sent->first->next;
            }
        }
        //queue_saved
        TRACE_HIGHPRIO("queue_saved _addr=0x%x,fist=0x%x,cnt=%x",(0xc0003844 + 1*sizeof(struct co_list_con)),queue_saved->first,queue_saved->cnt);
        if(queue_saved->first != NULL)
        {
            while(queue_saved->first != NULL)
            {
                struct ke_timer *timer = (struct ke_timer*) queue_saved->first;
                TRACE_HIGHPRIO("queue timer_ptr= 0x%08x,Next_ptr:0x%08x, id:0x%04x time:0x%08x",timer, timer->next,timer->id,timer->task,timer->time);
                queue_saved->first = queue_saved->first->next;
            }
        }
        //queue_timer
        TRACE_HIGHPRIO("queue_timer _addr=0x%x,fist=0x%x,cnt=%x",(0xc0003844 + 2*sizeof(struct co_list_con)),queue_timer->first,queue_timer->cnt);
        if(queue_timer->first != NULL)
        {
            while(queue_timer->first != NULL)
            {
                struct ke_timer *timer = (struct ke_timer*) queue_timer->first;
                struct ke_timer *last_timer = (struct ke_timer*) queue_timer->last;

                TRACE_HIGHPRIO("timer first_ptr= 0x%08x,val:0x%x,0x%x ,Next:0x%08x, id:0x%04x time:0x%08x",timer, 
                   *(uint32_t*)((uint8_t*)timer - 4),*(uint32_t*)((uint8_t*)timer - 8),
                    timer->next,timer->id,timer->task,timer->time);
                TRACE_HIGHPRIO("timer last_ptr= 0x%08x,val:0x%x,0x%x ,Next:0x%08x, id:0x%04x time:0x%08x",last_timer,
                   *(uint32_t*)((uint8_t*)last_timer - 4),*(uint32_t*)((uint8_t*)last_timer - 8),
                    last_timer->next,last_timer->id,last_timer->task,last_timer->time);
                   
                queue_timer->first = queue_timer->first->next;
            }
        }
        
        struct mblock_free * heap0 = ( struct mblock_free *)0xc00014cc;
        struct mblock_free * heap1 = ( struct mblock_free *)0xc0001f00;
        struct mblock_free * heap2 = ( struct mblock_free *)0xc0003438;
           
        uint16_t * heap_size = (uint16_t*)(0xc0003844 + 3*sizeof(struct co_list_con) + 3*sizeof(struct mblock_free*));
        uint16_t * heap_used = (uint16_t*)(0xc0003844 + 3*sizeof(struct co_list_con) + 3*sizeof(struct mblock_free*) + 3*sizeof(uint16_t));
        uint32_t * max_heap_used = (uint32_t*)(0xc0003844 + 3*sizeof(struct co_list_con) + 6*sizeof(struct mblock_free*));
        TRACE_HIGHPRIO("heap0 next =0x%08x, free size=%x,corrupt_check=%x",heap0->next,heap0->free_size,heap0->corrupt_check);
        TRACE_HIGHPRIO("heap1 next =0x%08x, free size=%x,corrupt_check=%x",heap1->next,heap1->free_size,heap1->corrupt_check);
        TRACE_HIGHPRIO("heap2 next =0x%08x, free size=%x,corrupt_check=%x.\nmax_heap_used=0x%x ",heap2->next,heap2->free_size,heap2->corrupt_check,*max_heap_used);
        
        if(heap1 != NULL)
        {
            while(heap1->next!= NULL){
                    struct mblock_free *node = (struct mblock_free*) heap1->next;
                    TRACE_HIGHPRIO("heap1->next_ptr= 0x%08x, free_size:0x%04x corrupt_check:0x%04x", node->next,node->free_size,node->corrupt_check);
                    heap1->next = heap1->next->next;
           }
            
            while(heap1->previous!= NULL){
                    struct mblock_free *node = (struct mblock_free*) heap1->previous;
                    TRACE_HIGHPRIO("heap1->previous_ptr= 0x%08x, free_size:0x%04x corrupt_check:0x%04x", node->next,node->free_size,node->corrupt_check);
                    heap1->previous = heap1->previous->previous;
            }
        }

        for(int i=0;i<3;i++)
        {
            TRACE_HIGHPRIO("heap[%d] size=0x%x, used=0x%x",i,heap_size[i],heap_used[i]);
        }
    }
#endif	
}

void bt_drv_reg_op_set_tx_pwr(uint16_t connHandle, uint8_t txpwr)
{
}

uint8_t bt_drv_reg_op_get_tx_pwr(uint16_t connHandle)
{
    return 0;
}

void bt_drv_reg_op_fix_tx_pwr(uint16_t connHandle)
{
}

void bt_drv_reg_op_lsto_hack(uint8_t link_id)
{
}

void bt_drv_reg_op_enable_emsack_mode(uint16_t connHandle, uint8_t master, uint8_t enable)
{
}


void bt_drv_reg_op_force_retrans(bool enalbe)
{
}

void bt_drv_reg_op_set_accessible_mode(uint8_t mode)
{
}

int8_t bt_drv_read_rssi_in_dbm(uint16_t connHandle)
{
    return 0;
}

void bt_drv_reg_op_call_monitor(uint16_t connHandle, uint8_t tws_role)
{
}

void bt_drv_reg_op_lock_sniffer_sco_resync(void)
{
}

void bt_drv_reg_op_unlock_sniffer_sco_resync(void)
{
}

uint8_t bt_drv_reg_op_retx_att_nb_manager(uint8_t mode)
{
    return 0;
}
void bt_drv_reg_op_modify_bitoff_timer(uint16_t time_out)
{
}

void bt_drv_reg_op_ble_llm_substate_hacker(void)
{

}

#ifdef __LARGE_SYNC_WIN__
void bt_drv_reg_op_enlarge_sync_winsize(uint16_t conhdl, bool on)
{
}
#endif

bool  bt_drv_reg_op_check_esco_acl_sniff_conflict(uint16_t hciHandle)
{
    return false;
}

void bt_drv_reg_op_esco_acl_sniff_delay_cal(uint16_t hciHandle,bool enable)
{
}

void bt_drv_reg_op_set_reboot_pairing_mode(uint8_t mode)
{
}

void bt_drv_reg_set_rssi_seed(uint32_t seed)
{
    if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_2 && hal_get_chip_metal_id()<HAL_CHIP_METAL_ID_5)
    {
        *(volatile uint32_t*)0xc0005f20 = seed;
    }
    else if(hal_get_chip_metal_id()>=HAL_CHIP_METAL_ID_5)
    {
        *(volatile uint32_t*)0xc0005fc8 = seed;///should be check when chip come back
    }
}

int8_t  bt_drv_rssi_correction(int8_t rssi)
{
    return rssi;
}


void bt_dev_music_link_config(uint16_t active_link,uint8_t active_role,uint16_t inactive_link,uint8_t inactive_role)
{
//muti_slave_schedure_en  0xc00002d5
//music_playing_link  0xc00002d2
//next_slave_cnt_number  0xc00002d3

//no_music_link  0xc00005dc
//current_slave_cnt_number  0xc00002d0

    TRACE("bt_dev_music_link_config %x %d %x %d",active_link,active_role,inactive_link,inactive_role);
    if(active_role == 1 && inactive_role== 1)
    {
    *(volatile uint8_t *)0xc00002d2 = active_link-0x80;
    *(volatile uint8_t *)0xc00005dc = inactive_link-0x80;
    *(volatile uint8_t *)0xc00002d0 = 0x0a;//set inactive cnt number
    *(volatile uint8_t *)0xc00002d3 = 0x23;//set active cnt number
    }
}

void bt_drv_pcm_set(uint8_t en)
{
    if(en)
        *(volatile unsigned int *)(0xd02201b8) |= 1<<14;
    else
        *(volatile unsigned int *)(0xd02201b8) &= 0xffffbfff;
}

void bt_drv_clear_skip_flag()
{
    (*(volatile uint32_t *)(0xc0000608)) &= 0x00ffffff;
    (*(volatile uint32_t *)(0xc00005f8)) &= 0xffff00ff;
}

void bt_drv_sco_txfifo_reset(uint16_t codec_id)
{
}