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
#include "hal_chipid.h"
#include "hal_intersys.h"
#include "bt_drv.h"
#include "bt_drv_interface.h"
#include "bt_drv_1400_internal.h"
#include "bt_drv_internal.h"

///enable m4 patch func
#define BTDRV_PATCH_EN_REG                  0xe0002000

//set m4 patch remap adress
#define BTDRV_PATCH_REMAP_REG               0xe0002004

//instruction patch compare src address
#define BTDRV_PATCH_INS_COMP_ADDR_START     0xe0002008

#define BTDRV_PATCH_INS_REMAP_ADDR_START    0xc0000100

//data patch compare src address
#define BTDRV_PATCH_DATA_COMP_ADDR_START    0xe00020e8

#define BTDRV_PATCH_DATA_REMAP_ADDR_START   0xc00001e0


#define BTDRV_PATCH_ACT     0x1
#define BTDRV_PATCH_INACT   0x0

typedef struct
{
    uint8_t patch_index;            //patch position
    uint8_t patch_state;            //is patch active
    uint16_t patch_length;          //patch length 0:one instrution replace  other:jump to ram to run more instruction
    uint32_t patch_remap_address;   //patch occured address
    uint32_t patch_remap_value;     //patch replaced instuction
    uint32_t patch_start_address;   //ram patch address for lenth>0
    uint8_t *patch_data;            //ram patch date for length >0

} BTDRV_PATCH_STRUCT;

/*****************************************************************************
CHIP ID                : 1400
Metal id               : 0
BT controller tag      : t1
*****************************************************************************/
const uint32_t bes1400_patch0_ins_data[] =
{
    0x601a4b01,
    0xbbe2f600,
    0x4000007C
}; // intersys mcu2bt interrupt clear

const BTDRV_PATCH_STRUCT bes1400_ins_patch0 =
{
    0,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch0_ins_data),
    0x00006ec8,
    0xbc1af1ff,
    0xc0006700,
    (uint8_t *)bes1400_patch0_ins_data
};

const uint32_t bes1400_patch1_ins_data[] =
{
    0x601a4b01,
    0xbbf8f600,
    0x4000007C
}; // intersys mcu2bt interrupt clear

const BTDRV_PATCH_STRUCT bes1400_ins_patch1 =
{
    1,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch1_ins_data),
    0x00006f04,
    0xbc04f1ff,
    0xc0006710,
    (uint8_t *)bes1400_patch1_ins_data
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch2 =
{
    2,
    BTDRV_PATCH_ACT,
    0,
    0x00023f28,
    0xbf00bd10,
    0,
    NULL
}; // remove assert

const uint32_t bes1400_patch3_ins_data[] =
{
    /*6720*/ 0x4a18b2e8,
    /*6724*/ 0x21cbf892,
    /*6728*/ 0x4b150112,
    /*672c*/ 0xbf005a9b,
    /*6730*/ 0x0309f3c3,
    /*6734*/ 0x2280ea4f,
    /*6738*/ 0x0003ea42,
    /*673c*/ 0xf826f000,
    /*6740*/ 0x0c00ea4f,
    /*6744*/ 0xb040f8df,
    /*6748*/ 0xb000f8cd,
    /*674c*/ 0x22019903,
    /*6750*/ 0xbf002300,
    /*6754*/ 0xf804f5fd,
    /*6758*/ 0xc000f8cd,
    /*675c*/ 0x0307f018,
    /*6760*/ 0xd00b9306,
    /*6764*/ 0x30acf9b6,
    /*6768*/ 0x44189800,
    /*676c*/ 0x00acf8a6,
    /*6770*/ 0x30aef896,
    /*6774*/ 0xbf003301,
    /*6778*/ 0x30aef886,
    /*677c*/ 0xbde6f625,
    /*6780*/ 0xd02115b4,
    /*6784*/ 0xc0005c28,
    /*6788*/ 0xc0004108,
    /*678c*/ 0x2385f3c0,
    /*6790*/ 0x490c3b48,
    /*6794*/ 0x0207f000,
    /*6798*/ 0x1a9b5c8a,
    /*679c*/ 0x02c3f3c0,
    /*67a0*/ 0x5c8a490b,
    /*67a4*/ 0x4a0f1a9b,
    /*67a8*/ 0x10c2f3c0,
    /*67ac*/ 0x1a1b5c10,
    /*67b0*/ 0xb25ab2db,
    /*67b4*/ 0xbfb42a07,
    /*67b8*/ 0x202f202e,
    /*67bc*/ 0xbf001a18,
    /*67c0*/ 0x4770b240,
    /*67c4*/ 0xc00067c8, // RF_RX_HWAGC_FLT_TBL
    /*67c8*/ 0xf4f1eeeb,
    /*67cc*/ 0x00fdfaf7,
    /*67d0*/ 0xc00067d4, // RF_RX_HWAGC_I2V_TBL
    /*67d4*/ 0xeeebe8e5,
    /*67d8*/ 0xfaf7f4f1,
    /*67dc*/ 0x7f7f00fd,
    /*67e0*/ 0x7f7f7f7f,
    /*67e4*/ 0xc00067e8, // RF_RX_HWAGC_LNA_TBL
    /*67e8*/ 0xfaf3ede7,
    /*67ec*/ 0x7f7f7f00
}; // acl rx rssi

const BTDRV_PATCH_STRUCT bes1400_ins_patch3 =
{
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch3_ins_data),
    0x0002c300,
    0xba0ef1da,
    0xc0006720,
    (uint8_t *)bes1400_patch3_ins_data
};

const uint32_t bes1400_patch4_ins_data[] =
{
    /*67f0*/ 0x30acf9b6,
    /*67f4*/ 0x20aef896,
    /*67f8*/ 0xf2f2fb93,
    /*67fc*/ 0x20aff886,
    /*6800*/ 0xbf002300,
    /*6804*/ 0x30aef886,
    /*6808*/ 0xb81af626
}; // acl report avg rssi

const BTDRV_PATCH_STRUCT bes1400_ins_patch4 =
{
    4,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch4_ins_data),
    0x0002c83c,
    0xbfd8f1d9,
    0xc00067f0,
    (uint8_t *)bes1400_patch4_ins_data
};

const uint32_t bes1400_patch5_ins_data[] =
{
    /*6810*/ 0xbf00b404,
    /*6814*/ 0x2083ea4f,
    /*6818*/ 0x4b0c7929,
    /*681c*/ 0x01c1ebc1,
    /*6820*/ 0x1011f833,
    /*6824*/ 0x01c9f3c1,
    /*6828*/ 0x0001ea40,
    /*682c*/ 0xffaef7ff,
    /*6830*/ 0x2814bc04,
    /*6834*/ 0x7328bfd2,
    /*6838*/ 0x732b237f,
    /*683c*/ 0x4b047b28,
    /*6840*/ 0x54587929,
    /*6844*/ 0x23004628,
    /*6848*/ 0xbcd6f639,
    /*684c*/ 0xd02104d6,
    /*6850*/ 0xc0006854,
    /*6854*/ 0x7f7f7f7f,
    /*6858*/ 0x7f7f7f7f
}; // ble rx rssi

const BTDRV_PATCH_STRUCT bes1400_ins_patch5 =
{
    5,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch5_ins_data),
    0x000401f4,
    0xbb0cf1c6,
    0xc0006810,
    (uint8_t *)bes1400_patch5_ins_data
};

const uint32_t bes1400_patch6_ins_data[] =
{
    /*6860*/ 0x4b040870,
    /*6864*/ 0x3000fba3,
    /*6868*/ 0x4b030880,
    /*686c*/ 0xbf005c18,
    /*6870*/ 0xbf9cf630,
    /*6874*/ 0x92492493,
    /*6878*/ 0xc0006854
}; // adv rssi report

const BTDRV_PATCH_STRUCT bes1400_ins_patch6 =
{
    6,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch6_ins_data),
    0x000377a4,
    0xb85cf1cf,
    0xc0006860,
    (uint8_t *)bes1400_patch6_ins_data
};

const uint32_t bes1400_patch7_ins_data[] =
{
    /*687c*/ 0x4b040870,
    /*6880*/ 0x3000fba3,
    /*6884*/ 0x4b030880,
    /*6888*/ 0xbf005c18,
    /*688c*/ 0xbffaf630,
    /*6890*/ 0x92492493,
    /*6894*/ 0xc0006854
}; // adv rssi report

const BTDRV_PATCH_STRUCT bes1400_ins_patch7 =
{
    7,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch7_ins_data),
    0x0003787c,
    0xbffef1ce,
    0xc000687c,
    (uint8_t *)bes1400_patch7_ins_data
};

#if 1

const uint32_t bes1400_patch8_ins_data[] =
{
    0xf5fa4628,
    0x4903fd79,
    0x46286008,
    0xbf002101,
    0xbf9cf613,
    0xc00068b8,
    0x00000000

};

const BTDRV_PATCH_STRUCT bes1400_ins_patch8=
{
    8,
    BTDRV_PATCH_INACT,
    sizeof(bes1400_patch8_ins_data),
    0x0001a7e8,
    0xb85af1ec,
    0xc00068a0,
    (uint8_t *)bes1400_patch8_ins_data
};



const uint32_t bes1400_patch9_ins_data[] =
{
    0x0f00f1b8,
    0x4628d004,
    0x68094904,
    0xfcf8f5fa,
    0x21134630,
    0x3044f894,
    0xbfd5f613,
    0xc00068b8


};

const BTDRV_PATCH_STRUCT bes1400_ins_patch9 =
{
    9,
    BTDRV_PATCH_INACT,
    sizeof(bes1400_patch9_ins_data),
    0x0001a880,
    0xb81ef1ec,
    0xc00068c0,
    (uint8_t *)bes1400_patch9_ins_data
};


#endif

const uint32_t bes1400_patch10_ins_data[] =
{
    /*68e0*/ 0x2000b081, //"sub sp, #4\n\t"                 // [b081]
    //"movs r0, #0\n\t"                // [2000]
    /*68e4*/ 0x88ca9000, //"str  r0, [sp]\n\t"              // [9000]
    //"ldrh r2, [r1, #6]\n\r"          // [88ca] r1 is desc_to_be_freed, load desc_to_be_freed->buffer_idx to r2
    /*68e8*/ 0x4f00f412, //"tst.w r2, #32768\n\r"           // [f412 4f00] test buffer_idx & 0x8000
    /*68ec*/ 0x2001d010, //"beq.n 0xa02069e0\n\r"           // [d010] if is zero then jump
    //"movs r0, #1\n\t"                // [2001]
    /*68f0*/ 0xbf009000, //"str  r0, [sp]\n\t"              // [9000]
    /*68f4*/ 0x020ef3c2, //"ubfx r2, r2, #0, #15\n\r"       // [f3c2 020e] r2 = buffer_idx & 0x7fff
    /*68f8*/ 0x323b4807, //"ldr r0, [pc, #28]\n\r"          // [4807] load address of em_buf_env (c00063f8) to r0
    //"adds r2, #59\n\r"               // [323b] (idx + 59) << 3 = 8 * idx + 59 * 8 = 8 * idx + 472
    /*68fc*/ 0x0c01ea5f, //"movs ip, r1\n\r"                // [ea5f 0c01] backup r1 to ip
    /*6900*/ 0x01c2eb00, //"add.w r1, r0, r2, lsl #3\n\r"   // [eb00 01c2] r1 is address of em_buf_env.tx_buff_node[idx]
    /*6904*/ 0xbf003014, //"adds r0, #20\n\r"               // [3014] r0 is address of em_buf_env.tx_buff_free
    /*6908*/ 0xfc58f5fc, //"bl 0xa00031bc\n\r"              // call co_list_push_back a0206908 -> a00031bc
    /*690c*/ 0x010cea5f, //"movs r1, ip\n\r"                // [ea5f 010c] restore r1
    /*6910*/ 0x23008888, //"ldrh r0, [r1, #4]\n\r"          // [8888]
    //"movs r3, #0\n\r"                // [2300]
    /*6914*/ 0xb8a6f63c, // jump a0206914 -> a0042a64
    /*6918*/ 0xc00063f8, // address of em_buf_env
};


const BTDRV_PATCH_STRUCT bes1400_ins_patch10 =
{
    10,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch10_ins_data),
    0x00042a60,
    0xbf3ef1c3, // jump a0042a60 -> a02068e0
    0xc00068e0,
    (uint8_t *)bes1400_patch10_ins_data
};


const uint32_t bes1400_patch11_ins_data[] =
{
    /*691c*/ 0xb662b904,
    /*6920*/ 0xb0019900, // ldr r1, [sp]; add sp, #4
    /*6924*/ 0xb8aaf63c, // jump a0206924 -> a0042a7c
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch11 =
{
    11,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch11_ins_data),
    0x00042a78,
    0xbf50f1c3, // jump a0042a78 -> a020691c
    0xc000691c,
    (uint8_t *)bes1400_patch11_ins_data
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch12 =
{
    12,
    BTDRV_PATCH_ACT,
    0,
    0x00042a7c,
    0xbd100008,  // movs r0, r1; pop {r4, pc}
    0,
    NULL
};

#if 0
const uint32_t bes1400_patch13_ins_data[] =
{
    /*6b10*/ 0x3078f886,
    /*6b14*/ 0xbf0088e3,
    /*6b18*/ 0x4343ea6f,
    /*6b1c*/ 0x4353ea6f,
    /*6b20*/ 0xbf0080e3,
    /*6b24*/ 0xb85cf639, // jump a0206b24 -> a003fbe0
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch13 =
{
    13,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch13_ins_data),
    0x0003fbdc,
    0xbf98f1c6, // jump a003fbdc -> a0206b10
    0xc0006b10,
    (uint8_t *)bes1400_patch13_ins_data
};
#endif

const uint32_t bes1400_patch13_ins_data[] =
{
    /*6928*/ 0x3078f886,
    /*692c*/ 0xbf0088e3,
    /*6930*/ 0x4343ea6f,
    /*6934*/ 0x4353ea6f,
    /*6938*/ 0xbf0080e3,
    /*693c*/ 0xb950f639, // jump a020693c -> a003fbe0
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch13 =
{
    13,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch13_ins_data),
    0x0003fbdc,
    0xbea4f1c6, // jump a003fbdc -> a0206928
    0xc0006928,
    (uint8_t *)bes1400_patch13_ins_data
};

/* 1400 t1 (CHIP_ID: 0) patches */
static const uint32_t best1400_ins_patch_config[] =
{
    14,
    (uint32_t)&bes1400_ins_patch0,
    (uint32_t)&bes1400_ins_patch1,
    (uint32_t)&bes1400_ins_patch2,
    (uint32_t)&bes1400_ins_patch3,
    (uint32_t)&bes1400_ins_patch4,
    (uint32_t)&bes1400_ins_patch5,
    (uint32_t)&bes1400_ins_patch6,
    (uint32_t)&bes1400_ins_patch7,
    (uint32_t)&bes1400_ins_patch8,
    (uint32_t)&bes1400_ins_patch9,

    (uint32_t)&bes1400_ins_patch10,
    (uint32_t)&bes1400_ins_patch11,
    (uint32_t)&bes1400_ins_patch12,
    (uint32_t)&bes1400_ins_patch13,
};


/***************************************************************************
 *
 * instruction patch Information
 *
 * BT ROM Chip Version:1400 T4
 * CHIP ID = 1
 * patch  0: in use,  can be closed
 * patch  1: in use,  when EM_BT_RXSEQERR_BIT, no rx traffic++
 * patch  2: in use,  remove the memset of ldenv

 * patch  2a: 0xC0006810--->0xC0006874 (patch 19)
 * patch  2b: 0xC0006880--->0xC000688c (patch 21)
 * patch  2c: 0xC0006894--->0xC00068ac  (patch 22)
 * patch  2d: 0xC00068b0--->0xC00068c0  (patch 24)

 * patch  3:  0xC0006900--->0xC0006940
 * patch  4:  0xC0006948--->0xC0006978
 * patch  5:  0xC0006980--->0xC0006990
 * patch  5a:  0xC0006994--->0xC00069ac (patch 13)
 * patch  6:  0xC00069c0--->0xC00069f0
 * patch  7:  0xC00069f4--->0xC0006a88
 * patch  8:  0xC0006aa0--->0xC0006abc
 * patch  9:  0xC0006ac0--->0xC0006ad4
 * patch 10: 0xC0006ae0--->0xC0006b2c
 * patch 11: 0xC0006b40--->0xC0006b50
 * patch 12: 0xC0006b54--->0xC0006B64
 * patch 13: in use:ld_sco_dynamic_switch
 * patch 14: 0xC0006B68--->0xC0006BAC
 * patch 15: 0xC0006BB8--->0xC0006BF4
 * patch 16: 0xC0006BFC--->0xC0006C18 
 * patch 18: in used,ld_sco_end2
 * patch 19: in used,__SW_SEQ_FILTER__
 * patch 20: in used,ibrt switch op =2
 * patch 21: in used
 * patch 22: in used,ld_report_master_snoop_switch_result_ack_callback
 * patch 23: in used,ibrt open afh update
 * patch 24: in used,__SW_SEQ_FILTER__
 * patch 25: 0xC0006C3C--->0xC0006c78
 * patch 26: in used,//acl end assert HACK
 * patch 27: 0xC0006C80--->0xC0006C90
 * patch 28: 0xC0006C98--->0xC0006CA4
 * patch 17: 0xC0006CA8--->0xC0006CD0
 * patch 29: in used:  ld_acl_start t_poll when ibrt
 * patch 30:  0xC0006CD4--->0xC0006D1C
 * patch 31:  0xC0006C24--->0xC0006D34
 * patch 32:  0xC0006C3C--->0xC0006D54
 * patch 33: in used: ld_sniffer_update_status: SNIFFER_EVT_SCO_CONNECTED
 ****************************************************************************/

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_0 =
{
    0,
    BTDRV_PATCH_ACT,
    0,
    0x0002bac4,
    0xbf00bf00,
    0,
    NULL
}; //ld_acl_sched_in_snoop_unsniff_mode :monitor_idx error

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_1 =
{
    1,
    BTDRV_PATCH_ACT,
    0,
    0x0002f2bc,
    0xbf0030bb,
    0,
    NULL
}; // when EM_BT_RXSEQERR_BIT, no rx traffic++

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_2 =
{
    2,
    BTDRV_PATCH_ACT,
    0,
    0x00025ef4,
    0xbf00bf00,
    0,
    NULL
};// remove the memset of ldenv

const uint32_t bes1400_t4_patch2a_ins_data[] =
{
    0x681b4b0e, /*6810*/
    0x4b0eb1ab,
    0x31cbf893,
    0x4a0d011b,
    0xf3c35a9b,
    0x4a0c2340,
    0x2a025c12,
    0x4a0ad103,
    0x20005413,
    0x42934770,
    0x4a07d005,
    0x20005413,
    0x20004770,
    0x20014770,
    0xbf004770,
    0xc000685c,
    0xc0005d34,
    0xd02115f2,
    0xc0006860,
    0x00000001,
    0x00020202,
    0x9805bf00,
    0xffd2f7ff,
    0xd1012800,
    0xbd61f628,
    0xbd9df628,
};

const uint32_t bes1400_t4_patch2b_ins_data[] =
{
    0xf855b2e6, /*6880*/
    0xB1091b04,
    0xbd79f610,
    0xbd80f610,
};

const uint32_t bes1400_t4_patch2c_ins_data[] =
{
    /*6894*/ 0x21057a98,
    /*6898*/ 0xbf002201,
    /*689c*/ 0xfb5af61d, // call a020689c -> a0023f54
    /*68a0*/ 0x7a984b02,
    /*68a4*/ 0xbf002102,
    /*68a8*/ 0xbb36f633, // jump a02068a8 -> a0039f18
    /*68ac*/ 0xc0006358, // sniffer_env
}; // ld_report_master_snoop_switch_result_ack_callback

const uint32_t bes1400_t4_patch2d_ins_data[] =
{
    /*68b0*/ 0x328af885,
    /*68b4*/ 0x22024b02,
    /*68b8*/ 0xbf00559a,
    /*68bc*/ 0xbe78f614, // jump a02068bc -> a001b5b0
    /*68c0*/ 0xc0006860, // link_id_seq
}; // lc_rsw_end_ind link_id_seq[link_id]=2


const uint32_t bes1400_t4_patch_3_ins_data[] =
{
    /*6900*/ 0xbf002002,
    /*6904*/ 0xfef4f605,
    /*6908*/ 0xd1132802, // bne.n 19
    /*690c*/ 0xfdf4f60c,
    /*6910*/ 0xbf00b980, // cbnz 16
    /*6914*/ 0xfc62f5fc,
    /*6918*/ 0x2bff7c03,
    /*691c*/ 0xbf00d00a, // beq.n 10
    /*6920*/ 0x40b2f89b,
    /*6924*/ 0xd00542a3,
    /*6928*/ 0xbf002300,
    /*692c*/ 0x30bbf88b,
    /*6930*/ 0xbf00e006, // b.n 6
    /*6934*/ 0x30bbf89b, // rx_traffic--
    /*6938*/ 0xbf003b01,
    /*693c*/ 0x30bbf88b,
    /*6940*/ 0xbc88f628, // jump a0206940 -> a002f254
}; // adjust music_playing_link traffic

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_3 =
{
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_3_ins_data),
    0x0002f24c,
    0xbb58f1d7, // jump a002f24c -> a0206900
    0xc0006900,
    (uint8_t *)bes1400_t4_patch_3_ins_data
};

const uint32_t bes1400_t4_patch_4_ins_data[] =
{
    /*6948*/ 0x8035d114,
    /*694c*/ 0x235a4628,
    /*6950*/ 0xf000fb03,
    /*6954*/ 0x5a834a08,
    /*6958*/ 0x5380f423,
    /*695c*/ 0x0c1b041b,
    /*6960*/ 0x5a835283,
    /*6964*/ 0x5300f423,
    /*6968*/ 0x0c1b041b,
    /*696c*/ 0xbf005283,
    /*6970*/ 0xbebcf637, // jump a0206970 -> a003e6ec
    /*6974*/ 0xbebcf637, // jump a0206974 -> a003e6f0
    /*6978*/ 0xd02100c8,
}; // llc_util_get_free_conhdl() set nesn sn 0

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_4 =
{
    4,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_4_ins_data),
    0x0003e6e8,
    0xb92ef1c8, // jump a003e6e8 -> a0206948
    0xc0006948,
    (uint8_t *)bes1400_t4_patch_4_ins_data
};

const uint32_t bes1400_t4_patch_5_ins_data[] =
{
    /*6980*/ 0x4b032200,
    /*6984*/ 0xbf00701a,
    /*6988*/ 0x23011ce2,
    /*698c*/ 0xbe2af624, // jump a020698c -> a002b5e4
    /*6990*/ 0xc000099a,
}; // ld_sco_end set inactive_sco_en 0

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_5 =
{
    5,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_5_ins_data),
    0x0002b5e0,
    0xb9cef1db, // jump a002b5e0 -> a0206980
    0xc0006980,
    (uint8_t *)bes1400_t4_patch_5_ins_data
};

const uint32_t bes1400_t4_patch_5a_ins_data[] =
{
    /*6994*/ 0x2b0310db,
    /*6998*/ 0x4b04d104,
    /*699c*/ 0x01c2f883,
    /*69a0*/ 0xb804f627, // jump a02069a0 -> a002d9ac
    /*69a4*/ 0x2b0210db,
    /*69a8*/ 0xbff4f626, // jump a02069a8 -> a002d994
    /*69ac*/ 0xc0005d34,
};//ld_sco_dynamic_switch

const uint32_t bes1400_t4_patch_6_ins_data[] =
{
    0xd00d2b00,
    0xf8384b07,
    0xf4133003,
    0xd0024f00,
    0x4b052201,
    0xf899701a,
    0x2b0130b3,
    0xba2af629,
    0xba9cf629,
    0xd02111f8,
    0xc00069ec,////sco_find_sync_flag
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_6 =
{
    6,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_6_ins_data),
    0x0002fe2c,
    0xbdc8f1d6,
    0xc00069c0,
    (uint8_t *)bes1400_t4_patch_6_ins_data
};//for ble and sco:  sco_find_sync_flag=1 in ld_sco_frm_isr

const uint32_t bes1400_t4_patch_7_ins_data[] =
{
    0x781b4b22,
    0x2200b18b,
    0x701a4b20,
    0x681b4b1b,
    0x4a1b4293,
    0xbfb96813,
    0x2340f423,
    0x2300f443,
    0x3340f423,
    0x3300f443,
    0x4b176013,
    0x2b00781b,
    0x2201d020,
    0x701a4b14,
    0xb1d84680,
    0x781b4b11,
    0x1f03b9c3,
    0x2b01b2db,
    0xf1b8d903,
    0xd1130f09,
    0x2201e00c,
    0x701a4b0d,
    0x0f09f1b8,
    0x2209d103,
    0x701a4b0b,
    0x68eb2704,
    0xb86ef627,
    0x4b072201,
    0xe7f4701a,
    0xb88af627,
    0xb877f627,
    0xd0220140,
    0xd0220144,
    0xc0000978,
    0xc00069ec,////////sco_find_sync_flag
    0xc0006a88,////ble_sco_need_move_flag
    0xc0006a8c,////prev_conflict_type_record
    0x00000000,
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_7 =
{
    7,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_7_ins_data),
    0x0002db28,
    0xbf64f1d8,
    0xc00069f4,
    (uint8_t *)bes1400_t4_patch_7_ins_data
};//for ble and sco:  d0220144 setting  &  OVERLAP_RESERVE2 in ld_sco_avoid_ble_connect


const uint32_t bes1400_t4_patch_8_ins_data[] =
{
    0x781b4b05,
    0xd1032b09,
    0x4b032200,
    0x2704701a,
    0x60eb1be3,
    0xb828f627,
    0xc0006a8c,////prev_conflict_type_record
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_8 =
{
    8,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_8_ins_data),
    0x0002db04,
    0xbfccf1d8,
    0xc0006aa0,
    (uint8_t *)bes1400_t4_patch_8_ins_data
};//for ble and sco:  prev_conflict_type_record in ld_sco_avoid_ble_connect

const uint32_t bes1400_t4_patch_9_ins_data[] =
{
    0x2c021b64,
    0x2009d101,
    0x3402e002,
    0xbb0af63c,
    0xbb66f63c,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_9 =
{
    9,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_9_ins_data),
    0x000430e0,
    0xbceef1c3,
    0xc0006ac0,
    (uint8_t *)bes1400_t4_patch_9_ins_data
};//for ble and sco:  OVERLAP_RESERVE2 in lld_check_conflict_with_ble

const uint32_t bes1400_t4_patch_10_ins_data[] =
{
    0xd1084298,
    0x3047f894,
    0x92004a0f,
    0x22024631,
    0xfc7af61f,
    0x4b0be014,
    0xb143781b,
    0x3047f894,
    0x92004a09,
    0x22024631,
    0xfc6ef61f,
    0xf894e008,
    0x4a053047,
    0x46319200,
    0xf61f2204,
    0xbf00fc65,
    0xb97af62b,
    0xc0006a88,///ble_sco_need_move_flag
    0xa002fc25,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_10 =
{
    10,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_10_ins_data),
    0x00031e04,
    0xbe6cf1d4,
    0xc0006ae0,
    (uint8_t *)bes1400_t4_patch_10_ins_data
};//for ble and sco:  ld_fm_prog_push LD_FRAME_TYPE_ESCO when ble_sco_need_move_flag in ld_sco_evt_start_cbk

const uint32_t bes1400_t4_patch_11_ins_data[] =
{
    0xfbb24a02,
    0xf5f9f3f3,
    0xbf00bd86,
    0x9af8da00,
};


const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_11 =
{
    11,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_11_ins_data),
    0x00000650,
    0xba76f206,
    0xc0006b40,
    (uint8_t *)bes1400_t4_patch_11_ins_data
};//lpclk

const uint32_t bes1400_t4_patch_12_ins_data[] =
{
    /*6b54*/ 0xd1052b01,
    /*6b58*/ 0x2b0178f3,
    /*6b5c*/ 0xbf00d902,
    /*6b60*/ 0xbc5af618, // jump a0206b60 -> a001f418
    /*6b64*/ 0xbc62f618, // jump a0206b64 -> a001f42c
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_12 =
{
    12,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_12_ins_data),
    0x0001f414,
    0xbb9ef1e7, // jump a001f414 -> a0206b54
    0xc0006b54,
    (uint8_t *)bes1400_t4_patch_12_ins_data
};//LMP_MSG_HANDLER(feats_res_ext)

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_13 =
{
    13,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_5a_ins_data),
    0x0002d990,
    0xb800f1d9, // jump a002d990 -> a0206994
    0xc0006994,
    (uint8_t *)bes1400_t4_patch_5a_ins_data
};//ld_sco_dynamic_switch


const uint32_t bes1400_t4_patch_14_ins_data[] =
{
    0x781b4b0b,
    0x0f53ebb4,
    0x4b0ed00e,
    0x31c0f8b3,
    0x0318f003,
    0xd0072b00,
    0x4b062201,
    0x4b07601a,
    0x0000601c,
    0xbd7cf624,
    0x22000164,
    0xbd76f624,
    0xc00060bd,
    0xc0006ba0,//acl_end
    0x00000000,
    0xc0006ba8,//background_sco_link
    0x000000ff,
    0xc0005d34,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_14 =
{
    14,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch_14_ins_data),
    0x0002b680,
    0xba72f1db,
    0xc0006b68,
    (uint8_t *)bes1400_t4_patch_14_ins_data
};//ld_sco_end

const uint32_t bes1400_t4_patch15_ins_data[] =
{
    0x681b4b0a,
    0xd00c2b00,
    0x681b4b09,
    0xd0082bff,
    0x2200015b,
    0x505a4907,
    0x601a4b04,
    0x4b0422ff,
    0x60af601a,
    0x00004620,
    0xbf10f628,
    0xc0006ba0,
    0xc0006ba8,
    0xd0220140,
    0xc0000980,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_15 =
{
    15,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch15_ins_data),
    0x0002fa00,
    0xb8daf1d7,
    0xc0006bb8,
    (uint8_t *)bes1400_t4_patch15_ins_data
};//ld_acl_frm_isr
//if acl_end and link, bt_e_scochancntl_pack(background_sco_link, 0, 0, 0, 0, 0);

const uint32_t bes1400_t4_patch16_ins_data[] =
{
    0x4b042201,
    0x2200601a,
    0x701a4b04,
    0x00004b03,
    0xbebbf626,
    0xc0006c14,//switch_sco_flag
    0x00000000,
    0xc000099a,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_16 =
{
    16,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch16_ins_data),
    0x0002d984,
    0xb93af1d9,
    0xc0006bfc,
    (uint8_t *)bes1400_t4_patch16_ins_data
};//ld_sco_dynamic_switch, set switch to link
//inactive_sco_en = 0,switch_sco_flag = 1

const uint32_t bes1400_t4_patch17_ins_data[] =
{
    0x3a700147,
    0xf44358bb,
    0x50bb4380,
    0xf8b34b05,
    0xf3c331c0,
    0x2b0303c1,
    0x2201d102,
    0x701a4b02,
    0xbffff626,
    0xc0005d34,
    0xc000099a,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_17 =
{
    17,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch17_ins_data),
    0x0002dcc0,
    0xbff2f1d8,
    0xc0006ca8,
    (uint8_t *)bes1400_t4_patch17_ins_data
};//ld_sco_switch_to_handler, set switch to link
//(bt_e_scochancntl_e_scochanen_setf 1)

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_18 =
{
    18,
    BTDRV_PATCH_ACT,
    0,
    0x0002b660,
    0xe0066013,
    0,
    NULL
};//ld_sco_end2

// __SW_SEQ_FILTER__ - add software seqn filter to protect recive identical repeating packages
#if defined(IBRT)
#define __SW_SEQ_FILTER__
#endif
const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_19 =
{
    19,
#ifdef __SW_SEQ_FILTER__
    BTDRV_PATCH_ACT,
#else
    BTDRV_PATCH_INACT,
#endif
    sizeof(bes1400_t4_patch2a_ins_data),
    0x0002f330,
    0xba98f1d7,
    0xc0006810,
    (uint8_t *)bes1400_t4_patch2a_ins_data
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_20 =
{
    20,
    BTDRV_PATCH_ACT,
    0,
    0x00039cb8,
    0xd13b2902,
    0,
    NULL
};//ibrt switch op

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_21 =
{
    21,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch2b_ins_data),
    0x00017378,
    0xba82f1ef,
    0xc0006880,
    (uint8_t *)bes1400_t4_patch2b_ins_data
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_22 =
{
    22,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch2c_ins_data),
    0x00039f14,
    0xbcbef1cc, // jump a0039f14 -> a0206894
    0xc0006894,
    (uint8_t *)bes1400_t4_patch2c_ins_data
}; // ld_report_master_snoop_switch_result_ack_callback

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_23 =
{
    23,
    BTDRV_PATCH_ACT,
    0,
    0x000387e8,
    0xbf00e001,
    0,
    NULL
};//open afh update

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_24 =
{
    24,
#ifdef __SW_SEQ_FILTER__
    BTDRV_PATCH_ACT,
#else
    BTDRV_PATCH_INACT,
#endif
    sizeof(bes1400_t4_patch2d_ins_data),
    0x0001b5ac,
    0xb980f1eb, // jump a001b5ac -> a02068b0
    0xc00068b0,
    (uint8_t *)bes1400_t4_patch2d_ins_data
}; // lc_rsw_end_ind link_id_seq[link_id]=2

const uint32_t bes1400_t4_patch25_ins_data[] =
{
    /*6c3c*/ 0xbf00b9d0,
    /*6c40*/ 0x8310f3ef,
    /*6c44*/ 0x0301f003,
    /*6c48*/ 0xb672b903,
    /*6c4c*/ 0xfa92f601, //call a0008174
    /*6c50*/ 0xb408b148,
    /*6c54*/ 0xff36f63f, //call a0046ac4
    /*6c58*/ 0xb120bc08,
    /*6c5c*/ 0xfce6f5fa, //call a000162c
    /*6c60*/ 0xb91bb108,
    /*6c64*/ 0xb92be004,
    /*6c68*/ 0xe0052000,
    /*6c6c*/ 0xbbf5f5f9, //jump a000045a
    /*6c70*/ 0xbbdbf5f9, //jump a000042a
    /*6c74*/ 0xbbc6f5f9, //jump a0000404
    /*6c78*/ 0xbbaef5f9, //jump a00003d8
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_25 =
{
    25,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch25_ins_data),
    0x000003c4,
    0xbc3af206, // jump a00003c4 -> a0206c3c
    0xc0006c3c,
    (uint8_t *)bes1400_t4_patch25_ins_data
}; // rwip_sleep() pscan assert when sniff

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_26 =
{
    26,
    BTDRV_PATCH_ACT,
    0,
    0x00030180,
    0xe0022000,
    0,
    NULL
};//acl end assert
const uint32_t bes1400_t4_patch27_ins_data[] =
{
    0x781b4b03,
    0xd1012b01,
    0xbbf8f633,
    0xbbfff633,
    0xc0006358,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_27 =
{
    27,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch27_ins_data),
    0x0003a468,
    0xbc0af1cc,
    0xc0006C80,
    (uint8_t *)bes1400_t4_patch27_ins_data
};//snoop disconnect

const uint32_t bes1400_t4_patch28_ins_data[] =
{
    0x0f01f1b8,
    0x380ebf0c,
    0xb280301c,
    0xbaccf632,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_28 =
{
    28,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch28_ins_data),
    0x00039218,
    0xbd3ef1cd,
    0xc0006C98,
    (uint8_t *)bes1400_t4_patch28_ins_data
};//start snoop using bitoff

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_29 =
{
    29,
#if defined(IBRT)
    BTDRV_PATCH_ACT,
#else
    BTDRV_PATCH_INACT,
#endif
    0,
    0x0002e094,
    0xf8a6230a,
    0,
    NULL,
};//ld_acl_start t_poll

const uint32_t bes1400_t4_patch30_ins_data[] =
{
    0x68124a0d,
    0xd1132a01,
    0x68104a0c,
    0x60103001,
    0xd10d280c,
    0xf890480b,
    0x014001c2,
    0x5842490a,
    0x4280f422,
    0x20005042,
    0x60104a04,
    0x60104a02,
    0x46294630,
    0xbcc0f623,
    0xc0006c14,//switch_sco_flag
    0xc0006d14,//switch_delay_counter
    0x00000000,
    0xc0005d34,
    0xd0220140,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_30 =
{
    30,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch30_ins_data),
    0x0002a688,
    0xbb24f1dc,
    0xc0006cd4,
    (uint8_t *)bes1400_t4_patch30_ins_data
};//ld_acl_clk_isr, set switch to link
//(bt_e_scochancntl_e_scochanen_setf 0)

const uint32_t bes1400_t4_patch31_ins_data[] =
{
    /*6d24*/ 0x20014a02,
    /*6d28*/ 0xb0045590,
    /*6d2c*/ 0xb850f61d, //jump a0023dd0
    /*6d30*/ 0xc0006d34,
    /*6d34*/ 0x00000000, //lmp_no_buf
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_31 =
{
    31,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch31_ins_data),
    0x00023da0,
    0xbfc0f1e2, //jump a0023da0 -> a0206d24
    0xc0006d24,
    (uint8_t *)bes1400_t4_patch31_ins_data
};//lc_send_lmp

const uint32_t bes1400_t4_patch32_ins_data[] =
{
    /*6d3c*/ 0x5d914a05,
    /*6d40*/ 0x2100b921,
    /*6d44*/ 0xbf00460a,
    /*6d48*/ 0xbbb2f625, //jump a002c4b0
    /*6d4c*/ 0x55902000,
    /*6d50*/ 0xbbdaf625, //jump a002c508
    /*6d54*/ 0xc0006d34, //lmp_no_buf
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_32 =
{
    32,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_patch32_ins_data),
    0x0002c4ac,
    0xbc46f1da, //jump a002c4ac -> a0206d3c
    0xc0006d3c,
    (uint8_t *)bes1400_t4_patch32_ins_data
};//ld_acl_sched

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_33 =
{
    33,
    BTDRV_PATCH_ACT,
    0,
    0x0003884c,
    0xbf00e008,
    0,
    NULL,
};// ld_sniffer_update_status: SNIFFER_EVT_SCO_CONNECTED

static const uint32_t best1400_t4_ins_patch_config[] =
{
    34,
    (uint32_t)&bes1400_t4_ins_patch_0,
    (uint32_t)&bes1400_t4_ins_patch_1,
    (uint32_t)&bes1400_t4_ins_patch_2,
    (uint32_t)&bes1400_t4_ins_patch_3,
    (uint32_t)&bes1400_t4_ins_patch_4,
    (uint32_t)&bes1400_t4_ins_patch_5,
    (uint32_t)&bes1400_t4_ins_patch_6,
    (uint32_t)&bes1400_t4_ins_patch_7,
    (uint32_t)&bes1400_t4_ins_patch_8,
    (uint32_t)&bes1400_t4_ins_patch_9,
    (uint32_t)&bes1400_t4_ins_patch_10,
    (uint32_t)&bes1400_t4_ins_patch_11,
    (uint32_t)&bes1400_t4_ins_patch_12,
    (uint32_t)&bes1400_t4_ins_patch_13,
    (uint32_t)&bes1400_t4_ins_patch_14,
    (uint32_t)&bes1400_t4_ins_patch_15,
    (uint32_t)&bes1400_t4_ins_patch_16,
    (uint32_t)&bes1400_t4_ins_patch_17,
    (uint32_t)&bes1400_t4_ins_patch_18,
    (uint32_t)&bes1400_t4_ins_patch_19,
    (uint32_t)&bes1400_t4_ins_patch_20,
    (uint32_t)&bes1400_t4_ins_patch_21,
    (uint32_t)&bes1400_t4_ins_patch_22,
    (uint32_t)&bes1400_t4_ins_patch_23,
    (uint32_t)&bes1400_t4_ins_patch_24,
    (uint32_t)&bes1400_t4_ins_patch_25,
    (uint32_t)&bes1400_t4_ins_patch_26,
    (uint32_t)&bes1400_t4_ins_patch_27,
    (uint32_t)&bes1400_t4_ins_patch_28,
    (uint32_t)&bes1400_t4_ins_patch_29,
    (uint32_t)&bes1400_t4_ins_patch_30,
    (uint32_t)&bes1400_t4_ins_patch_31,
    (uint32_t)&bes1400_t4_ins_patch_32,
    (uint32_t)&bes1400_t4_ins_patch_33,
};


/*****************************************************************************
 Prototype    : btdrv_ins_patch_write
 Description  : bt driver instruction patch write
 Input        : BTDRV_PATCH_STRUCT *ins_patch_p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2018/12/6
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_ins_patch_write(BTDRV_PATCH_STRUCT *ins_patch_p)
{
    uint32_t remap_addr;
    uint8_t i=0;
    remap_addr =   ins_patch_p->patch_remap_address | 1;
    btdrv_write_memory(_32_Bit,(BTDRV_PATCH_INS_REMAP_ADDR_START + ins_patch_p->patch_index*4),
                       (uint8_t *)&ins_patch_p->patch_remap_value,4);
    if(ins_patch_p->patch_length != 0)  //have ram patch data
    {
        for( ; i<(ins_patch_p->patch_length)/128; i++)
        {
            btdrv_write_memory(_32_Bit,ins_patch_p->patch_start_address+i*128,
                               (ins_patch_p->patch_data+i*128),128);
        }

        btdrv_write_memory(_32_Bit,ins_patch_p->patch_start_address+i*128,ins_patch_p->patch_data+i*128,
                           ins_patch_p->patch_length%128);
    }

    btdrv_write_memory(_32_Bit,(BTDRV_PATCH_INS_COMP_ADDR_START + ins_patch_p->patch_index*4),
                       (uint8_t *)&remap_addr,4);

}
/*****************************************************************************
 Prototype    : btdrv instruction patch init
 Description  : btdrv_ins_patch_init
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/12/6
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_ins_patch_init(void)
{
    const BTDRV_PATCH_STRUCT *ins_patch_p;
    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0)
    {
        for(uint8_t i=0; i<best1400_ins_patch_config[0]; i++)
        {
            ins_patch_p = (BTDRV_PATCH_STRUCT *)best1400_ins_patch_config[i+1];
            if(ins_patch_p->patch_state == BTDRV_PATCH_ACT)
            {
                btdrv_ins_patch_write((BTDRV_PATCH_STRUCT *)best1400_ins_patch_config[i+1]);
            }
            if (1 == i)
            {
                /*when intersys patch send done,enable patch directly to avoid interrupt missing*/
                btdrv_patch_en(1);
                btdrv_delay(100);
                hal_intersys_peer_irq_auto_clear(false);
            }
        }
    }
    else if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1)
    {
        for(uint8_t i=0; i<best1400_t4_ins_patch_config[0]; i++)
        {
            ins_patch_p = (BTDRV_PATCH_STRUCT *)best1400_t4_ins_patch_config[i+1];
            if(ins_patch_p->patch_state == BTDRV_PATCH_ACT)
            {
                btdrv_ins_patch_write((BTDRV_PATCH_STRUCT *)best1400_t4_ins_patch_config[i+1]);
            }
        }
        btdrv_patch_en(1);
    }
}

static const uint32_t bes1400_dpatch0_data[] =
{
    0x4000007C,
};

static const BTDRV_PATCH_STRUCT bes1400_data_patch0 =
{
    0,
    BTDRV_PATCH_INACT,
    sizeof(bes1400_dpatch0_data),
    0x00006EF0,
    0xc0006720,
    0xc0006720,
    (uint8_t *)&bes1400_dpatch0_data
};

static const BTDRV_PATCH_STRUCT bes1400_data_patch1 =
{
    1,
    BTDRV_PATCH_INACT,
    sizeof(bes1400_dpatch0_data),
    0x00006F70,
    0xc0006720,
    0xc0006720,
    (uint8_t *)&bes1400_dpatch0_data
};

static const uint32_t best1400_data_patch_config[] =
{
    2,
    (uint32_t)&bes1400_data_patch0,
    (uint32_t)&bes1400_data_patch1,
};
/*****************************************************************************
 Prototype    : btdrv_data_patch_write
 Description  :  btdrv data patch write function
 Input        : const BTDRV_PATCH_STRUCT *d_patch_p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/12/6
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_data_patch_write(const BTDRV_PATCH_STRUCT *d_patch_p)
{

    uint32_t remap_addr;
    uint8_t i=0;

    remap_addr = d_patch_p->patch_remap_address |1;
    btdrv_write_memory(_32_Bit,(BTDRV_PATCH_DATA_COMP_ADDR_START + d_patch_p->patch_index*4),
                       (uint8_t *)&remap_addr,4);
    btdrv_write_memory(_32_Bit,(BTDRV_PATCH_DATA_REMAP_ADDR_START + d_patch_p->patch_index*4),
                       (uint8_t *)&d_patch_p->patch_remap_value,4);

    if(d_patch_p->patch_length != 0)  //have ram patch data
    {
        for( ; i<(d_patch_p->patch_length-1)/128; i++)
        {
            btdrv_write_memory(_32_Bit,d_patch_p->patch_start_address+i*128,
                               (d_patch_p->patch_data+i*128),128);

        }
        btdrv_write_memory(_32_Bit,d_patch_p->patch_start_address+i*128,d_patch_p->patch_data+i*128,
                           d_patch_p->patch_length%128);
    }

}


void btdrv_ins_patch_disable(uint8_t index)
{
    uint32_t addr=0;
    btdrv_write_memory(_32_Bit,(BTDRV_PATCH_INS_COMP_ADDR_START + index*4),
                       (uint8_t *)&addr,4);

}
/*****************************************************************************
 Prototype    : btdrv_data_patch_init
 Description  : btdrv data data patch init function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/12/6
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_data_patch_init(void)
{
    const BTDRV_PATCH_STRUCT *data_patch_p;
    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0)
    {
        for(uint8_t i=0; i<best1400_data_patch_config[0]; i++)
        {
            data_patch_p = (BTDRV_PATCH_STRUCT *)best1400_data_patch_config[i+1];
            if(data_patch_p->patch_state == BTDRV_PATCH_ACT)
                btdrv_data_patch_write((BTDRV_PATCH_STRUCT *)best1400_data_patch_config[i+1]);
        }
    }
}
/*****************************************************************************
 Prototype    : btdrv_patch_en
 Description  : patch enable function
 Input        : uint8_t en
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/12/6
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_patch_en(uint8_t en)
{
    uint32_t value[2];

    //set patch enable
    value[0] = 0x2f02 | en;
    //set patch remap address  to 0xc0000100
    value[1] = 0x20000100;
    btdrv_write_memory(_32_Bit,BTDRV_PATCH_EN_REG,(uint8_t *)&value,8);
}
/*****************************************************************************
 Patch Description  : lc_get_ptt will always return 0 when in no-signal rx mode
 Patch function     : lc_get_ptt
 Patch Line         : line4428
 Patch Level        : test mode patch, not used in offical release

 History      :
 Date         : 2018/12/12
 Author       : lufang.qin
*****************************************************************************/
const uint32_t bes1400_ins_patch_testmode_data[] =
{
    /*60*/ 0xbf002000,//mov r0 0
    /*64*/ 0xbf004b02,//ldr r3,[pc,#2]
    /*68*/ 0xbf006018,//str r0 [r3,#0]
    /*6c*/ 0xba90f624,//B 0xa002ac90
    /*70*/ 0xc0006774,//data
    /*74*/ 0xabcdabcd,//data
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch_testmode =
{
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_ins_patch_testmode_data),
    0x0002ac8c,
    0xbd68f1db,
    0xc0006760,
    (uint8_t *)bes1400_ins_patch_testmode_data
};
/*****************************************************************************
 Prototype    : btdrv_ins_patch_test_init
 Description  : test mode patch init function
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2018/12/6
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
const uint32_t bes1400_patch3_ins_test_data[] =
{
    0xf0025bf2,
    0xbf00080f,
    0xbc05f622,
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch3_testmode =
{
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch3_ins_test_data),
    0x00028f30,
    0xbbf6f1dd,
    0xc0006720,
    (uint8_t *)bes1400_patch3_ins_test_data
};


const uint32_t bes1400_patch4_ins_test_data[] =
{
    0xf0025bf2,
    0xbf00020f,
    0xbc22f622,
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch4_testmode =
{
    4,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch4_ins_test_data),
    0x00028f7c,
    0xbbd8f1dd,
    0xc0006730,
    (uint8_t *)bes1400_patch4_ins_test_data
};



const uint32_t bes1400_patch5_ins_test_data[] =
{
    0xf0035bf3,
    0xbf00080f,
    0xbc32f622,

};

const BTDRV_PATCH_STRUCT bes1400_ins_patch5_testmode =
{
    5,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch5_ins_test_data),
    0x00028fac,
    0xbbc8f1dd,
    0xc0006740,
    (uint8_t *)bes1400_patch5_ins_test_data
};



const uint32_t bes1400_patch6_ins_test_data[] =
{
    0xf0035bf3,
    0xbf00010f,
    0xbc52f622,

};

const BTDRV_PATCH_STRUCT bes1400_ins_patch6_testmode =
{
    6,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch6_ins_test_data),
    0x00028ffc,
    0xbba8f1dd,
    0xc0006750,
    (uint8_t *)bes1400_patch6_ins_test_data
};


const uint32_t bes1400_patch7_ins_test_data[] =
{
    0x22445243,
    0x601a4b06,
//   0x020cf641,
    0x020cf243,
    0x601a3308,
    0x33382204,
    0x220d601a,
    0x601a3304,
    0xfb04f85d,
    0xd0350300,

};

const BTDRV_PATCH_STRUCT bes1400_ins_patch7_testmode =
{
    7,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch7_ins_test_data),
    0x000038c8,
    0xbf4af202,
    0xc0006760,
    (uint8_t *)bes1400_patch7_ins_test_data
};


const uint32_t bes1400_patch8_ins_test_data[] =
{
    0xd00e2b02,
    0x4b0f2244,
//    0xf641601a,
    0xf242601a,
//   0x33080209,
    0x33080208,
    0x2204601a,
    0x601a3338,
    //  0x33042209,
    0x33042208,
    0x2000601a,
    0x2244e00d,
    0x601a4b07,
//    0x020cf641,
    0x020cf243,
    0x601a3308,
    0x33382204,
    0x220d601a,
    0x601a3304,
    0xf85d2001,
    0x47704b04,
    0xd0350300,

};

const BTDRV_PATCH_STRUCT bes1400_ins_patch8_testmode =
{
    8,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch8_ins_test_data),
    0x000035a0,
    0xb8f0f203,
    0xc0006784,
    (uint8_t *)bes1400_patch8_ins_test_data
};


const uint32_t bes1400_patch9_ins_test_data[] =
{
    0xf0035243,
    0x2b00030f,
    0x2244d00e,
    0x601a4b0e,
//    0x0209f641,
    0x0208f242,
    0x601a3308,
    0x33382204,
//    0x2209601a,
    0x2208601a,
    0x601a3304,
    0xe00d2000,
    0x4b072244,
    //  0xf641601a,
    0xf241601a,
//    0x33080206,
    0x33084205,
    0x2204601a,
    0x601a3338,
//   0x33042206,
    0x33042205,
    0x2001601a,
    0xbf004770,
    0xd0350300,


};

const BTDRV_PATCH_STRUCT bes1400_ins_patch9_testmode =
{
    9,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch9_ins_test_data),
    0x00003568,
    0xb932f203,
    0xc00067d0,
    (uint8_t *)bes1400_patch9_ins_test_data
};

////hw loopback disable and en
const uint32_t bes1400_patch10_ins_test_data[] =
{
    0x30fcf8d4,
    0xd0092b00,
    0x680b4905,
    0x3f00f413,
    0x680bd004,
    0x3300f423,
    0xbf00600b,
    0xbf48f626,
    0xd02200a4,
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch10_testmode =
{
    10,
    BTDRV_PATCH_INACT,
    sizeof(bes1400_patch10_ins_test_data),
    0x0002d6c8,
    0xb8aaf1d9,
    0xc0006820,
    (uint8_t *)bes1400_patch10_ins_test_data
};



const uint32_t bes1400_patch11_ins_test_data[] =
{
    0x30fcf8d4,
    0xd00b2b00,
    0xf003789b,
    0x2b0503fd,
    0x4b04d106,
    0xf443681b,
    0x4a023300,
    0xbf006013,
    0xba9ff625,
    0xd02200a4,
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch11_testmode =
{
    11,
    BTDRV_PATCH_INACT,
    sizeof(bes1400_patch11_ins_test_data),
    0x0002bcd0,
    0xbdbef1da,
    0xc0006850,
    (uint8_t *)bes1400_patch11_ins_test_data
};


const uint32_t bes1400_patch12_ins_test_data[] =
{
    /*687c*/0x6d584b07,
    /*6880*/0xbf002800,
    /*6884*/0xbf00d108,
    /*6888*/0xbf004805,
    /*688c*/0xbf004905,
    /*6890*/0xff86f606,
    /*6894*/0xbf004802,
    /*6898*/0xb932f605,
    /*689c*/0xc00059f0,
    /*68a0*/0xc00068a4,
    /*68a4*/0x00000c13,
    /*68a8*/0x41004142
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch12_testmode =
{
    12,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch12_ins_test_data),
    0x0000bafc,
    0xbebef1fa,//jump 0xa000bafc 0xa020687c
    0xc000687c,
    (uint8_t *)bes1400_patch12_ins_test_data
};//write local name when name_ptr is null pointer


const uint32_t bes1400_patch13_ins_test_data[] =
{
    /*68b0*/ 0xbf004b05,
    /*68b4*/ 0x3020f853,
    /*68b8*/ 0xbf002b00,
    /*68bc*/ 0xbf00d002,
    /*68c0*/ 0xfd7cf60f,
    /*68c4*/ 0xb9e4f624,
    /*68c8*/ 0xc0005b64,
};
const BTDRV_PATCH_STRUCT bes1400_ins_patch13_testmode =
{
    13,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch13_ins_test_data),
    0x0002ac8c,
    0xbe10f1db,//jump 0xa002ac8c 0xa02068b0
    0xc00068b0,
    (uint8_t *)bes1400_patch13_ins_test_data
};//no sig_rx lc_get_ptt assert

////for power control test mode
const BTDRV_PATCH_STRUCT bes1400_ins_patch14_testmode =
{
    14,
    BTDRV_PATCH_ACT,
    0,
    0x00028d1c,
    0xe0cfbf00,
    0,
    NULL
};

const uint32_t bes1400_patch15_ins_test_data[] =
{
    0xd02f2b00,
    0x33fff10b,
    0x2b01b2db,
    0xf8d6d827,
    0x788b10fc,
    0x03fdf003,
    0xd1202b05,
    0x68134a14,
    0x60133301,
    0xf8834b13,
    0x9b058000,
    0x02c9f3c3,
    0x801a4b11,
    0xf8834b11,
    0x8c08b000,
    0x4050f100,
    0x1004f500,
    0xf8934b08,
    0x011b31cb,
    0x5a594907,
    0xf101b289,
    0xf5014150,
    0xf63f1104,
    0x2401fe73,
    0xbeccf625,
    0xbe7cf625,
    0xc0005c28,
    0xd02115b0,
    0xc0006958,
    0xc000695c,
    0xc0006960,
    0xc0006964,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch15_testmode =
{
    15,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch15_ins_test_data),
    0x0002c634,
    0xb950f1da,
    0xc00068d8,
    (uint8_t *)bes1400_patch15_ins_test_data
};


const uint32_t bes1400_patch16_ins_test_data[] =
{
    0xbf00789a,
    0xb2d23a05,
    0xd9002a03,
    0x2a00e040,
    0x2a02d001,
    0xbf00d13e,
    0x681b4b1f,
    0xd0392b00,
    0xeb04bf00,
    0xf8370484,
    0xb29b3014,
    0x0378f023,
    0x88124a1a,
    0x03c2ea43,
    0xf827b29b,
    0x4b193014,
    0xf043681b,
    0x4a160304,
    0xea438812,
    0xb29b03c2,
    0x3014f82a,
    0x30fcf8d5,
    0x4a138c1b,
    0x3014f822,
    0x3014f836,
    0x030ef3c3,
    0x3014f826,
    0x30c2f895,
    0xf383fab3,
    0xf885095b,
    0xf89530c2,
    0x330130c3,
    0x30c3f885,
    0x68134a04,
    0x60133b01,
    0xb8daf625,
    0xbf6af624,
    0xb8dbf625,
    0xc0006958,
    0xc000695c,
    0xc0006960,
    0xc0006964,
    0xd02115ec,
};

const BTDRV_PATCH_STRUCT bes1400_ins_patch16_testmode =
{
    16,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch16_ins_test_data),
    0x0002b8d4,
    0xb850f1db,
    0xc0006978,
    (uint8_t *)bes1400_patch16_ins_test_data
};
////end for power control test mode


const uint32_t bes1400_patch17_ins_test_data[] =
{
    0x60bb9b01,
    0xfb00206e,
    0x4a0bf004,
    0xf0436813,
    0x60130302,
    0x4b092202,
    0xf5a3601a,
    0x3ba6436e,
    0x3003f830,
    0x030cf3c3,
    0x5300f443,
    0xf8204a04,
    0xbf003002,
    0xbe4ef624,
    0xD022000C,
    0xd0220018,
    0xd0211172,

};

const BTDRV_PATCH_STRUCT bes1400_ins_patch17_testmode =
{
    17,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_patch17_ins_test_data),
    0x0002b700,
    0xb996f1db,
    0xc0006a30,
    (uint8_t *)bes1400_patch17_ins_test_data
};


static const uint32_t ins_patch_1400_config_testmode[] =
{
    15,
    (uint32_t)&bes1400_ins_patch3_testmode,
    (uint32_t)&bes1400_ins_patch4_testmode,
    (uint32_t)&bes1400_ins_patch5_testmode,
    (uint32_t)&bes1400_ins_patch6_testmode,
    (uint32_t)&bes1400_ins_patch7_testmode,
    (uint32_t)&bes1400_ins_patch8_testmode,
    (uint32_t)&bes1400_ins_patch9_testmode,
    (uint32_t)&bes1400_ins_patch10_testmode,
    (uint32_t)&bes1400_ins_patch11_testmode,
    (uint32_t)&bes1400_ins_patch12_testmode,
    (uint32_t)&bes1400_ins_patch13_testmode,
    (uint32_t)&bes1400_ins_patch14_testmode,
    (uint32_t)&bes1400_ins_patch15_testmode,
    (uint32_t)&bes1400_ins_patch16_testmode,
    (uint32_t)&bes1400_ins_patch17_testmode,

};

/*****************************************************************************
CHIP ID                : 1400
Metal id               : 1
BT controller tag      : t4
*****************************************************************************/

const uint32_t bes1400_t4_ins_patch_data_testmode_0[] =
{
    /*6900*/ 0x6d5b4b0d,
    /*6904*/ 0xbf00b9a3, // cbnz local_name
    /*6908*/ 0x70132307,
    /*690c*/ 0x70032344,
    /*6910*/ 0x70432355,
    /*6914*/ 0x70832354,
    /*6918*/ 0x70c32331,
    /*691c*/ 0x71022234,
    /*6920*/ 0x71422230,
    /*6924*/ 0xbf007183,
    /*6928*/ 0x71c32300,
    /*692c*/ 0xb85ef606, // jump a000c9ec
    /*6930*/ 0x6d584b01,
    /*6934*/ 0xb81cf606, // jump a000c970
    /*6938*/ 0xc0005afc,
}; // send default name when null pointer

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_testmode_0 =
{
    0,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_ins_patch_data_testmode_0),
    0x0000c96c,
    0xbfc8f1f9, // jump a000c96c -> a0206900
    0xc0006900,
    (uint8_t*)bes1400_t4_ins_patch_data_testmode_0
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_testmode_1 =
{
    1,
    BTDRV_PATCH_ACT,
    0,
    0x0002f438,
    0x789abf00,
    0,
    NULL
};///test mode: remove rxllid's judging condition in ld_acl_rx

const uint32_t bes1400_t4_ins_patch_data_testmode_2[] =
{
    0x20fcf8d5,
    0xf8928811,
    0xf0088007,
    0xb909080f,
    0x88194b11,
    0x0484eb04,
    0x3014f837,
    0xf423b29b,
    0x4a0f7300,
    0xea438812,
    0xb29b2342,
    0x3014f827,
    0x3014f837,
    0xf023b29b,
    0xea430378,
    0xf82708c8,
    0x4b068014,
    0xf043781b,
    0xea430304,
    0xb28b01c1,
    0x3014f82a,
    0xbe6af627,
    0xc000099c,////loopback_length
    0xc000097b,////loopback_llid
    0xc00069a4,////rxseq_flag
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_testmode_2 =
{
    2,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_ins_patch_data_testmode_2),
    0x0002e604,
    0xb99cf1d8,
    0xc0006940,
    (uint8_t*)bes1400_t4_ins_patch_data_testmode_2
};///test mode: ld_acl_tx_prog


const uint32_t bes1400_t4_ins_patch_data_testmode_3[] =
{
    0xfc14f5fc,
    0x3060f890,
    0xd01f2b00,
    0x30fcf8db,
    0xd01b2b00,
    0x0f03f1b9,
    0xf1b9d018,
    0xd0150f08,
    0xf8934b0b,
    0x011b31cb,
    0x5a9b4a0a,
    0x7f80f413,
    0x4b09d10a,
    0x4b0a781a,
    0x4b08601a,
    0x4b09881a,
    0x2201601a,
    0x601a4b08,
    0xbc66f628,
    0xbc62f628,
    0xc0005d34,
    0xd02115f2,
    0xc000097b,////loopback_llid
    0xc000099c,////loopback_length
    0xc0006a1c,////rxllid_flag
    0xc0006a20,////rxlength_flag
    0xc0006a24,////unack_seqerr_flag
    0x00000000,
    0x00000000,
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_testmode_3 =
{
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_ins_patch_data_testmode_3),
    0x0002f2c0,
    0xbb76f1d7,
    0xc00069b0,
    (uint8_t*)bes1400_t4_ins_patch_data_testmode_3
};///test mode: skip rx seq err in ld_acl_rx


const uint32_t bes1400_t4_ins_patch_data_testmode_4[] =
{
    0xf015d002,
    0xd0010f24,
    0xbc46f628,
    0xbd28f628,
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_testmode_4 =
{
    4,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_ins_patch_data_testmode_4),
    0x0002f2cc,
    0xbbb4f1d7,
    0xc0006a38,
    (uint8_t*)bes1400_t4_ins_patch_data_testmode_4
};///test mode: skip crc err in ld_acl_rx


const uint32_t bes1400_t4_ins_patch_data_testmode_5[] =
{
    0x700d4914,
    0xf891490c,
    0x010911cb,
    0x5a8a4a0b,
    0x2240f3c2,
    0x6002480a,
    0x6800480a,
    0x2100b150,
    0x60014808,
    0x68014808,
    0x80014808,
    0x68014808,
    0x70014808,
    0xbce6f628,
    0xc0005d34,
    0xd02115f2,
    0xc00069a4,////rxseq_flag
    0xc0006a24,////unack_seqerr_flag
    0xc0006a20,////rxlength_flag
    0xc000099c,////loopback_length
    0xc0006a1c,////rxllid_flag
    0xc000097b,////loopback_llid
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_testmode_5 =
{
    5,
    BTDRV_PATCH_ACT,
    sizeof(bes1400_t4_ins_patch_data_testmode_5),
    0x0002f45c,
    0xbafef1d7,
    0xc0006a5c,
    (uint8_t*)bes1400_t4_ins_patch_data_testmode_5
};


const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_testmode_6 =
{
    6,
    BTDRV_PATCH_ACT,
    0,
    0x0003a6e4,
    0x2005e004,
    0,
    NULL
};

const BTDRV_PATCH_STRUCT bes1400_t4_ins_patch_testmode_7 =
{
    7,
    BTDRV_PATCH_ACT,
    0,
    0x00006f8c,
    0xbdf8bf00,
    0,
    NULL
};




static const uint32_t ins_patch_1400_t4_config_testmode[] =
{
    8,
    (uint32_t)&bes1400_t4_ins_patch_testmode_0,
    (uint32_t)&bes1400_t4_ins_patch_testmode_1,
    (uint32_t)&bes1400_t4_ins_patch_testmode_2,
    (uint32_t)&bes1400_t4_ins_patch_testmode_3,
    (uint32_t)&bes1400_t4_ins_patch_testmode_4,
    (uint32_t)&bes1400_t4_ins_patch_testmode_5,
    (uint32_t)&bes1400_t4_ins_patch_testmode_6,
    (uint32_t)&bes1400_t4_ins_patch_testmode_7,
    
};


void btdrv_ins_patch_test_init(void)
{

    const BTDRV_PATCH_STRUCT *ins_patch_p;
    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0 )
    {
        //btdrv_patch_en(0);
        btdrv_ins_patch_disable(3);
        btdrv_ins_patch_disable(4);
        btdrv_ins_patch_disable(5);
        btdrv_ins_patch_disable(6);
        btdrv_ins_patch_disable(7);
        btdrv_ins_patch_disable(8);
        btdrv_ins_patch_disable(9);
        btdrv_ins_patch_disable(10);
        btdrv_ins_patch_disable(11);
        btdrv_ins_patch_disable(12);
        btdrv_ins_patch_disable(13);

        for(uint8_t i=0; i<ins_patch_1400_config_testmode[0]; i++)
        {
            ins_patch_p = (BTDRV_PATCH_STRUCT *)ins_patch_1400_config_testmode[i+1];
            if(ins_patch_p->patch_state ==BTDRV_PATCH_ACT)
                btdrv_ins_patch_write((BTDRV_PATCH_STRUCT *)ins_patch_1400_config_testmode[i+1]);
        }
        //btdrv_patch_en(1);
    }
    else if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1)
    {
        btdrv_patch_en(0);

        for (int i = 3; i < 30; i += 1)
        {
            btdrv_ins_patch_disable(i);
        }

        *(volatile uint8_t*)0xc0005873 = 0x00; // dbg_trace_level

        for(uint8_t i = 0; i < ins_patch_1400_t4_config_testmode[0]; i++)
        {
            ins_patch_p = (BTDRV_PATCH_STRUCT *)ins_patch_1400_t4_config_testmode[i+1];
            if(ins_patch_p->patch_state ==BTDRV_PATCH_ACT)
                btdrv_ins_patch_write((BTDRV_PATCH_STRUCT *)ins_patch_1400_t4_config_testmode[i+1]);
        }

        btdrv_patch_en(1);
    }
}
