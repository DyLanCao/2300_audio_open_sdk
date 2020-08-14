#ifndef _SMARTVOICE_TASK_H_
#define _SMARTVOICE_TASK_H_

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
#include <stdint.h>
#include "rwip_task.h" // Task definitions

/*
 * DEFINES
 ****************************************************************************************
 */
#define SMARTVOICE_CONTENT_TYPE_COMMAND		0 
#define SMARTVOICE_CONTENT_TYPE_DATA		1

/// Messages for Smart Voice Profile 
enum sv_msg_id
{
	SMARTVOICE_CMD_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_VOICEPATH),

	SMARTVOICE_TX_DATA_SENT,
	
	SMARTVOICE_CMD_RECEIVED,

	SMARTVOICE_SEND_CMD_VIA_NOTIFICATION,

	SMARTVOICE_SEND_CMD_VIA_INDICATION,

	SMARTVOICE_DATA_CCC_CHANGED,

	SMARTVOICE_DATA_RECEIVED,

	SMARTVOICE_SEND_DATA_VIA_NOTIFICATION,

	SMARTVOICE_SEND_DATA_VIA_INDICATION,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct ble_sv_tx_notif_config_t
{
	bool 		isNotificationEnabled;
};

struct ble_sv_rx_data_ind_t
{
	uint16_t	length;
	uint8_t		data[0];
};

struct ble_sv_tx_sent_ind_t
{
	uint8_t 	status;
};

struct ble_sv_send_data_req_t
{
	uint8_t 	connecionIndex;
	uint32_t 	length;
	uint8_t  	value[__ARRAY_EMPTY];
};


/// @} SMARTVOICETASK

#endif /* _SMARTVOICE_TASK_H_ */

