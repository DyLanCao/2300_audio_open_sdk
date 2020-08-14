/**
 ****************************************************************************************
 *
 * @file app_sv_data_handler.c
 *
 * @date 23rd Nov 2017
 *
 * @brief The framework of the smart voice data handler
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
#include "app_sv_cmd_code.h"
#include "app_sv_cmd_handler.h"
#include "app_sv_data_handler.h"
#include "rwapp_config.h"
#include "crc32.h"
#include "app_spp.h"

#if (BLE_APP_VOICEPATH)

#define APP_SV_DATA_BUF_SIZE	640
#define APP_SV_DATA_CMD_TIME_OUT_IN_MS	5000

/**
 * @brief smart voice data handling environment
 *
 */
typedef struct
{
    uint8_t		isDataXferInProgress; 	
	uint8_t		isCrcCheckEnabled;
    APP_SV_TRANSMISSION_PATH_E     pathType;
	uint32_t	wholeDataXferLen;
	uint32_t	dataXferLenUntilLastSegVerification;
	uint32_t	currentWholeCrc32;
	uint32_t 	wholeCrc32UntilLastSeg;
	uint32_t	crc32OfCurrentSeg;											
} APP_SV_DATA_HANDLER_ENV_T;

static 	uint8_t		app_sv_tmpDataXferBuf[APP_SV_DATA_BUF_SIZE];	
static APP_SV_DATA_HANDLER_ENV_T app_sv_data_rec_handler_env;
static APP_SV_DATA_HANDLER_ENV_T app_sv_data_trans_handler_env;

void app_sv_data_set_path_type(APP_SV_TRANSMISSION_PATH_E pathType)
{
    app_sv_data_rec_handler_env.pathType = pathType;
}

void app_sv_data_reset_env(void)
{ 
	app_sv_data_rec_handler_env.isDataXferInProgress = false;
	app_sv_data_rec_handler_env.isCrcCheckEnabled = false;
	app_sv_data_rec_handler_env.wholeDataXferLen = 0;
	app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification = 0;
	app_sv_data_rec_handler_env.currentWholeCrc32 = 0;
	app_sv_data_rec_handler_env.wholeCrc32UntilLastSeg = 0;	
	app_sv_data_rec_handler_env.crc32OfCurrentSeg = 0;	

	app_sv_data_trans_handler_env.isDataXferInProgress = false;
	app_sv_data_trans_handler_env.isCrcCheckEnabled = false;
	app_sv_data_trans_handler_env.wholeDataXferLen = 0;
	app_sv_data_trans_handler_env.dataXferLenUntilLastSegVerification = 0;
	app_sv_data_trans_handler_env.currentWholeCrc32 = 0;
	app_sv_data_trans_handler_env.wholeCrc32UntilLastSeg = 0;	
	app_sv_data_trans_handler_env.crc32OfCurrentSeg = 0;		
}

bool app_sv_data_is_data_transmission_started(void)
{
    return app_sv_data_trans_handler_env.isDataXferInProgress;
}

APP_SV_CMD_RET_STATUS_E app_sv_data_xfer_stop_handler(APP_SV_STOP_DATA_XFER_T* pStopXfer)
{
	APP_SV_CMD_RET_STATUS_E retStatus = NO_ERROR;
	
	if (pStopXfer->isHasWholeCrcCheck)
	{
		if (pStopXfer->wholeDataLenToCheck != app_sv_data_rec_handler_env.wholeDataXferLen)
		{
			retStatus = DATA_XFER_LEN_NOT_MATCHED;
		}

		if (pStopXfer->crc32OfWholeData != app_sv_data_rec_handler_env.currentWholeCrc32)
		{
			retStatus = WHOLE_DATA_CRC_CHECK_FAILED;
		}
	}

	app_sv_data_reset_env();
	app_sv_data_rec_handler_env.isDataXferInProgress = false;

	return retStatus;
}

void app_sv_data_xfer_control_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
	APP_SV_CMD_RET_STATUS_E retStatus = NO_ERROR;
    
	if (OP_START_DATA_XFER == funcCode)
	{
		if (app_sv_data_rec_handler_env.isDataXferInProgress)
		{
			retStatus = DATA_XFER_ALREADY_STARTED;
		}
		else
		{
			if (paramLen < sizeof(APP_SV_START_DATA_XFER_T))
			{
				retStatus = PARAMETER_LENGTH_TOO_SHORT;
			}
			else if (paramLen > sizeof(APP_SV_START_DATA_XFER_T))
			{
				retStatus = PARAM_LEN_OUT_OF_RANGE;
			}
			else
			{
				app_sv_data_reset_env();
				APP_SV_START_DATA_XFER_T* pStartXfer = (APP_SV_START_DATA_XFER_T *)ptrParam;
				app_sv_data_rec_handler_env.isDataXferInProgress = true;
				app_sv_data_rec_handler_env.isCrcCheckEnabled = pStartXfer->isHasCrcCheck;
			}
		}
        APP_SV_START_DATA_XFER_RSP_T rsp = {0};
        app_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, 
            sizeof(rsp), app_sv_data_rec_handler_env.pathType);
	}
	else if (OP_STOP_DATA_XFER == funcCode)
	{
		if (!app_sv_data_rec_handler_env.isDataXferInProgress)
		{
			retStatus = DATA_XFER_NOT_STARTED_YET;
		}
		else
		{
			if (paramLen < sizeof(APP_SV_STOP_DATA_XFER_T))
			{
				retStatus = PARAMETER_LENGTH_TOO_SHORT;
			}
			else if (paramLen > sizeof(APP_SV_STOP_DATA_XFER_T))
			{
				retStatus = PARAM_LEN_OUT_OF_RANGE;
			}
			else
			{
				retStatus = app_sv_data_xfer_stop_handler((APP_SV_STOP_DATA_XFER_T *)ptrParam);
			}
		}
        APP_SV_STOP_DATA_XFER_RSP_T rsp = {0};
        app_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, 
            sizeof(rsp), app_sv_data_rec_handler_env.pathType);
	}
	else
	{
		retStatus = INVALID_CMD;
        app_sv_send_response_to_command(funcCode, retStatus, NULL, 
            0, app_sv_data_rec_handler_env.pathType);
	}		
}

__attribute__((weak)) void app_sv_data_xfer_started(APP_SV_CMD_RET_STATUS_E retStatus)
{

}

void app_sv_kickoff_dataxfer(void)
{
	app_sv_data_reset_env();
	app_sv_data_trans_handler_env.isDataXferInProgress = true;    
}

void app_sv_start_data_xfer_control_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    if (NO_ERROR == retStatus)
	{
		app_sv_kickoff_dataxfer();
	}
	app_sv_data_xfer_started(retStatus);
}

__attribute__((weak)) void app_sv_data_xfer_stopped(APP_SV_CMD_RET_STATUS_E retStatus)
{

}

void app_sv_stop_dataxfer(void)
{
	app_sv_data_reset_env();
	app_sv_data_trans_handler_env.isDataXferInProgress = false;    
}

void app_sv_stop_data_xfer_control_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    if ((NO_ERROR == retStatus) || (WAITING_RSP_TIMEOUT == retStatus))
	{
		app_sv_stop_dataxfer();
	}
	app_sv_data_xfer_stopped(retStatus);
}

void app_sv_data_segment_verifying_handler(APP_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
	APP_SV_CMD_RET_STATUS_E retStatus = NO_ERROR;
	if (paramLen < sizeof(APP_SV_VERIFY_DATA_SEGMENT_T))
	{
		retStatus = PARAMETER_LENGTH_TOO_SHORT;
	}
	else if (paramLen > sizeof(APP_SV_VERIFY_DATA_SEGMENT_T))
	{
		retStatus = PARAM_LEN_OUT_OF_RANGE;
	}	
	else
	{
		APP_SV_VERIFY_DATA_SEGMENT_T* pVerifyData = (APP_SV_VERIFY_DATA_SEGMENT_T *)ptrParam;
		if (pVerifyData->segmentDataLen != 
			(app_sv_data_rec_handler_env.wholeDataXferLen - 
			app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification))
		{
			retStatus = DATA_XFER_LEN_NOT_MATCHED;
		}
		else if (pVerifyData->crc32OfSegment != app_sv_data_rec_handler_env.crc32OfCurrentSeg)
		{	
			app_sv_data_rec_handler_env.wholeDataXferLen = app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification;
			app_sv_data_rec_handler_env.currentWholeCrc32 = app_sv_data_rec_handler_env.wholeCrc32UntilLastSeg;
			retStatus = DATA_SEGMENT_CRC_CHECK_FAILED;
		}
		else
		{
			app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification = app_sv_data_rec_handler_env.wholeDataXferLen;
			app_sv_data_rec_handler_env.wholeCrc32UntilLastSeg = app_sv_data_rec_handler_env.currentWholeCrc32;		
		}

		app_sv_data_rec_handler_env.crc32OfCurrentSeg = 0;
	}

    APP_SV_VERIFY_DATA_SEGMENT_RSP_T rsp = {0};
	app_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp,
        sizeof(rsp), app_sv_data_rec_handler_env.pathType);
}

__attribute__((weak)) void app_sv_data_segment_verification_result_report(APP_SV_CMD_RET_STATUS_E retStatus)
{
	// should re-send this segment
}

void app_sv_verify_data_segment_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
	if (NO_ERROR != retStatus)
	{	
		app_sv_data_trans_handler_env.wholeDataXferLen = app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification;
		app_sv_data_trans_handler_env.currentWholeCrc32 = app_sv_data_trans_handler_env.wholeCrc32UntilLastSeg;
	}
	else
	{
		app_sv_data_trans_handler_env.dataXferLenUntilLastSegVerification = app_sv_data_trans_handler_env.wholeDataXferLen;
		app_sv_data_trans_handler_env.wholeCrc32UntilLastSeg = app_sv_data_trans_handler_env.currentWholeCrc32;		
	}

	app_sv_data_trans_handler_env.crc32OfCurrentSeg = 0;

	app_sv_data_segment_verification_result_report(retStatus);
}

__attribute__((weak)) void app_sv_data_received_callback(uint8_t* ptrData, uint32_t dataLength)
{
	
}

/**
 * @brief Receive the data from the peer device and parse them
 *
 * @param ptrData 		Pointer of the received data
 * @param dataLength	Length of the received data
 * 
 * @return APP_SV_CMD_RET_STATUS_E
 */
APP_SV_CMD_RET_STATUS_E app_sv_data_received(uint8_t* ptrData, uint32_t dataLength)
{
	if ((OP_DATA_XFER != (APP_SV_CMD_CODE_E)(((uint16_t *)ptrData)[0])) || 
		(dataLength < SMARTVOICE_CMD_CODE_SIZE))
	{
		return INVALID_DATA_PACKET;
	}

	uint8_t* rawDataPtr = ptrData + SMARTVOICE_CMD_CODE_SIZE;
	uint32_t rawDataLen = dataLength - SMARTVOICE_CMD_CODE_SIZE;
	app_sv_data_received_callback(rawDataPtr, rawDataLen);

	app_sv_data_rec_handler_env.wholeDataXferLen += rawDataLen;

	if (app_sv_data_rec_handler_env.isCrcCheckEnabled)
	{
		// calculate the CRC
		app_sv_data_rec_handler_env.currentWholeCrc32 = 
		crc32(app_sv_data_rec_handler_env.currentWholeCrc32, rawDataPtr, rawDataLen);
		app_sv_data_rec_handler_env.crc32OfCurrentSeg = 
			crc32(app_sv_data_rec_handler_env.crc32OfCurrentSeg, rawDataPtr, rawDataLen);
	}
	return NO_ERROR;
}

void app_sv_send_data(APP_SV_TRANSMISSION_PATH_E path, uint8_t* ptrData, uint32_t dataLength)
{
	if (path < APP_SV_TRANSMISSION_PATH_COUNT)
	{
		((uint16_t *)app_sv_tmpDataXferBuf)[0] = OP_DATA_XFER;
		memcpy(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE, 
			ptrData, dataLength);
		
		app_sv_data_trans_handler_env.wholeDataXferLen += dataLength;
		if (app_sv_data_trans_handler_env.isCrcCheckEnabled)
		{
			app_sv_data_trans_handler_env.currentWholeCrc32 = 
				crc32(app_sv_data_trans_handler_env.currentWholeCrc32, ptrData, dataLength);
			app_sv_data_trans_handler_env.crc32OfCurrentSeg = 
				crc32(app_sv_data_trans_handler_env.crc32OfCurrentSeg, ptrData, dataLength);
		}
		switch (path)
		{
			case APP_SV_VIA_NOTIFICATION:			
				app_sv_send_data_via_notification(app_sv_tmpDataXferBuf, dataLength + SMARTVOICE_CMD_CODE_SIZE);
				break;
			case APP_SV_VIA_INDICATION:			
				app_sv_send_data_via_indication(app_sv_tmpDataXferBuf, dataLength + SMARTVOICE_CMD_CODE_SIZE);
				break;	
			case APP_SV_VIA_SPP:
                app_sv_send_data_via_spp(app_sv_tmpDataXferBuf, dataLength + SMARTVOICE_CMD_CODE_SIZE);
				break;
			default:
				break;
		}	
	}
}

void app_sv_start_data_xfer(APP_SV_TRANSMISSION_PATH_E path, APP_SV_START_DATA_XFER_T* req)
{
	app_sv_data_trans_handler_env.isCrcCheckEnabled = req->isHasCrcCheck;
    app_sv_send_command(OP_START_DATA_XFER, (uint8_t *)req, sizeof(APP_SV_START_DATA_XFER_T), path);	
}

void app_sv_stop_data_xfer(APP_SV_TRANSMISSION_PATH_E path, APP_SV_STOP_DATA_XFER_T* req)
{
	if (req->isHasWholeCrcCheck)
	{
		req->crc32OfWholeData = app_sv_data_trans_handler_env.currentWholeCrc32;
		req->wholeDataLenToCheck = app_sv_data_trans_handler_env.wholeDataXferLen;
	}
	
	app_sv_send_command(OP_STOP_DATA_XFER, (uint8_t *)req, sizeof(APP_SV_STOP_DATA_XFER_T), path);	
}

void app_sv_verify_data_segment(APP_SV_TRANSMISSION_PATH_E path)
{
	APP_SV_VERIFY_DATA_SEGMENT_T req;
	req.crc32OfSegment = app_sv_data_trans_handler_env.crc32OfCurrentSeg;
	req.segmentDataLen = (app_sv_data_trans_handler_env.wholeDataXferLen - 
			app_sv_data_trans_handler_env.dataXferLenUntilLastSegVerification);
	
	app_sv_send_command(OP_VERIFY_DATA_SEGMENT, (uint8_t *)&req, sizeof(APP_SV_VERIFY_DATA_SEGMENT_T), path);	
}

#if !DEBUG_BLE_DATAPATH
CUSTOM_COMMAND_TO_ADD(OP_START_DATA_XFER, 	app_sv_data_xfer_control_handler, TRUE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,  	app_sv_start_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_STOP_DATA_XFER, 	app_sv_data_xfer_control_handler, TRUE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,  	app_sv_stop_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_VERIFY_DATA_SEGMENT, app_sv_data_segment_verifying_handler, TRUE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,  	app_sv_verify_data_segment_rsp_handler );
#else
CUSTOM_COMMAND_TO_ADD(OP_START_DATA_XFER, 	app_sv_data_xfer_control_handler, FALSE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,  	app_sv_start_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_STOP_DATA_XFER, 	app_sv_data_xfer_control_handler, FALSE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,  	app_sv_stop_data_xfer_control_rsp_handler );
CUSTOM_COMMAND_TO_ADD(OP_VERIFY_DATA_SEGMENT, app_sv_data_segment_verifying_handler, FALSE,  APP_SV_DATA_CMD_TIME_OUT_IN_MS,  	app_sv_verify_data_segment_rsp_handler );
#endif

#endif

