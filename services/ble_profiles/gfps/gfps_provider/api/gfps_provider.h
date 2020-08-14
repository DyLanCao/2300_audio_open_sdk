#ifndef GFPSP_H_
#define GFPSP_H_

/**
 ****************************************************************************************
 * @addtogroup GFPSP google fast pair s
 * @ingroup DIS
 * @brief Device Information Service Server
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_GFPS_PROVIDER)
#include "prf_types.h"
#include "prf.h"


/*
 * DEFINES
 ****************************************************************************************
 */


#define GFPSP_IDX_MAX        (BLE_CONNECTION_MAX)

#define GFPSP_ENCRYPTED_RSP_LEN     16

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the GFPSP task
enum
{
    /// Idle state
    GFPSP_IDLE,
    /// Busy state
    GFPSP_BUSY,
    /// Number of defined states.
    GFPSP_STATE_MAX
};

#if 0
/// GFPSP Attributes database handle list
enum gfpsp_att_db_handles
{
    GFPSP_IDX_SVC,

    GFPSP_IDX_MANUFACTURER_NAME_CHAR,
    GFPSP_IDX_MANUFACTURER_NAME_VAL,

    GFPSP_IDX_MODEL_NB_STR_CHAR,
    GFPSP_IDX_MODEL_NB_STR_VAL,

    GFPSP_IDX_SERIAL_NB_STR_CHAR,
    GFPSP_IDX_SERIAL_NB_STR_VAL,

    GFPSP_IDX_HARD_REV_STR_CHAR,
    GFPSP_IDX_HARD_REV_STR_VAL,

    GFPSP_IDX_FIRM_REV_STR_CHAR,
    GFPSP_IDX_FIRM_REV_STR_VAL,

    GFPSP_IDX_SW_REV_STR_CHAR,
    GFPSP_IDX_SW_REV_STR_VAL,

    GFPSP_IDX_SYSTEM_ID_CHAR,
    GFPSP_IDX_SYSTEM_ID_VAL,

    GFPSP_IDX_IEEE_CHAR,
    GFPSP_IDX_IEEE_VAL,

    GFPSP_IDX_PNP_ID_CHAR,
    GFPSP_IDX_PNP_ID_VAL,

    GFPSP_IDX_NB,
};
#endif
/// GFPSP Attributes database handle list 
enum gfpsp_att_db_handles
{
    GFPSP_IDX_SVC,

    GFPSP_IDX_KEY_BASED_PAIRING_CHAR,
    GFPSP_IDX_KEY_BASED_PAIRING_VAL,
    GFPSP_IDX_KEY_BASED_PAIRING_NTF_CFG,
    
    GFPSP_IDX_PASSKEY_CHAR,
    GFPSP_IDX_PASSKEY_VAL,
    GFPSP_IDX_PASSKEY_NTF_CFG,
    
    GFPSP_IDX_ACCOUNT_KEY_CHAR,
    GFPSP_IDX_ACCOUNT_KEY_VAL,
    GFPSP_IDX_ACCOUNT_KEY_CFG,
    
    
    GFPSP_IDX_NB,
};

/// Value element
struct gfpsp_val_elmt
{
    /// list element header
    struct co_list_hdr hdr;
    /// value identifier
    uint8_t value;
    /// value length
    uint8_t length;
    /// value data
    uint8_t data[__ARRAY_EMPTY];
};

///Device Information Service Server Environment Variable
struct gfpsp_env_tag
{
    /// profile environment
    prf_env_t prf_env;
    /// List of values set by application
    struct co_list values;
    /// Service Attribute Start Handle
    uint16_t start_hdl;
    /// Services features
    uint16_t features;
    /// Last requested value
    uint8_t  req_val;
    /// Last connection index which request value
    uint8_t  req_conidx;

    /// GFPSP task state
    ke_state_t state[GFPSP_IDX_MAX];
};
typedef struct _gfpsp_encrypted_req_uint128{
    uint8_t uint128_array[16];
}gfpsp_encrypted_req_uint128;

typedef struct _gfpsp_64B_public_key_{
    uint8_t public_key_64B[64];
}gfpsp_64B_public_key;

// typedef struct _gfpsp_64B_public_key_{
//    uint8_t public_key_64B[64];
//}gfpsp_64B_public_key;

#define RAW_REQ_FLAGS_DISCOVERABILITY_BIT0_EN (1)
#define RAW_REQ_FLAGS_DISCOVERABILITY_BIT0_DIS (0)
#define RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_EN (1)
#define RAW_REQ_FLAGS_INTBONDING_SEEKERADDR_BIT1_DIS (0)



typedef struct _gfpsp_raw_req{
    uint8_t message_type;
    uint8_t flags_discoverability:1;
    uint8_t flags_bonding_addr:1;
    uint8_t flags_reserved    :6;
    uint8_t provider_addr[6]  ;
    uint8_t seeker_addr[6]      ;
    uint8_t reserved[2]         ;
}gfpsp_raw_req;

typedef struct _gfpsp_raw_resp{
    uint8_t message_type;
    uint8_t provider_addr[6];
    uint8_t salt[9];
}gfpsp_raw_resp;
typedef struct _gfpsp_raw_passkey_resp{
    uint8_t message_type;
    uint8_t passkey[3];
    uint8_t reserved[12];
}gfpsp_raw_pass_key_resp;

typedef struct _gfpsp_encrypted_resp{
    uint8_t uint128_array[GFPSP_ENCRYPTED_RSP_LEN];
}gfpsp_encrypted_resp;
 typedef struct _gfpsp_req_resp{
    union {
         gfpsp_encrypted_req_uint128 en_req;
         gfpsp_raw_req               raw_req;     
         gfpsp_raw_resp              raw_rsp;
         gfpsp_encrypted_resp        en_rsp;        
         
    }rx_tx;
}gfpsp_req_resp;

#define GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITH_PUBLIC_KEY (80)
#define GFPSP_KEY_BASED_PAIRING_REQ_LEN_WITHOUT_PUBLIC_KEY (16)


typedef struct _gfpsp_Key_based_Pairing_req{
    gfpsp_encrypted_req_uint128 * en_req;
    gfpsp_64B_public_key        * pub_key;
}gfpsp_Key_based_Pairing_req;

//#if (sizeof(_gfpsp_encrypted_req)!=80)
//#error size of gfpsp_encrypted_req must be 80 bytes
//#endif


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve DIS service profile interface
 *
 * @return DIS service profile interface
 ****************************************************************************************
 */
const struct prf_task_cbs* gfpsp_prf_itf_get(void);


/**
 ****************************************************************************************
 * @brief Check if an attribute shall be added or not in the database
 *
 * @param features DIS features
 *
 * @return Feature config flag
 ****************************************************************************************
 */
uint32_t gfpsp_compute_cfg_flag(uint16_t features);

/**
 ****************************************************************************************
 * @brief Check if the provided value length matches characteristic requirements
 * @param char_code Characteristic Code
 * @param val_len   Length of the Characteristic value
 *
 * @return status if value length is ok or not
 ****************************************************************************************
 */
uint8_t gfpsp_check_val_len(uint8_t char_code, uint8_t val_len);

/**
 ****************************************************************************************
 * @brief Retrieve handle attribute from value
 *
 * @param[in] env   Service environment variable
 * @param[in] value Value to search
 *
 * @return Handle attribute from value
 ****************************************************************************************
 */
uint16_t gfpsp_value_to_handle(struct gfpsp_env_tag* env, uint8_t value);

/**
 ****************************************************************************************
 * @brief Retrieve value from attribute handle
 *
 * @param[in] env    Service environment variable
 * @param[in] handle Attribute handle to search
 *
 * @return  Value from attribute handle
 ****************************************************************************************
 */
uint8_t gfpsp_handle_to_value(struct gfpsp_env_tag* env, uint16_t handle);

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void gfpsp_task_init(struct ke_task_desc *task_desc);

uint32_t gfpsp_crypto_gen_DHKey(const uint8_t *in_PubKey,const uint8_t *in_PrivateKey,uint8_t *out_DHKey);
uint32_t gfpsp_crypto_make_P256_key(uint8_t * out_public_key,uint8_t * out_private_key);

#endif //BLE_GFPSP_SERVER

/// @} GFPSP

#endif // GFPSP_H_
