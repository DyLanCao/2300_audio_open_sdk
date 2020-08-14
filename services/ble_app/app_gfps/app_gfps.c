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
/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_GFPS)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "cmsis_os.h"
#include "app.h"                     // Application Manager Definitions
#include "apps.h"
#include "app_gfps.h"                 // Device Information Service Application Definitions
#include "gfps_provider_task.h"               // Device Information Profile Functions
#include "prf_types.h"               // Profile Common Types Definitions
#include "gapm_task.h"               // GAP Manager Task API
#include <string.h>
#include "gfps_crypto.h"
#include "gfps_provider.h"
#include "gfps_provider_errors.h"
#include "gap.h"
#define _FILE_TAG_ "appgfps"
#include "color_log.h"
#include "app_ble_mode_switch.h"
#include "nvrecord.h"
#include "nvrecord_fp_account_key.h"
#include "gfps_crypto.h"
#include "gapm.h"

#define USE_BLE_ADDR_AS_SALT        0
#define USE_RANDOM_NUM_AS_SALT      1
#define GFPS_ACCOUNTKEY_SALT_TYPE   USE_BLE_ADDR_AS_SALT
void AES128_ECB_decrypt(uint8_t* input, const uint8_t* key, uint8_t *output);

struct app_gfps_env_tag app_gfps_env;

static void app_gfps_init_env(void)
{
    memset((uint8_t *)&(app_gfps_env.connectionIndex), 0, sizeof(struct app_gfps_env_tag));
	app_gfps_env.connectionIndex = BLE_INVALID_CONNECTION_INDEX;
}

static void big_little_switch(const uint8_t* in, uint8_t* out,uint8_t len)
{
    if(len<1) return;
    for(int i=0;i<len;i++)
    {
        out[i] = in[len-i-1];
    }
    return;
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
extern uint8_t bt_addr[6] ;

void app_gfps_connected_evt_handler(uint8_t conidx)
{
    app_gfps_env.connectionIndex = conidx;
    TRACE("local LE addr: ");

	bd_addr_t* pBdAddr = gapm_get_connected_bdaddr(conidx);
    big_little_switch(pBdAddr->addr,&app_gfps_env.local_le_addr.addr[0],6);
    big_little_switch((&bt_addr[0]),&app_gfps_env.local_bt_addr.addr[0],6);
    DUMP8("0x%02x ",pBdAddr->addr, 6);
    TRACE("local bt addr: ");
    DUMP8("0x%02x ",&bt_addr[0], 6);
}

void app_gfps_disconnected_evt_handler(uint8_t conidx)
{
	if (conidx == app_gfps_env.connectionIndex)
	{
	    //recover classic bt iocap
	    if (app_gfps_env.bt_set_iocap != NULL)
	    {
	    	app_gfps_env.bt_set_iocap(app_gfps_env.bt_iocap);
	    }
	    
	    app_gfps_env.isKeyBasedPairingNotificationEnabled = false;
	    app_gfps_env.isPassKeyNotificationEnabled = false;

		app_gfps_env.connectionIndex = BLE_INVALID_CONNECTION_INDEX;
	}
}

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static int gfpsp_value_req_ind_handler(ke_msg_id_t const msgid,
                                          struct gfpsp_value_req_ind const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Initialize length
    uint8_t len = 0;
    // Pointer to the data
    uint8_t *data = NULL;
    LOG_V("val %d",param->value);
    // Check requested value
    switch (param->value)
    {
        case GFPSP_MANUFACTURER_NAME_CHAR:
        case GFPSP_MODEL_NB_STR_CHAR:
        case GFPSP_SYSTEM_ID_CHAR:
        case GFPSP_PNP_ID_CHAR:
        case GFPSP_SERIAL_NB_STR_CHAR:
        case GFPSP_HARD_REV_STR_CHAR:
        case GFPSP_FIRM_REV_STR_CHAR:
        case GFPSP_SW_REV_STR_CHAR:
        case GFPSP_IEEE_CHAR:
        {
            // Set information
            len = APP_GFPS_IEEE_LEN;
            data = (uint8_t *)APP_GFPS_IEEE;
        } break;

        default:
            ASSERT_ERR(0);
            break;
    }

    // Allocate confirmation to send the value
    struct gfpsp_value_cfm *cfm_value = KE_MSG_ALLOC_DYN(GFPSP_VALUE_CFM,
            src_id, dest_id,
            gfpsp_value_cfm,
            len);

    // Set parameters
    cfm_value->value = param->value;
    cfm_value->length = len;
    if (len)
    {
        // Copy data
        memcpy(&cfm_value->data[0], data, len);
    }
    // Send message
    ke_msg_send(cfm_value);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
//MSB->LSB
const uint8_t bes_demo_Public_anti_spoofing_key[64] = {
    0x40,0x96,0x2B,0x1E,0x4D,0x45,0x2A,0x79,0xBA,0xFE,
    0x66,0xA0,0xAA,0x9A,0x9E,0x06,0x5F,0xEC,0x66,0xB6,
    0x8B,0xE3,0x30,0x30,0xE9,0x8F,0x3A,0x00,0xC0,0x3C,
    0x0E,0xC8,0xC0,0x6A,0xD5,0xCB,0x30,0x04,0xE7,0xDE,
    0x80,0xC6,0x44,0xF5,0xD3,0x13,0x7B,0x00,0x27,0x40,
    0xBC,0x8B,0x98,0xF2,0xE1,0x13,0x9E,0x6C,0x82,0x1B,
    0xDD,0x14,0x0F,0x4F
};

//MSB->LSB
const uint8_t bes_demo_private_anti_spoofing_key[32]=
{
    0xFA,0x90,0x30,0xC0,0xD7,0x43,0xF6,0xE8,0xC9,0xE5,
    0xBD,0xDF,0xCE,0xB1,0x82,0xE7,0xAF,0x65,0x8F,0x8A,
    0xB0,0xA1,0x08,0x26,0xBA,0x0E,0xBD,0x0D,0x16,0xE3,
    0xF5,0x32
};

extern void fast_pair_enter_pairing_mode_handler(void);

void app_gfps_init(void)
{
    app_gfps_init_env();
    app_gfps_env.enter_pairing_mode =NULL;
    app_gfps_env.bt_set_iocap =NULL;
    gfps_crypto_init();
    gfps_crypto_set_p256_key(bes_demo_Public_anti_spoofing_key,bes_demo_private_anti_spoofing_key);

    app_gfps_set_bt_access_mode(fast_pair_enter_pairing_mode_handler);
    app_gfps_set_io_cap((gfps_bt_io_cap_set)btif_sec_set_io_capabilities);
}

void app_gfps_set_bt_access_mode(gfps_enter_pairing_mode cb)
{
    app_gfps_env.enter_pairing_mode = cb;
}

void app_gfps_set_io_cap(gfps_bt_io_cap_set cb)
{
    app_gfps_env.bt_set_iocap = cb;
}

void app_gfps_add_gfps(void)
{
    struct gfpsp_db_cfg* db_cfg;
    // Allocate the DISS_CREATE_DB_REQ
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, sizeof(struct gfpsp_db_cfg));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX>1
    req->sec_lvl = PERM(SVC_AUTH, ENABLE)|PERM(SVC_MI, ENABLE);
#else
    req->sec_lvl = PERM(SVC_AUTH, ENABLE);
#endif

    req->prf_task_id = TASK_ID_GFPSP;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    // Set parameters
    db_cfg = (struct gfpsp_db_cfg* ) req->param;
    db_cfg->features = APP_GFPS_FEATURES;

    // Send the message
    ke_msg_send(req);
}

void app_gfps_send_keybase_pairing_via_notification(uint8_t* ptrData, uint32_t length)
{
    struct gfpsp_send_data_req_t * req = KE_MSG_ALLOC_DYN(GFPSP_KEY_BASSED_PAIRING_WRITE_NOTIFY,
                                                KE_BUILD_ID(prf_get_task_from_id(TASK_ID_GFPSP),
                                                app_gfps_env.connectionIndex),
                                                TASK_APP,
                                                gfpsp_send_data_req_t,
                                                length);
    req->connecionIndex = app_gfps_env.connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);
}

int app_gfps_send_passkey_via_notification(uint8_t* ptrData, uint32_t length)
{
    struct gfpsp_send_data_req_t * req = KE_MSG_ALLOC_DYN(GFPSP_KEY_PASS_KEY_WRITE_NOTIFY,
                                                KE_BUILD_ID(prf_get_task_from_id(TASK_ID_GFPSP),
                                                app_gfps_env.connectionIndex),
                                                TASK_APP,
                                                gfpsp_send_data_req_t,
                                                length);
    req->connecionIndex = app_gfps_env.connectionIndex;
    req->length = length;
    memcpy(req->value, ptrData, length);

    ke_msg_send(req);

    return (KE_MSG_CONSUMED);
}

void app_gfps_enter_connectable_mode_req_handler(uint8_t* response)
{
#ifdef __BT_ONE_BRING_TWO__
    app_gfps_send_keybase_pairing_via_notification(response, GFPSP_ENCRYPTED_RSP_LEN);
#else
    if (btif_me_get_activeCons() > 0)
    {
        memcpy(app_gfps_env.pendingLastResponse, response, GFPSP_ENCRYPTED_RSP_LEN);
        app_gfps_env.isLastResponsePending = true;
        app_disconnect_all_bt_connections();
    }
    else
    {       
        app_gfps_env.isLastResponsePending = false;
        app_gfps_send_keybase_pairing_via_notification(response, GFPSP_ENCRYPTED_RSP_LEN);
        TRACE("wait for pair req maybe classic or ble");
        if (app_gfps_env.enter_pairing_mode != NULL)
        {
              app_gfps_env.enter_pairing_mode();
        }

        if (app_gfps_env.bt_set_iocap!=NULL)
        {
              TRACE("SET IOC");
             app_gfps_env.bt_iocap= app_gfps_env.bt_set_iocap(1);//IO_display_yesno
        }
    }
#endif
}

uint8_t* app_gfps_get_last_response(void)
{
    return app_gfps_env.pendingLastResponse;
}

bool app_gfps_is_last_response_pending(void)
{
    return app_gfps_env.isLastResponsePending;
}

static void app_gfps_handle_decrypted_keybase_pairing_request(gfpsp_req_resp* raw_req,
	uint8_t* out_key)
{
	gfpsp_encrypted_resp  en_rsp;
	gfpsp_raw_resp  raw_rsp;  
	memcpy(app_gfps_env.keybase_pair_key, out_key, 16);   
	memcpy(&app_gfps_env.seeker_bt_addr.addr[0] ,&raw_req->rx_tx.raw_req.seeker_addr[0],6);   
	if(raw_req->rx_tx.raw_req.flags_discoverability ==RAW_REQ_FLAGS_DISCOVERABILITY_BIT0_EN)
	{
		LOG_E("TODO discoverable 10S");
		//TODO start a timer keep discoverable 10S... 
		//TODO make sure there is no ble ADV with the MODEL ID data
	}
	raw_rsp.message_type = 0x01;// Key-based Pairing Response
	memcpy(raw_rsp.provider_addr, app_gfps_env.local_bt_addr.addr, 6);

	DUMP8(" 0x%02x",raw_rsp.provider_addr, 6);

	for (uint8_t index = 0;index < 9;index++)
	{
		raw_rsp.salt[index] = (uint8_t)rand();
	}

	gfps_crypto_encrypt((const uint8_t *)(&raw_rsp.message_type), sizeof(raw_rsp),app_gfps_env.keybase_pair_key,en_rsp.uint128_array);

	if(raw_req->rx_tx.raw_req.flags_bonding_addr == RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_EN)
	{
		LOG_E("try connect to remote BR/EDR addr");
		// TODO:
		
		app_gfps_send_keybase_pairing_via_notification((uint8_t *)en_rsp.uint128_array, sizeof(en_rsp));       
	}
	else if(raw_req->rx_tx.raw_req.flags_bonding_addr == RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_DIS)
	{
		app_gfps_enter_connectable_mode_req_handler((uint8_t *)en_rsp.uint128_array);
	}else{
		app_gfps_send_keybase_pairing_via_notification((uint8_t *)en_rsp.uint128_array, sizeof(en_rsp));       
	}
}

static bool app_gfps_decrypt_keybase_pairing_request(uint8_t* pairing_req, uint8_t* output)
{
	uint8_t keyCount = nv_record_fp_account_key_count();
	if (0 == keyCount)
	{
		return false;	
	}

	gfpsp_req_resp raw_req;
	uint8_t accountKey[FP_ACCOUNT_KEY_SIZE];
	for (uint8_t keyIndex = 0;keyIndex < keyCount;keyIndex++)
	{
		nv_record_fp_account_key_get_by_index(keyIndex, accountKey);

		AES128_ECB_decrypt(pairing_req, (const uint8_t *)accountKey, (uint8_t *)&raw_req);
		TRACE("Decrypted keybase pairing req result:");
		DUMP8("0x%02x ", (uint8_t *)&raw_req, 16);
		
		if ((0x00 == raw_req.rx_tx.raw_req.message_type) &&
			((memcmp(raw_req.rx_tx.raw_req.provider_addr, 
				app_gfps_env.local_bt_addr.addr , 6)==0) ||
		 	(memcmp(raw_req.rx_tx.raw_req.provider_addr, 
		 		app_gfps_env.local_le_addr.addr , 6)==0)))
		{
			memcpy(output, accountKey, FP_ACCOUNT_KEY_SIZE);
			app_gfps_handle_decrypted_keybase_pairing_request(&raw_req, accountKey);
			return true;
		}
	}

	return false;
}

int app_gfps_write_key_based_pairing_ind_hander(ke_msg_id_t const msgid,
                                          struct gfpsp_write_ind_t const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{

    gfpsp_Key_based_Pairing_req en_req;
    gfpsp_req_resp* ptr_raw_req;
      
    en_req.en_req = (gfpsp_encrypted_req_uint128 *)&(param->data[0]);
    en_req.pub_key = (gfpsp_64B_public_key *)&(param->data[16]);
    uint8_t out_key[16] = {0};
    uint8_t decryptdata[16] = {0};
    
    LOG_E("length = %d value = 0x%x  0x%x",param->length,param->data[0],param->data[1]);
    DUMP8("0X%02X, ", param->data, 80);

    if (param->length == GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITH_PUBLIC_KEY)
    {   
        memset(app_gfps_env.keybase_pair_key, 0, 6);
        uint32_t gfps_state = gfps_crypto_get_secret_decrypt((const uint8_t*)en_req.en_req,
                                            (const uint8_t*)en_req.pub_key,
                                            out_key,
                                            decryptdata);
        if (gfps_state == GFPS_SUCCESS)
        {
           ptr_raw_req = (gfpsp_req_resp *)decryptdata;
           LOG_D("raw req provider's addr:");
           DUMP8("0x%02x", ptr_raw_req->rx_tx.raw_req.provider_addr, 6);
           LOG_D("raw req seeker's addr:");
           DUMP8("0x%02x", ptr_raw_req->rx_tx.raw_req.seeker_addr, 6);
           
           if ((0x00 == ptr_raw_req->rx_tx.raw_req.message_type) &&
			((memcmp(ptr_raw_req->rx_tx.raw_req.provider_addr, 
				app_gfps_env.local_bt_addr.addr , 6)==0) ||
		 	(memcmp(ptr_raw_req->rx_tx.raw_req.provider_addr, 
		 		app_gfps_env.local_le_addr.addr , 6)==0)))
		 	{
		   		app_gfps_handle_decrypted_keybase_pairing_request(ptr_raw_req, out_key);
			}else{
                LOG_E("decrypt false..ingore");
            }

        }else{
            LOG_E("error = %x",gfps_state);
        }
        
    }
	else if (param->length == GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITHOUT_PUBLIC_KEY){
        
        bool isDecryptedSuccessful = 
			app_gfps_decrypt_keybase_pairing_request((uint8_t*)en_req.en_req, out_key);
		TRACE("Decrypt keybase pairing req without public key result: %d", isDecryptedSuccessful);
	}
	else
	{
        LOG_E("who you are??");
    }
    return (KE_MSG_CONSUMED);
}

int app_gfps_write_passkey_ind_hander(ke_msg_id_t const msgid,
                                          struct gfpsp_write_ind_t const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
	gfpsp_raw_pass_key_resp  raw_rsp;    
	gfpsp_encrypted_resp  en_rsp;
	uint8_t decryptdata[16] = {0};
	LOG_E("length = %d value = 0x",param->length);
	DUMP8("%02X, ", param->data, 16);
	gfps_crypto_decrypt(param->data, 16, app_gfps_env.keybase_pair_key, decryptdata);
	LOG_E("decrypt data =0x");
	TRACE("===============================");
	DUMP8("%02X", decryptdata, 16);
	TRACE("===============================");

	LOG_D("pass key = 1-3 bytes");
        
    raw_rsp.message_type = 0x03;//Provider's passkey
    raw_rsp.passkey[0] = decryptdata[1];
    raw_rsp.passkey[1] = decryptdata[2];
    raw_rsp.passkey[2] = decryptdata[3];
    raw_rsp.reserved[0] = 0x38;     //my magic num  temp test
    raw_rsp.reserved[1] = 0x30;
    raw_rsp.reserved[2] = 0x23;
    raw_rsp.reserved[3] = 0x30;
    raw_rsp.reserved[4] = 0x06;
    raw_rsp.reserved[5] = 0x10;
    raw_rsp.reserved[6] = 0x05;
    raw_rsp.reserved[7] = 0x13;
    raw_rsp.reserved[8] = 0x06;
    raw_rsp.reserved[9] = 0x12;
    raw_rsp.reserved[10] = 0x12;
    raw_rsp.reserved[11] = 0x01;
    gfps_crypto_encrypt((const uint8_t *)(&raw_rsp.message_type),sizeof(raw_rsp),app_gfps_env.keybase_pair_key,en_rsp.uint128_array);
    app_gfps_send_passkey_via_notification((uint8_t *)en_rsp.uint128_array,sizeof(en_rsp));
    
    return (KE_MSG_CONSUMED);
}

int app_gfps_write_accountkey_ind_hander(ke_msg_id_t const msgid,
                                          struct gfpsp_write_ind_t const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
	NV_FP_ACCOUNT_KEY_ENTRY_T accountkey;

	LOG_E("length = %d value = 0x",param->length);
	DUMP8("%02X, ", param->data, FP_ACCOUNT_KEY_SIZE);
	gfps_crypto_decrypt(param->data, FP_ACCOUNT_KEY_SIZE, 
		app_gfps_env.keybase_pair_key, accountkey.key);
	LOG_E("decrypt account key:");
	TRACE("===============================");
	DUMP8("%02X", accountkey.key, FP_ACCOUNT_KEY_SIZE);
	TRACE("===============================");

    nv_record_fp_account_key_add(&accountkey);
    return (KE_MSG_CONSUMED);
}

uint8_t app_gfps_generate_accountkey_data(uint8_t* outputData)
{
	uint8_t keyCount = nv_record_fp_account_key_count();
	if (0 == keyCount)
	{
		outputData[0] = 0;
		outputData[1] = 0;
		return 2;
	}

	uint8_t accountKeyData[32];
	accountKeyData[0] = 0;

	uint8_t accountKeyDataLen = 2;
	uint8_t hash256Result[32];

	uint8_t sizeOfFilter = (((uint8_t)((float)1.2*keyCount)) + 3);
	uint8_t FArray[2*FP_ACCOUNT_KEY_RECORD_NUM + 3];
	memset(FArray, 0, sizeof(FArray));

#if GFPS_ACCOUNTKEY_SALT_TYPE==USE_BLE_ADDR_AS_SALT
	uint8_t VArray[FP_ACCOUNT_KEY_SIZE + 6];
#else
	uint8_t VArray[FP_ACCOUNT_KEY_SIZE + 1];
	uint8_t randomSalt = (uint8_t)rand();
#endif
	
	uint8_t index;
	
	for (uint8_t keyIndex = 0;keyIndex < keyCount;keyIndex++)
	{
		nv_record_fp_account_key_get_by_index(keyIndex, VArray);

#if GFPS_ACCOUNTKEY_SALT_TYPE==USE_BLE_ADDR_AS_SALT
        uint8_t* currentBleAddr = appm_get_current_ble_addr();
        for (index = 0;index < 6;index++)
        {
            VArray[FP_ACCOUNT_KEY_SIZE + index] = currentBleAddr[5-index]; 
        }
#else
        VArray[FP_ACCOUNT_KEY_SIZE] = randomSalt;
#endif

		TRACE("To hash256 on:");
		DUMP8("0x%02x ", VArray, sizeof(VArray));
		
		gfps_SHA256_hash(VArray, sizeof(VArray), hash256Result);

		// K = Xi % (s * 8)
		// F[K/8] = F[K/8] | (1 << (K % 8))
		uint32_t pX[8];
		for (index = 0;index < 8;index++)
		{
			pX[index] = (((uint32_t)(hash256Result[index*4])) << 24) |
				(((uint32_t)(hash256Result[index*4 + 1])) << 16) |
				(((uint32_t)(hash256Result[index*4 + 2])) << 8) |
				(((uint32_t)(hash256Result[index*4 + 3])) << 0);
		}

		for (index = 0;index < 8;index++)
		{
			uint32_t K = pX[index]%(sizeOfFilter*8);
			FArray[K/8] = FArray[K/8]|(1 << (K%8));
		}
	}

	memcpy(&accountKeyData[2], FArray, sizeOfFilter);

	accountKeyDataLen += sizeOfFilter;

	accountKeyData[1] = sizeOfFilter<<4;

#if GFPS_ACCOUNTKEY_SALT_TYPE==USE_RANDOM_NUM_AS_SALT
	accountKeyData[2+sizeOfFilter] = 0x11;
	accountKeyData[2+sizeOfFilter+1] = randomSalt;

	accountKeyDataLen += 2;
#endif

	TRACE("Generated accountkey data len:", accountKeyDataLen);
	DUMP8("0x%02x ", accountKeyData, accountKeyDataLen);

	memcpy(outputData, accountKeyData, accountKeyDataLen);
	return accountKeyDataLen;
}

static void gfpsp_update_connection_state(uint8_t conidx)
{
	if (BLE_INVALID_CONNECTION_INDEX == app_gfps_env.connectionIndex)
	{
		app_gfps_connected_evt_handler(conidx);
	}
}

static int app_gfpsp_key_based_pairing_ntf_handler(ke_msg_id_t const msgid,
                              struct app_gfps_key_based_notif_config_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    app_gfps_env.isKeyBasedPairingNotificationEnabled = param->isNotificationEnabled;
	if (app_gfps_env.isKeyBasedPairingNotificationEnabled)
	{
		uint8_t conidx = KE_IDX_GET(src_id);
		gfpsp_update_connection_state(conidx);
	}

	return (KE_MSG_CONSUMED);
}

static int app_gfpsp_pass_key_ntf_handler(ke_msg_id_t const msgid,
                              struct app_gfps_pass_key_notif_config_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    app_gfps_env.isPassKeyNotificationEnabled = param->isNotificationEnabled;
	if (app_gfps_env.isPassKeyNotificationEnabled)
	{
		uint8_t conidx = KE_IDX_GET(src_id);
		gfpsp_update_connection_state(conidx);
	}

	return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_gfps_msg_handler_list[] =
{
    {GFPSP_VALUE_REQ_IND,     			(ke_msg_func_t)gfpsp_value_req_ind_handler},
    {GFPSP_KEY_BASSED_PAIRING_WRITE_IND,(ke_msg_func_t)app_gfps_write_key_based_pairing_ind_hander},
    {GFPSP_KEY_PASS_KEY_WRITE_IND, 		(ke_msg_func_t)app_gfps_write_passkey_ind_hander},
	{GFPSP_KEY_ACCOUNT_KEY_WRITE_IND, 	(ke_msg_func_t)app_gfps_write_accountkey_ind_hander},
    {GFPSP_KEY_BASSED_PAIRING_NTF_CFG, 	(ke_msg_func_t)app_gfpsp_key_based_pairing_ntf_handler},
	{GFPSP_KEY_PASS_KEY_NTF_CFG,		(ke_msg_func_t)app_gfpsp_pass_key_ntf_handler},
	
};

const struct ke_state_handler app_gfps_table_handler =
    {&app_gfps_msg_handler_list[0], (sizeof(app_gfps_msg_handler_list)/sizeof(struct ke_msg_handler))};

#endif //BLE_APP_DIS

/// @} APP
