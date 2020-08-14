/**
 ****************************************************************************************
 *
 * @file app_svc_cmd_handler.c
 *
 * @date 23rd Nov 2017
 *
 * @brief The framework of the smart voice command handler
 *
 * Copyright (C) 2017
 *
 *
 ****************************************************************************************
 */
#include "string.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "apps.h"
#include "stdbool.h"
#include "app_sv.h" 
#include "app_sv_cmd_handler.h"
#include "app_sv_cmd_code.h"
#include "rwapp_config.h"
#include "app_spp.h"

#if (BLE_APP_VOICEPATH)

#define APP_SV_CMD_HANDLER_WAITING_RSP_TIMEOUT_SUPERVISOR_COUNT	8

/**
 * @brief waiting response timeout supervision data structure
 *
 */
typedef struct
{
    uint16_t		entryIndex; 		/**< The command waiting for the response */
	uint16_t		msTillTimeout;	/**< run-time timeout left milliseconds */
} APP_SV_CMD_WAITING_RSP_SUPERVISOR_T;

/**
 * @brief smart voice command handling environment
 *
 */
typedef struct
{
	APP_SV_CMD_WAITING_RSP_SUPERVISOR_T	
		waitingRspTimeoutInstance[APP_SV_CMD_HANDLER_WAITING_RSP_TIMEOUT_SUPERVISOR_COUNT];
	uint32_t	lastSysTicks;
	uint8_t		timeoutSupervisorCount;
	osTimerId	supervisor_timer_id;
	osMutexId	mutex;
												
} APP_SV_CMD_HANDLER_ENV_T;

static APP_SV_CMD_HANDLER_ENV_T sv_cmd_handler_env;

osMutexDef(app_sv_cmd_handler_mutex);

static void app_sv_cmd_handler_rsp_supervision_timer_cb(void const *n);
osTimerDef (APP_SV_CMD_HANDLER_RSP_SUPERVISION_TIMER, app_sv_cmd_handler_rsp_supervision_timer_cb); 

static void app_sv_cmd_handler_remove_waiting_rsp_timeout_supervision(uint16_t entryIndex);
static void app_sv_cmd_handler_add_waiting_rsp_timeout_supervision(uint16_t entryIndex);

/**
 * @brief Callback function of the waiting response supervisor timer.
 *
 */
static void app_sv_cmd_handler_rsp_supervision_timer_cb(void const *n)
{
	uint32_t entryIndex = sv_cmd_handler_env.waitingRspTimeoutInstance[0].entryIndex;

	app_sv_cmd_handler_remove_waiting_rsp_timeout_supervision(entryIndex);

	// it means time-out happens before the response is received from the peer device,
	// trigger the response handler
	CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex)->cmdRspHandler(WAITING_RSP_TIMEOUT, NULL, 0);
}

APP_SV_CMD_INSTANCE_T* app_sv_cmd_handler_get_entry_pointer_from_cmd_code(APP_SV_CMD_CODE_E cmdCode)
{
	for (uint32_t index = 0;
		index < ((uint32_t)__custom_handler_table_end-(uint32_t)__custom_handler_table_start)/sizeof(APP_SV_CMD_INSTANCE_T);index++)
	{
		if (CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index)->cmdCode == cmdCode)
		{
			return CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index);
		}			
	}

	return NULL;
}

uint16_t app_sv_cmd_handler_get_entry_index_from_cmd_code(APP_SV_CMD_CODE_E cmdCode)
{

	for (uint32_t index = 0;
		index < ((uint32_t)__custom_handler_table_end-(uint32_t)__custom_handler_table_start)/sizeof(APP_SV_CMD_INSTANCE_T);index++)
	{
		if (CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index)->cmdCode == cmdCode)
		{
			return index;
		}			
	}

	return INVALID_CUSTOM_ENTRY_INDEX;
}

void app_sv_get_cmd_response_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
	// parameter length check
	if (paramLen > sizeof(APP_SV_CMD_RSP_T))
	{
		return;
	}

	if (0 == sv_cmd_handler_env.timeoutSupervisorCount)
	{
		return;
	}

	APP_SV_CMD_RSP_T* rsp = (APP_SV_CMD_RSP_T *)ptrParam;

	
	uint16_t entryIndex = app_sv_cmd_handler_get_entry_index_from_cmd_code((APP_SV_CMD_CODE_E)(rsp->cmdCodeToRsp));
	if (INVALID_CUSTOM_ENTRY_INDEX == entryIndex)
	{
		return;
	}
	
	// remove the function code from the time-out supervision chain
	app_sv_cmd_handler_remove_waiting_rsp_timeout_supervision(entryIndex);	

    APP_SV_CMD_INSTANCE_T* ptCmdInstance = CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);

	// call the response handler
	if (ptCmdInstance->cmdRspHandler)
	{
		ptCmdInstance->cmdRspHandler((APP_SV_CMD_RET_STATUS_E)(rsp->cmdRetStatus), rsp->rspData, rsp->rspDataLen);
	}
}



/**
 * @brief Refresh the waiting response supervisor list
 *
 */
static void app_sv_cmd_refresh_supervisor_env(void)
{
	// do nothing if no supervisor was added
	if (sv_cmd_handler_env.timeoutSupervisorCount > 0)
	{	
		uint32_t currentTicks = GET_CURRENT_TICKS();
		uint32_t passedTicks;
		if (currentTicks >= sv_cmd_handler_env.lastSysTicks)
		{
			passedTicks = (currentTicks - sv_cmd_handler_env.lastSysTicks);
		}
		else
		{
			passedTicks = (hal_sys_timer_get_max() - sv_cmd_handler_env.lastSysTicks + 1) + currentTicks;
		}
		
		uint32_t deltaMs = TICKS_TO_MS(passedTicks);
		
		APP_SV_CMD_WAITING_RSP_SUPERVISOR_T* pRspSupervisor = &(sv_cmd_handler_env.waitingRspTimeoutInstance[0]);
		for (uint32_t index = 0;index < sv_cmd_handler_env.timeoutSupervisorCount;index++)
		{
			if (pRspSupervisor[index].msTillTimeout > deltaMs)
			{
                pRspSupervisor[index].msTillTimeout -= deltaMs;
			}
            else
            {
                pRspSupervisor[index].msTillTimeout = 0;
            }
		}
	}

	sv_cmd_handler_env.lastSysTicks = GET_CURRENT_TICKS();
}

/**
 * @brief Remove the time-out supervision of waiting response
 *
 * @param entryIndex 	Entry index of the command table
 * 
 */
static void app_sv_cmd_handler_remove_waiting_rsp_timeout_supervision(uint16_t entryIndex)
{
	ASSERT(sv_cmd_handler_env.timeoutSupervisorCount > 0,
		"%s The BLE custom command time-out supervisor is already empty!!!", __FUNCTION__);

	osMutexWait(sv_cmd_handler_env.mutex, osWaitForever);

	uint32_t index;
	for (index = 0;index < sv_cmd_handler_env.timeoutSupervisorCount;index++)
	{
		if (sv_cmd_handler_env.waitingRspTimeoutInstance[index].entryIndex == entryIndex)
		{
			memcpy(&(sv_cmd_handler_env.waitingRspTimeoutInstance[index]), 
				&(sv_cmd_handler_env.waitingRspTimeoutInstance[index + 1]), 
				(sv_cmd_handler_env.timeoutSupervisorCount - index - 1)*sizeof(APP_SV_CMD_WAITING_RSP_SUPERVISOR_T));
			break;
		}
	}

	// cannot find it, directly return
	if (index == sv_cmd_handler_env.timeoutSupervisorCount)
	{
		goto exit;
	}
	
	sv_cmd_handler_env.timeoutSupervisorCount--;	

    //TRACE("decrease the supervisor timer to %d", sv_cmd_handler_env.timeoutSupervisorCount);

	if (sv_cmd_handler_env.timeoutSupervisorCount > 0)
	{
		// refresh supervisor environment firstly
		app_sv_cmd_refresh_supervisor_env();
			
		// start timer, the first entry is the most close one
		osTimerStart(sv_cmd_handler_env.supervisor_timer_id, 
			sv_cmd_handler_env.waitingRspTimeoutInstance[0].msTillTimeout);
	}
	else
	{
		// no supervisor, directly stop the timer
		osTimerStop(sv_cmd_handler_env.supervisor_timer_id);
	}

exit:
	osMutexRelease(sv_cmd_handler_env.mutex);
}

/**
 * @brief Add the time-out supervision of waiting response
 *
 * @param entryIndex 	Index of the command entry
 * 
 */
static void app_sv_cmd_handler_add_waiting_rsp_timeout_supervision(uint16_t entryIndex)
{
	ASSERT(sv_cmd_handler_env.timeoutSupervisorCount < APP_SV_CMD_HANDLER_WAITING_RSP_TIMEOUT_SUPERVISOR_COUNT,
		"%s The smart voice command response time-out supervisor is full!!!", __FUNCTION__);

	osMutexWait(sv_cmd_handler_env.mutex, osWaitForever);

	// refresh supervisor environment firstly
	app_sv_cmd_refresh_supervisor_env();

	APP_SV_CMD_INSTANCE_T* pInstance = CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);

	APP_SV_CMD_WAITING_RSP_SUPERVISOR_T	waitingRspTimeoutInstance[APP_SV_CMD_HANDLER_WAITING_RSP_TIMEOUT_SUPERVISOR_COUNT];

	uint32_t index = 0, insertedIndex = 0;
	for (index = 0;index < sv_cmd_handler_env.timeoutSupervisorCount;index++)
	{
		uint32_t msTillTimeout = sv_cmd_handler_env.waitingRspTimeoutInstance[index].msTillTimeout;

		// in the order of low to high
		if ((sv_cmd_handler_env.waitingRspTimeoutInstance[index].entryIndex != entryIndex) &&
			(pInstance->timeoutWaitingRspInMs >= msTillTimeout))
		{
			waitingRspTimeoutInstance[insertedIndex++] = sv_cmd_handler_env.waitingRspTimeoutInstance[index];
		}
		else if (pInstance->timeoutWaitingRspInMs < msTillTimeout)
		{
			waitingRspTimeoutInstance[insertedIndex].entryIndex = entryIndex;
			waitingRspTimeoutInstance[insertedIndex].msTillTimeout = pInstance->timeoutWaitingRspInMs;

			insertedIndex++;
		}
	}

	// biggest one? then put it at the end of the list
	if (sv_cmd_handler_env.timeoutSupervisorCount == index)
	{
		waitingRspTimeoutInstance[insertedIndex].entryIndex = entryIndex;
		waitingRspTimeoutInstance[insertedIndex].msTillTimeout = pInstance->timeoutWaitingRspInMs;
		
		insertedIndex++;
	}

	// copy to the global variable
	memcpy((uint8_t *)&(sv_cmd_handler_env.waitingRspTimeoutInstance), (uint8_t *)&waitingRspTimeoutInstance,
		insertedIndex*sizeof(APP_SV_CMD_WAITING_RSP_SUPERVISOR_T));

	sv_cmd_handler_env.timeoutSupervisorCount = insertedIndex;	

    //TRACE("increase the supervisor timer to %d", sv_cmd_handler_env.timeoutSupervisorCount);


    osMutexRelease(sv_cmd_handler_env.mutex);

	// start timer, the first entry is the most close one
	osTimerStart(sv_cmd_handler_env.supervisor_timer_id, sv_cmd_handler_env.waitingRspTimeoutInstance[0].msTillTimeout);

}

/**
 * @brief Receive the data from the peer device and parse them
 *
 * @param ptrData 		Pointer of the received data
 * @param dataLength	Length of the received data
 * 
 * @return APP_SV_CMD_RET_STATUS_E
 */
APP_SV_CMD_RET_STATUS_E app_sv_cmd_received(uint8_t* ptrData, uint32_t dataLength)
{
	TRACE("Receive data:");
	DUMP8("0x%02x ", ptrData, dataLength);
	APP_SV_CMD_PAYLOAD_T* pPayload = (APP_SV_CMD_PAYLOAD_T *)ptrData;
	
	// check command code
	if (pPayload->cmdCode >= OP_COMMAND_COUNT)
	{
		return INVALID_CMD;
	}

	// check parameter length
	if (pPayload->paramLen > sizeof(pPayload->param))
	{
		return PARAM_LEN_OUT_OF_RANGE;
	}

	APP_SV_CMD_INSTANCE_T* pInstance = 
		app_sv_cmd_handler_get_entry_pointer_from_cmd_code((APP_SV_CMD_CODE_E)(pPayload->cmdCode));

	// execute the command handler
	pInstance->cmdHandler((APP_SV_CMD_CODE_E)(pPayload->cmdCode), pPayload->param, pPayload->paramLen);

	return NO_ERROR;
}

static void app_sv_cmd_send(APP_SV_TRANSMISSION_PATH_E path, APP_SV_CMD_PAYLOAD_T* ptPayLoad)
{
    TRACE("Send cmd:");
    DUMP8("%02x ", (uint8_t *)ptPayLoad, 
	    (uint32_t)(&(((APP_SV_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
    
	switch (path)
	{
		case APP_SV_VIA_NOTIFICATION:			
			app_sv_send_cmd_via_notification((uint8_t *)ptPayLoad, 
				(uint32_t)(&(((APP_SV_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
			break;
		case APP_SV_VIA_INDICATION:			
			app_sv_send_cmd_via_indication((uint8_t *)ptPayLoad, 
				(uint32_t)(&(((APP_SV_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
			break;	
        case APP_SV_VIA_SPP:
            app_sv_send_cmd_via_spp((uint8_t *)ptPayLoad, 
				(uint32_t)(&(((APP_SV_CMD_PAYLOAD_T *)0)->param)) + ptPayLoad->paramLen);
            break;
		default:
			break;
	}	
}

/**
 * @brief Send response to the command request 
 *
 * @param responsedCmdCode 	Command code of the responsed command request
 * @param returnStatus		Handling result
 * @param rspData 			Pointer of the response data
 * @param rspDataLen		Length of the response data
 * @param path				Path of the data transmission
 * 
 * @return APP_SV_CMD_RET_STATUS_E
 */
APP_SV_CMD_RET_STATUS_E app_sv_send_response_to_command
	(APP_SV_CMD_CODE_E responsedCmdCode, APP_SV_CMD_RET_STATUS_E returnStatus, 
	uint8_t* rspData, uint32_t rspDataLen, APP_SV_TRANSMISSION_PATH_E path)
{
	// check responsedCmdCode's validity
	if (responsedCmdCode >= OP_COMMAND_COUNT)
	{
		return INVALID_CMD;
	}

	APP_SV_CMD_PAYLOAD_T payload;

	APP_SV_CMD_RSP_T* pResponse = (APP_SV_CMD_RSP_T *)&(payload.param);

	// check parameter length
	if (rspDataLen > sizeof(pResponse->rspData))
	{
		return PARAM_LEN_OUT_OF_RANGE;
	}

	pResponse->cmdCodeToRsp = responsedCmdCode;
	pResponse->cmdRetStatus = returnStatus;
	pResponse->rspDataLen = rspDataLen;
	memcpy(pResponse->rspData, rspData, rspDataLen);

	payload.paramLen = 3*sizeof(uint16_t) + rspDataLen;

	payload.cmdCode = OP_RESPONSE_TO_CMD;

	app_sv_cmd_send(path, &payload);

	return NO_ERROR;
}

/**
 * @brief Send the smart voice command to the peer device
 *
 * @param cmdCode 	Command code
 * @param ptrParam 	Pointer of the output parameter
 * @param paramLen	Length of the output parameter
 * @param path		Path of the data transmission
 * 
 * @return APP_SV_CMD_RET_STATUS_E
 */
APP_SV_CMD_RET_STATUS_E app_sv_send_command(APP_SV_CMD_CODE_E cmdCode, 
	uint8_t* ptrParam, uint32_t paramLen, APP_SV_TRANSMISSION_PATH_E path)
{
	// check cmdCode's validity
	if (cmdCode >= OP_COMMAND_COUNT)
	{
		return INVALID_CMD;
	}

	APP_SV_CMD_PAYLOAD_T payload;

	// check parameter length
	if (paramLen > sizeof(payload.param))
	{
		return PARAM_LEN_OUT_OF_RANGE;
	}
	
	uint16_t entryIndex = app_sv_cmd_handler_get_entry_index_from_cmd_code(cmdCode);
    if (INVALID_CUSTOM_ENTRY_INDEX == entryIndex)
    {
        return INVALID_CMD;
    }
     
	 APP_SV_CMD_INSTANCE_T* pInstance = CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(entryIndex);

	// wrap the command payload	
	payload.cmdCode = cmdCode;
	payload.paramLen = paramLen;
	memcpy(payload.param, ptrParam, paramLen);

	// send out the data
	app_sv_cmd_send(path, &payload);

	// insert into time-out supervison
	if (pInstance->isNeedResponse)
	{
		app_sv_cmd_handler_add_waiting_rsp_timeout_supervision(entryIndex);
	}

	return NO_ERROR;
}

/**
 * @brief Initialize the smart voice command handler framework
 *
 */
void app_sv_cmd_handler_init(void)
{
	memset((uint8_t *)&sv_cmd_handler_env, 0, sizeof(sv_cmd_handler_env));

	sv_cmd_handler_env.supervisor_timer_id = 
		osTimerCreate(osTimer(APP_SV_CMD_HANDLER_RSP_SUPERVISION_TIMER), osTimerOnce, NULL);

	sv_cmd_handler_env.mutex = osMutexCreate((osMutex(app_sv_cmd_handler_mutex)));
}

CUSTOM_COMMAND_TO_ADD(OP_RESPONSE_TO_CMD, app_sv_get_cmd_response_handler, false, 0, NULL );

#endif	// #if BLE_APP_VOICEPATH

