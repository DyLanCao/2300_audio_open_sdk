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
#include "bt_drv.h"
#include "bt_drv_interface.h"
#include "bt_drv_2300p_internal.h"

extern void btdrv_write_memory(uint8_t wr_type,uint32_t address,const uint8_t *value,uint8_t length);

///enable m4 patch func
#define BTDRV_PATCH_EN_REG   0xe0002000

//set m4 patch remap adress
#define BTDRV_PATCH_REMAP_REG    0xe0002004

////instruction patch compare src address
#define BTDRV_PATCH_INS_COMP_ADDR_START   0xe0002008

#define BTDRV_PATCH_INS_REMAP_ADDR_START   0xc0000100

////data patch compare src address
#define BTDRV_PATCH_DATA_COMP_ADDR_START   0xe00020e8

#define BTDRV_PATCH_DATA_REMAP_ADDR_START   0xc00001e0



#define BTDRV_PATCH_ACT   0x1
#define BTDRV_PATCH_INACT   0x0



typedef struct
{
    uint8_t patch_index;   //patch position
    uint8_t patch_state;   //is patch active
    uint16_t patch_length;     ///patch length 0:one instrution replace  other:jump to ram to run more instruction
    uint32_t patch_remap_address;   //patch occured address
    uint32_t patch_remap_value;      //patch replaced instuction
    uint32_t patch_start_address;    ///ram patch address for lenth>0
    uint8_t *patch_data;                  //ram patch date for length >0

} BTDRV_PATCH_STRUCT;


/***************************************************************************
 *
 * instruction patch Information
 *
 * BT ROM Chip Version:1302 T1
 *
 * [IBRT]
 * patch  7: 0xC0006a50--->0xC0006a78
 * patch  8: 0xC0006a90--->0xC0006a9C
 * patch  9: unused
 * patch 10: 0xC0006AA0--->0xC0006AC8
 * patch 11: 0xC0006AD0--->0xC0006AEC
 * patch 12: 0xC0006AF0--->0xC0006B04
 * patch 13: 0xC0006B08--->0xC0006B18
 * patch 14: in use
 * patch 15: 0xC0006B74--->0xC0006B80 
 * patch 16: 0xC0006B84--->0xC0006B94 
 * patch 17: 0xC0006B98--->0xC0006BD8 
 * patch 18: 0xC0006BDC--->0xC0006C70 
 * patch 19: in used
 * patch 20: in used
 * patch 21: 0xC0006C78--->0xC0006C88 
 * patch 22: 0xC0006810--->0xC0006878
 * patch 23: 0xC0006880--->0xC0006890
 * patch 24: 0xC0006C90--->0xC000689C
 * patch 25: in used:ld_acl_start t_poll
 * patch 26: 0xC0006CA0--->0xC0006CC8:  unused
 * patch 27: in used: ld_sniffer_update_status: SNIFFER_EVT_SCO_CONNECTED
 ****************************************************************************/

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch0 =
{
    0,
    BTDRV_PATCH_ACT,
    0,
    0x00025ef4,
    0xbf00bf00,
    0,
    NULL
};///remove the memset of ldenv

const uint32_t bes2300p_t1_patch_0a_ins_data[] =
{
    /*6810*/ 0x681b4b0e,
    /*6814*/ 0x4b0eb1ab,
    /*6818*/ 0x31cbf893,
    /*681c*/ 0x4a0d011b,
    /*6820*/ 0xf3c35a9b,
    /*6824*/ 0x4a0c2340,
    /*6828*/ 0x2a025c12,
    /*682c*/ 0x4a0ad103,
    /*6830*/ 0x20005413,
    /*6834*/ 0x42934770,
    /*6838*/ 0x4a07d005,
    /*683c*/ 0x20005413,
    /*6840*/ 0x20004770,
    /*6844*/ 0x20014770,
    /*6848*/ 0xbf004770,
    /*684c*/ 0xc000685c,
    /*6850*/ 0xc0005d34,
    /*6854*/ 0xd02115f2,
    /*6858*/ 0xc0006860,
    /*685c*/ 0x00000001, // sw_seqn_filter_en
    /*6860*/ 0x00020202, // link_id_seq
    /*6864*/ 0x9805bf00,
    /*6868*/ 0xffd2f7ff, // call a0206810
    /*686c*/ 0x2019b910,
    /*6870*/ 0xbd62f628, // jump a002f338
    /*6874*/ 0x22802107,
    /*6878*/ 0xbe22f628, // jump a002f4c0
}; // ld_acl_rx ld_sw_seqn_filter

const uint32_t bes2300p_t1_patch_0b_ins_data[] =
{
    /*6880*/ 0x328af885,
    /*6884*/ 0x22024b02,
    /*6888*/ 0xbf00559a,
    /*688c*/ 0xbe90f614, // jump a001b5b0
    /*6890*/ 0xc0006860, // link_id_seq
}; // lc_rsw_end_ind link_id_seq[link_id]=2

const uint32_t bes2300p_t1_patch_1_ins_data[] =
{
    0x2c021b64,
    0x2009d101,
    0x3402e002,
    0xbc4af63c,
    0xbca6f63c,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch1 =
{
    1,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_t1_patch_1_ins_data),
    0x000431a0,
    0xbbaef1c3,
    0xc0006900,
    (uint8_t *)bes2300p_t1_patch_1_ins_data
};//for ble and sco:  OVERLAP_RESERVE2 in lld_check_conflict_with_ble

const uint32_t bes2300p_t1_patch_2_ins_data[] =
{
    0x781b4b05,
    0xd1032b09,
    0x4b032200,
    0x2704701a,
    0x60eb1be3,
    0xb8ecf627,
    0xc000693c,////prev_conflict_type_record
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch2 =
{
    2,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_t1_patch_2_ins_data),
    0x0002db0c,
    0xbf08f1d8,
    0xc0006920,
    (uint8_t *)bes2300p_t1_patch_2_ins_data
};//for ble and sco:  prev_conflict_type_record in ld_sco_avoid_ble_connect

const uint32_t bes2300p_t1_patch_3_ins_data[] =
{
    0xd1084298,
    0x3047f894,
    0x92004a0e,
    0x22024631,
    0xfd42f61f,
    0x4b0ce014,
    0xb143781b,
    0x3047f894,
    0x92004a08,
    0x22024631,
    0xfd36f61f,
    0xf894e008,
    0x4a043047,
    0x46319200,
    0xbf002204,
    0xfd2cf61f,
    0xbaa2f62b,
    0xa002fce5,
    0xc000699c,///ble_sco_need_move_flag
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch3 =
{
    3,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_t1_patch_3_ins_data),
    0x00031ec4,
    0xbd44f1d4,
    0xc0006950,
    (uint8_t *)bes2300p_t1_patch_3_ins_data
};//for ble and sco: ld_fm_prog_push LD_FRAME_TYPE_ESCO when ble_sco_need_move_flag in ld_sco_evt_start_cbk


const uint32_t bes2300p_t1_patch_4_ins_data[] =
{
    0x781b4b18,
    0x2200b113,
    0x701a4b16,
    0x781b4b18,
    0xd0232b00,
    0x4b162201,
    0xbf00701a,
    0xb1e84680,
    0x781b4b11,
    0xd1192b00,
    0x0304f1a8,
    0x2b01b2db,
    0xf1b8d903,
    0xd1130f09,
    0x2201e00c,
    0x701a4b09,
    0x0f09f1b8,
    0x2209d103,
    0x701a4b08,
    0x68eb2704,
    0xb8a6f627,
    0x4b032201,
    0xe7f4701a,
    0xb8c2f627,
    0xb8aff627,
    0xc000699c,///ble_sco_need_move_flag
    0xc000099f,
    0xc000693c,///prev_conflict_type_record
    0xc0006a18,///sco_find_sync_flag
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch4 =
{
    4,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_t1_patch_4_ins_data),
    0x0002db30,
    0xbf38f1d8,
    0xc00069a4,
    (uint8_t *)bes2300p_t1_patch_4_ins_data
};//for ble and sco: ble_sco_need_move_flag = 1,need_move=4 in ld_sco_avoid_ble_connect

const uint32_t bes2300p_t1_patch_5_ins_data[] =
{
    0xfbb24a02,
    0xf5f9f3f3,
    0xbf00bec6,
    0x9af8da00,
};


const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch5 =
{
    5,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_t1_patch_5_ins_data),
    0x00000650,
    0xb936f206,
    0xc00068c0,
    (uint8_t *)bes2300p_t1_patch_5_ins_data
};//lpclk


const uint32_t bes2300p_t1_patch_6_ins_data[] =
{
    0xd00d2b00,
    0xf8384b07,
    0xf4133003,
    0xd0024f00,
    0x4b052201,
    0xf899701a,
    0x2b0130b3,
    0xba5af629,
    0xbaccf629,
    0xd02111f8,
    0xc0006a18,////sco_find_sync_flag
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch6 =
{
    6,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_t1_patch_6_ins_data),
    0x0002feec,
    0xbd98f1d6,
    0xc0006a20,
    (uint8_t *)bes2300p_t1_patch_6_ins_data
};///for ble and sco: sco_find_sync_flag = 1 in ld_sco_frm_isr

const uint32_t bes2300p_t1_patch_7_ins_data[] =
{
    0xd10f2f03,
    0xfd16f642,
    0x4008f8d8,
    0x46216940,
    0xf6242220,
    0xb128fff5,
    0x0320f104,
    0x4378f023,
    0x3008f8c8,
    0xbf002000,
    0xbc7df625,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch7 =
{
    7,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_7_ins_data),
    0x0002c32c,
    0xbb90f1da,
    0xc0006A50,
    (uint8_t *)bes2300p_t1_patch_7_ins_data
};//ble avoid role switch

const uint32_t bes2300p_t1_patch_8_ins_data[] =
{
    0xf855b2e6,
    0xb1091b04,
    0xbc71f610,
    0xbc78f610,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch8 =
{
    8,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_8_ins_data),
    0x00017378,
    0xbb8af1ef,
    0xc0006A90,
    (uint8_t *)bes2300p_t1_patch_8_ins_data
};//lc_check_bt_device_conected

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch9 =
{
    9,
    BTDRV_PATCH_INACT,
    0,
    0x0002bb78,
    0x75a3230a,//priority =10
    0,
    NULL
};//unsniff acl priority in sco

const uint32_t bes2300p_t1_patch_10_ins_data[] =
{
    0xf8934b07,
    0x98053078,
    0xd1074298,
    0xd0052b03,
    0x681b4b04,
    0x2201b913,
    0x601a4b02,
    0xbc3ff628,
    0xc0006358,
    0xc0006AC8,
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch10 =
{
    10,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_10_ins_data),
    0x0002f338,
    0xbbb2f1d7,
    0xc0006AA0,
    (uint8_t *)bes2300p_t1_patch_10_ins_data
};//ibrt snoop success

const uint32_t bes2300p_t1_patch_11_ins_data[] =
{
    0xb149d208,
    0x681b4b05,
    0x2200b133,
    0x601a4b03,
    0xbce4f632,
    0xbcf2f632,
    0xbcfff632,
    0xc0006AC8,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch11 =
{
    11,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_11_ins_data),
    0x000394a8,
    0xbb12f1cd,
    0xc0006AD0,
    (uint8_t *)bes2300p_t1_patch_11_ins_data
};//ibrt snoop success 2

const uint32_t bes2300p_t1_patch_12_ins_data[] =
{
    0x46032102,
    0xf9b4f633,
    0xbf004618,
    0x22012105,
    0xfa28f61d,
    0xba4ff633,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch12 =
{
    12,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_12_ins_data),
    0x00039fa0,
    0xbda6f1cc,
    0xc0006AF0,
    (uint8_t *)bes2300p_t1_patch_12_ins_data
};// hack tws switch tx seqn error

const uint32_t bes2300p_t1_patch_13_ins_data[] =
{
    0xfc6ef631,
    0xfcbaf631,
    0xbf004620,
    0xfcc2f631,
    0xbfb2f62a,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch13 =
{
    13,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_13_ins_data),
    0x00031a7c,
    0xb844f1d5,
    0xc0006B08,
    (uint8_t *)bes2300p_t1_patch_13_ins_data
};// lmp filter enable

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch14 =
{
    14,
    BTDRV_PATCH_ACT,
    0,
    0x000388a8,
    0xbf00e001,
    0,
    NULL
};//open afh update

const uint32_t bes2300p_t1_patch_15_ins_data[] =
{
    0xfbd0f631,
    0x20004601,
    0xfbb8f633,
    0xba9ef618,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch15 =
{
    15,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_15_ins_data),
    0x0001f0bc,
    0xbd5af1e7,
    0xc0006B74,
    (uint8_t *)bes2300p_t1_patch_15_ins_data
};//ibrt slave stop sco

const uint32_t bes2300p_t1_patch_16_ins_data[] =
{
    0x60184b02,
    0xfc18f631,
    0xbe0af617,
    0xc0006b94,
    0x07ffffff,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch16 =
{
    16,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_16_ins_data),
    0x0001e7a0,
    0xb9f0f1e8,
    0xc0006B84,
    (uint8_t *)bes2300p_t1_patch_16_ins_data
};//slave record 1st sco instant
const uint32_t bes2300p_t1_patch_17_ins_data[] =
{
    0xf8934b0a,
    0x2b0231c2,
    0x4a09d009,
    0x3023f852,
    0x6b58b13b,
    0x70b4f500,
    0x4078f020,
    0x2000e002,
    0x2000e000,
    0x60184b03,
    0xbf004770,
    0xc0005d34,
    0xc0000980,
    0xc0006b94,
    0xffe2f7ff,
    0xbf004606,
    0xbe00f617,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch17 =
{
    17,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_17_ins_data),
    0x0001e7d8,
    0xb9faf1e8,
    0xc0006B98,
    (uint8_t *)bes2300p_t1_patch_17_ins_data
};//master record 1st sco instant


const uint32_t bes2300p_t1_patch_18_ins_data[] =
{
    0xb5704614,
    0x681b4b1d,
    0x4278f06f,
    0xd0344293,
    0x460e4605,
    0x78124a1a,
    0x2101bb32,
    0x70114a18,
    0x0478f103,
    0x4478f024,
    0x340ce00e,
    0x4478f024,
    0xf0231aa3,
    0x2b034378,
    0x1b13d906,
    0x4378f023,
    0xd9012b03,
    0xd3f0428c,
    0x75ab2314,
    0xfad8f5fc,
    0x3025f890,
    0x631cf5c3,
    0x826b3304,
    0xf0231da3,
    0x60ab4378,
    0xbd706034,
    0x3278680a,
    0x4278f022,
    0x010cf102,
    0x4178f021,
    0xbd70e7e4,
    0xc0006b94,
    0xc0006c60,
    0x00000000,
    0x49034620,
    0x200cf8da,
    0xffb6f7ff,
    0xbf9df624,
    0xc0000328,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch18 =
{
    18,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_18_ins_data),
    0x0002bad8,
    0xb8c4f1db,
    0xc0006BdC,
    (uint8_t *)bes2300p_t1_patch_18_ins_data
};//fix acl interval in sco

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch19 =
{
    19,
    BTDRV_PATCH_ACT,
    0,
    0x00039d78,
    0xd13b2902,
    0,
    NULL
};//ibrt switch op

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch20 =
{
    20,
    BTDRV_PATCH_ACT,
    0,
    0x00030240,
    0xe0022000,
    0,
    NULL
};//acl end assert

const uint32_t bes2300p_t1_patch_21_ins_data[] =
{
    0x781b4b03,
    0xd1012b01,
    0xbc5cf633,
    0xbc63f633,
    0xc0006358,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch21 =
{
    21,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_21_ins_data),
    0x0003a528,
    0xbba6f1cc,
    0xc0006C78,
    (uint8_t *)bes2300p_t1_patch_21_ins_data
};//snoop disconnect

// __SW_SEQ_FILTER__ - add software seqn filter to protect recive identical repeating packages
#if defined(IBRT)
#define __SW_SEQ_FILTER__
#endif
const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch22 =
{
    22,
#ifdef __SW_SEQ_FILTER__
    BTDRV_PATCH_INACT,
#else
    BTDRV_PATCH_INACT,
#endif
    sizeof(bes2300p_t1_patch_0a_ins_data),
    0x0002f334,
    0xba96f1d7, // jump a002f334 -> a0206864
    0xc0006810,
    (uint8_t *)bes2300p_t1_patch_0a_ins_data
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch23 =
{
    23,
#ifdef __SW_SEQ_FILTER__
    BTDRV_PATCH_INACT,
#else
    BTDRV_PATCH_INACT,
#endif
    sizeof(bes2300p_t1_patch_0b_ins_data),
    0x0001b5ac,
    0xb968f1eb, // jump a001b5ac -> a0206880
    0xc0006880,
    (uint8_t *)bes2300p_t1_patch_0b_ins_data
}; // lc_rsw_end_ind link_id_seq[link_id]=2


const uint32_t bes2300p_t1_patch_24_ins_data[] =
{
    0x0f01f1b8,
    0x380ebf0c,
    0xb280301c,
    0xbb30f632,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch24 =
{
    24,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_24_ins_data),
    0x000392d8,
    0xbcdaf1cd,
    0xc0006c90,
    (uint8_t *)bes2300p_t1_patch_24_ins_data
};//start snoop using bitoff

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch25 =
{
    25,
#if defined(IBRT)
    BTDRV_PATCH_ACT,
#else
    BTDRV_PATCH_INACT,
#endif
    0,
    0x0002e09c,
    0xf8a6230a,
    0,
    NULL,
};//ld_acl_start t_poll

const uint32_t bes2300p_t1_patch_26_ins_data[] =
{
    0x5040f898,
    0xd80d2d14,
    0xfbecf642,
    0x690068a6,
    0xb2ea4631,
    0xfeccf624,
    0xf106b120,
    0xf0230364,
    0x60a34378,
    0xfbdef642,
    0xbfb2f624,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch26 =
{
    26,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_t1_patch_26_ins_data),
    0x0002bc2c,
    0xb838f1db,
    0xc0006CA0,
    (uint8_t *)bes2300p_t1_patch_26_ins_data
};//mobile link avoid tws link when send profile

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch27 =
{
    27,
    BTDRV_PATCH_ACT,
    0,
    0x0003890c,
    0xbf00e008,
    0,
    NULL,
};// ld_sniffer_update_status: SNIFFER_EVT_SCO_CONNECTED

/////2300p t1 patch
static const uint32_t best2300p_t1_ins_patch_config[] =
{
    28,
    (uint32_t)&bes2300p_t1_ins_patch0,
    (uint32_t)&bes2300p_t1_ins_patch1,
    (uint32_t)&bes2300p_t1_ins_patch2,
    (uint32_t)&bes2300p_t1_ins_patch3,
    (uint32_t)&bes2300p_t1_ins_patch4,
    (uint32_t)&bes2300p_t1_ins_patch5,
    (uint32_t)&bes2300p_t1_ins_patch6,
    (uint32_t)&bes2300p_t1_ins_patch7,
    (uint32_t)&bes2300p_t1_ins_patch8,
    (uint32_t)&bes2300p_t1_ins_patch9,
    (uint32_t)&bes2300p_t1_ins_patch10,
    (uint32_t)&bes2300p_t1_ins_patch11,
    (uint32_t)&bes2300p_t1_ins_patch12,
    (uint32_t)&bes2300p_t1_ins_patch13,
    (uint32_t)&bes2300p_t1_ins_patch14,
    (uint32_t)&bes2300p_t1_ins_patch15,
    (uint32_t)&bes2300p_t1_ins_patch16,
    (uint32_t)&bes2300p_t1_ins_patch17,
    (uint32_t)&bes2300p_t1_ins_patch18,
    (uint32_t)&bes2300p_t1_ins_patch19,
    (uint32_t)&bes2300p_t1_ins_patch20,
    (uint32_t)&bes2300p_t1_ins_patch21,
    (uint32_t)&bes2300p_t1_ins_patch22,
    (uint32_t)&bes2300p_t1_ins_patch23,
    (uint32_t)&bes2300p_t1_ins_patch24,
    (uint32_t)&bes2300p_t1_ins_patch25,
    (uint32_t)&bes2300p_t1_ins_patch26,
    (uint32_t)&bes2300p_t1_ins_patch27,
};

void btdrv_ins_patch_write(BTDRV_PATCH_STRUCT *ins_patch_p)
{
    uint32_t remap_addr;
    /// uint8_t i=0;
    remap_addr =   ins_patch_p->patch_remap_address | 1;
    btdrv_write_memory(_32_Bit,(BTDRV_PATCH_INS_REMAP_ADDR_START + ins_patch_p->patch_index*4),
                       (uint8_t *)&ins_patch_p->patch_remap_value,4);
    if(ins_patch_p->patch_length != 0)  //have ram patch data
    {
        btdrv_memory_copy((uint32_t *)ins_patch_p->patch_start_address,(uint32_t *)ins_patch_p->patch_data,ins_patch_p->patch_length);
#if 0
        for( ; i<(ins_patch_p->patch_length)/128; i++)
        {
            btdrv_write_memory(_32_Bit,ins_patch_p->patch_start_address+i*128,
                               (ins_patch_p->patch_data+i*128),128);
        }

        btdrv_write_memory(_32_Bit,ins_patch_p->patch_start_address+i*128,ins_patch_p->patch_data+i*128,
                           ins_patch_p->patch_length%128);
#endif
    }

    btdrv_write_memory(_32_Bit,(BTDRV_PATCH_INS_COMP_ADDR_START + ins_patch_p->patch_index*4),
                       (uint8_t *)&remap_addr,4);

}

void btdrv_ins_patch_init(void)
{
    const BTDRV_PATCH_STRUCT *ins_patch_p;
    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0)
    {
        for(uint8_t i=0; i<best2300p_t1_ins_patch_config[0]; i++)
        {
            ins_patch_p = (BTDRV_PATCH_STRUCT *)best2300p_t1_ins_patch_config[i+1];
            if(ins_patch_p->patch_state ==BTDRV_PATCH_ACT)
                btdrv_ins_patch_write((BTDRV_PATCH_STRUCT *)best2300p_t1_ins_patch_config[i+1]);
        }
    }
}

///////////////////data  patch ..////////////////////////////////////
#if 0
0106 0109 010a 0114 0114 010a 0212 0105     ................
010a 0119 0119 0105 0105 0108 0214 0114     ................
0108 0114 010a 010a 0105 0105 0114 010a     ................
0b0f 0105 010a 0000
#endif

static const uint32_t bes2300p_dpatch0_data[] =
{
    0x01090106,
    0x0114010a,
    0x010a0114,
    0x01050212,
    0x0119010a,
    0x01050119,
    0x01080105,
    0x01140214,
    0x01140108,
    0x010a010a,
    0x01050105,
    0x0b0f0114,
    0x01050b0f,
    0x0000010a,
};

static const BTDRV_PATCH_STRUCT bes2300p_data_patch0 =
{
    0,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_dpatch0_data),
    0x00043318,
    0xc0000058,
    0xc0000058,
    (uint8_t *)&bes2300p_dpatch0_data
};

static const POSSIBLY_UNUSED uint32_t best2300p_data_patch_config[] =
{
    1,
    (uint32_t)&bes2300p_data_patch0,

};






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


void btdrv_data_patch_init(void)
{
#if 0

    const BTDRV_PATCH_STRUCT *data_patch_p;
    if(hal_get_chip_metal_id() > HAL_CHIP_METAL_ID_1 && hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_4)
    {
        for(uint8_t i=0; i<best2300_data_patch_config[0]; i++)
        {
            data_patch_p = (BTDRV_PATCH_STRUCT *)best2300_data_patch_config[i+1];
            if(data_patch_p->patch_state == BTDRV_PATCH_ACT)
                btdrv_data_patch_write((BTDRV_PATCH_STRUCT *)best2300_data_patch_config[i+1]);
        }
    }
#endif
}


//////////////////////////////patch enable////////////////////////


void btdrv_patch_en(uint8_t en)
{
    uint32_t value[2];

    //set patch enable
    value[0] = 0x2f02 | en;
    //set patch remap address  to 0xc0000100
    value[1] = 0x20000100;
    btdrv_write_memory(_32_Bit,BTDRV_PATCH_EN_REG,(uint8_t *)&value,8);
}



const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_0 =
{
    0,
    BTDRV_PATCH_ACT,
    0,
    0x0002f440,
    0x789abf00,
    0,
    NULL
};///test mode: remove rxllid's judging condition in ld_acl_rx

const uint32_t bes2300p_t1_ins_patch_data_testmode_1[] =
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
    0xbe8ef627,
    0xc000099c,////loopback_length
    0xc000097d,////loopback_llid
    0xc0006964,////rxseq_flag
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_1 =
{
    1,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_ins_patch_data_testmode_1),
    0x0002e60c,
    0xb978f1d8,
    0xc0006900,
    (uint8_t*)bes2300p_t1_ins_patch_data_testmode_1
};///test mode: ld_acl_tx_prog


const uint32_t bes2300p_t1_ins_patch_data_testmode_2[] =
{
    0xfbdcf5fc,
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
    0xbc32f628,
    0xbc2ef628,
    0xc0005d34,
    0xd02115f2,
    0xc000097d,////loopback_llid
    0xc000099c,////loopback_length
    0xc0006a8c,////rxllid_flag
    0xc0006a90,////rxlength_flag
    0xc0006a94,////unack_seqerr_flag
    0x00000000,
    0x00000000,
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_2 =
{
    2,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_ins_patch_data_testmode_2),
    0x0002f2c8,
    0xbbaaf1d7,
    0xc0006a20,
    (uint8_t*)bes2300p_t1_ins_patch_data_testmode_2
};///test mode: skip rx seq err in ld_acl_rx


const uint32_t bes2300p_t1_ins_patch_data_testmode_3[] =
{
    0xf015d002,
    0xd0010f24,
    0xbc1af628,
    0xbcfcf628,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_3 =
{
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_ins_patch_data_testmode_3),
    0x0002f2d4,
    0xbbe0f1d7,
    0xc0006a98,
    (uint8_t*)bes2300p_t1_ins_patch_data_testmode_3
};///test mode: skip crc err in ld_acl_rx


const uint32_t bes2300p_t1_ins_patch_data_testmode_4[] =
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
    0xbcbaf628,
    0xc0005d34,
    0xd02115f2,
    0xc0006964,////rxseq_flag
    0xc0006a94,////unack_seqerr_flag
    0xc0006a90,////rxlength_flag
    0xc000099c,////loopback_length
    0xc0006a8c,////rxllid_flag
    0xc000097d,////loopback_llid
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_4 =
{
    4,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_ins_patch_data_testmode_4),
    0x0002f464,
    0xbb2af1d7,
    0xc0006abc,
    (uint8_t*)bes2300p_t1_ins_patch_data_testmode_4
};///test mode: ld_acl_rx


static const uint32_t ins_patch_2300p_t1_config_testmode[] =
{
    5,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_0,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_1,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_2,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_3,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_4,
};



void btdrv_ins_patch_test_init(void)
{

    const BTDRV_PATCH_STRUCT *ins_patch_p;

    btdrv_patch_en(0);

    for(uint8_t i=0; i<56; i++)
    {
        btdrv_ins_patch_disable(i);
    }

    if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0 )
    {
        for(uint8_t i=0; i<ins_patch_2300p_t1_config_testmode[0]; i++)
        {
            ins_patch_p = (BTDRV_PATCH_STRUCT *)ins_patch_2300p_t1_config_testmode[i+1];
            if(ins_patch_p->patch_state ==BTDRV_PATCH_ACT)
                btdrv_ins_patch_write((BTDRV_PATCH_STRUCT *)ins_patch_2300p_t1_config_testmode[i+1]);
        }

    }
    btdrv_patch_en(1);
}

void btdrv_dynamic_patch_moble_disconnect_reason_hacker(uint16_t hciHandle)
{
    uint32_t lc_ptr=0;
    uint32_t disconnect_reason_address = 0;
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
    {
        lc_ptr = *(uint32_t *)(0xc0005c70+(hciHandle-0x80)*4);
    }
    //TRACE("lc_ptr %x, disconnect_reason_addr %x",lc_ptr,lc_ptr+66);
    if(lc_ptr == 0)
    {
        return;
    }
    else
    {
        disconnect_reason_address = (uint32_t)(lc_ptr+66);
        TRACE("%s:hdl=%x reason=%d",__func__,hciHandle,*(uint8_t *)(disconnect_reason_address));
        *(uint8_t *)(disconnect_reason_address) = 0x0;
        TRACE("After op,reason=%d",*(uint8_t *)(disconnect_reason_address));
        return;
    }
}

void btdrv_current_packet_type_hacker(uint16_t hciHandle,uint8_t rate)
{
    uint32_t lc_ptr=0;
    uint32_t currentPacketTypeAddr = 0;
    uint32_t acl_par_ptr = 0;
    uint32_t packet_type_addr = 0;
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0)
    {
        lc_ptr = *(uint32_t *)(0xc0005c70+(hciHandle-0x80)*4);
        acl_par_ptr = *(uint32_t *)(0xc000098c+(hciHandle-0x80)*4);
    }
    if(lc_ptr == 0)
    {
        return;
    }
    else
    {
        currentPacketTypeAddr = (uint32_t)(lc_ptr+40);
        packet_type_addr = (uint32_t)(acl_par_ptr+176);
        TRACE("lc_ptr %x, acl_par_ptr %x",lc_ptr,acl_par_ptr);
        TRACE("%s:hdl=%x currentPacketType %x packet_types %x",__func__,hciHandle, *(uint16_t *)currentPacketTypeAddr,*(uint16_t *)(packet_type_addr));
        if(rate == BT_TX_3M)
        {
            *(uint16_t *)currentPacketTypeAddr = 0xdd1a;
            *(uint16_t *)(packet_type_addr) = 0x553f;
        }
        else if(rate == BT_TX_2M)
        {
            *(uint16_t *)currentPacketTypeAddr = 0xee1c;
            *(uint16_t *)(packet_type_addr) = 0x2a3f;
        }
        else if(rate == (BT_TX_2M | BT_TX_3M))
        {
            *(uint16_t *)currentPacketTypeAddr = 0xcc18;
            *(uint16_t *)(packet_type_addr) = 0x7f3f;
        }
        TRACE("After op,currentPacketType=%x packet_types %x",*(uint16_t *)currentPacketTypeAddr,*(uint16_t *)(packet_type_addr));
    }
}


void btdrv_preferre_rate_send(uint16_t conhdl,uint8_t rate)
{
}

void btdrv_preferre_rate_clear(void)
{
}
void btdrv_dynamic_patch_sco_status_clear(void)
{
}


void btdrv_seq_bak_mode(uint8_t mode,uint8_t linkid)
{
#ifdef __SW_SEQ_FILTER__
    if(hal_get_chip_metal_id() > HAL_CHIP_METAL_ID_1 && hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_4)
    {
        uint32_t seq_bak_ptr = 0xc0006c8c;
        TRACE("btdrv_seq_bak_mode mode=%x,linkid = %x",mode,linkid);
        uint32_t val = *(uint32_t *)seq_bak_ptr;
        val &= ~(0xff<<(linkid*8));
        if(mode ==1)//en
        {
            val |=(0x2<<linkid*8);
        }
        else if(mode ==0)
        {
            val =0xffffffff;
        }
        *(uint32_t *)seq_bak_ptr = val;

        TRACE("val=%x",val);
    }
#endif
}



void bt_drv_patch_cvsd_check_check(void)
{

}



