#ifndef _SMARTVOICE_GATT_SERVER_H_
#define _SMARTVOICE_GATT_SERVER_H_

/**
 ****************************************************************************************
 * @addtogroup SMARTVOICETASK Task
 * @ingroup SMARTVOICE
 * @brief Smart Voice Profile Task.
 *
 * The SMARTVOICETASK is responsible for handling the messages coming in and out of the
 * @ref SMARTVOICE collector block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */



#ifdef __SMART_VOICE__
#include "ke_task.h"



///Attributes State Machine
enum {
    SMARTVOICE_IDX_SVC,

    SMARTVOICE_IDX_CMD_TX_CHAR,
    SMARTVOICE_IDX_CMD_TX_VAL,
    SMARTVOICE_IDX_CMD_TX_NTF_CFG,

    SMARTVOICE_IDX_CMD_RX_CHAR,
    SMARTVOICE_IDX_CMD_RX_VAL,

    SMARTVOICE_IDX_DATA_TX_CHAR,
    SMARTVOICE_IDX_DATA_TX_VAL,
    SMARTVOICE_IDX_DATA_TX_NTF_CFG,

    SMARTVOICE_IDX_DATA_RX_CHAR,
    SMARTVOICE_IDX_DATA_RX_VAL,

    SMARTVOICE_IDX_NB,
};


/// Messages for Smart Voice Profile 
enum sv_msg_id {
	SMARTVOICE_CMD_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_AI),

	SMARTVOICE_TX_DATA_SENT,
	
	SMARTVOICE_CMD_RECEIVED,

	SMARTVOICE_SEND_CMD_VIA_NOTIFICATION,

	SMARTVOICE_SEND_CMD_VIA_INDICATION,

	SMARTVOICE_DATA_CCC_CHANGED,

	SMARTVOICE_DATA_RECEIVED,

	SMARTVOICE_SEND_DATA_VIA_NOTIFICATION,

	SMARTVOICE_SEND_DATA_VIA_INDICATION,
};


/**
 ****************************************************************************************
 * Initialize task handler
 *
 * @param task_desc Task descriptor to fill
 ****************************************************************************************
 */
void smartvoice_task_init(struct ke_task_desc *task_desc);



#endif /* __SMART_VOICE__ */



#endif //_SMARTVOICE_GATT_SERVER_H_

