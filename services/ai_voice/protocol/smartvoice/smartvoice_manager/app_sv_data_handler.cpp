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
#include "bluetooth.h"
#include "app_sv_cmd_code.h"
#include "app_sv_cmd_handler.h"
#include "app_sv_data_handler.h"
#include "app_smartvoice_handle.h"
#include "rwapp_config.h"
#include "crc32.h"
#include "app_smartvoice_ble.h"
#include "app_smartvoice_bt.h"
#include "ai_thread.h"
#include "ai_transport.h"
#include "voice_compression.h"

#ifdef __SMART_VOICE__

#define APP_SV_DATA_CMD_TIME_OUT_IN_MS	5000
#define VOB_VOICE_XFER_CHUNK_SIZE   (VOB_VOICE_ENCODED_DATA_FRAME_SIZE*VOB_VOICE_CAPTURE_FRAME_CNT)

/**
 * @brief smart voice data handling environment
 *
 */
typedef struct {
    uint8_t		isDataXferInProgress; 	
	uint8_t		isCrcCheckEnabled;
	uint32_t	wholeDataXferLen;
	uint32_t	dataXferLenUntilLastSegVerification;
	uint32_t	currentWholeCrc32;
	uint32_t 	wholeCrc32UntilLastSeg;
	uint32_t	crc32OfCurrentSeg;											
} APP_SV_DATA_HANDLER_ENV_T;

static APP_SV_DATA_HANDLER_ENV_T app_sv_data_rec_handler_env;
static APP_SV_DATA_HANDLER_ENV_T app_sv_data_trans_handler_env;

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
	
	if (pStopXfer->isHasWholeCrcCheck) {
		if (pStopXfer->wholeDataLenToCheck != app_sv_data_rec_handler_env.wholeDataXferLen) {
			retStatus = DATA_XFER_LEN_NOT_MATCHED;
		}

		if (pStopXfer->crc32OfWholeData != app_sv_data_rec_handler_env.currentWholeCrc32) {
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
    
	if (OP_START_DATA_XFER == funcCode) {
		if (app_sv_data_rec_handler_env.isDataXferInProgress) {
			retStatus = DATA_XFER_ALREADY_STARTED;
		} else {
			if (paramLen < sizeof(APP_SV_START_DATA_XFER_T)) {
				retStatus = PARAMETER_LENGTH_TOO_SHORT;
			} else if (paramLen > sizeof(APP_SV_START_DATA_XFER_T)) {
				retStatus = PARAM_LEN_OUT_OF_RANGE;
			} else {
				app_sv_data_reset_env();
				APP_SV_START_DATA_XFER_T* pStartXfer = (APP_SV_START_DATA_XFER_T *)ptrParam;
				app_sv_data_rec_handler_env.isDataXferInProgress = true;
				app_sv_data_rec_handler_env.isCrcCheckEnabled = pStartXfer->isHasCrcCheck;
			}
		}
        APP_SV_START_DATA_XFER_RSP_T rsp = {0};
        app_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, sizeof(rsp));
	} else if (OP_STOP_DATA_XFER == funcCode) {
		if (!app_sv_data_rec_handler_env.isDataXferInProgress) {
			retStatus = DATA_XFER_NOT_STARTED_YET;
		} else {
			if (paramLen < sizeof(APP_SV_STOP_DATA_XFER_T)) {
				retStatus = PARAMETER_LENGTH_TOO_SHORT;
			} else if (paramLen > sizeof(APP_SV_STOP_DATA_XFER_T)) {
				retStatus = PARAM_LEN_OUT_OF_RANGE;
			} else {
				retStatus = app_sv_data_xfer_stop_handler((APP_SV_STOP_DATA_XFER_T *)ptrParam);
			}
		}
        APP_SV_STOP_DATA_XFER_RSP_T rsp = {0};
        app_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, sizeof(rsp));
	} else {
		retStatus = INVALID_CMD;
        app_sv_send_response_to_command(funcCode, retStatus, NULL, 0);
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
	if (paramLen < sizeof(APP_SV_VERIFY_DATA_SEGMENT_T)) {
		retStatus = PARAMETER_LENGTH_TOO_SHORT;
	} else if (paramLen > sizeof(APP_SV_VERIFY_DATA_SEGMENT_T)) {
		retStatus = PARAM_LEN_OUT_OF_RANGE;
	} else {
		APP_SV_VERIFY_DATA_SEGMENT_T* pVerifyData = (APP_SV_VERIFY_DATA_SEGMENT_T *)ptrParam;
		if (pVerifyData->segmentDataLen != 
			(app_sv_data_rec_handler_env.wholeDataXferLen - 
			app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification)) {
			retStatus = DATA_XFER_LEN_NOT_MATCHED;
		} else if (pVerifyData->crc32OfSegment != app_sv_data_rec_handler_env.crc32OfCurrentSeg) {	
			app_sv_data_rec_handler_env.wholeDataXferLen = app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification;
			app_sv_data_rec_handler_env.currentWholeCrc32 = app_sv_data_rec_handler_env.wholeCrc32UntilLastSeg;
			retStatus = DATA_SEGMENT_CRC_CHECK_FAILED;
		} else {
			app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification = app_sv_data_rec_handler_env.wholeDataXferLen;
			app_sv_data_rec_handler_env.wholeCrc32UntilLastSeg = app_sv_data_rec_handler_env.currentWholeCrc32;		
		}

		app_sv_data_rec_handler_env.crc32OfCurrentSeg = 0;
	}

    APP_SV_VERIFY_DATA_SEGMENT_RSP_T rsp = {0};
	app_sv_send_response_to_command(funcCode, retStatus, (uint8_t *)&rsp, sizeof(rsp));
}

__attribute__((weak)) void app_sv_data_segment_verification_result_report(APP_SV_CMD_RET_STATUS_E retStatus)
{
	// should re-send this segment
}

void app_sv_verify_data_segment_rsp_handler(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
	if (NO_ERROR != retStatus) {	
		app_sv_data_trans_handler_env.wholeDataXferLen = app_sv_data_rec_handler_env.dataXferLenUntilLastSegVerification;
		app_sv_data_trans_handler_env.currentWholeCrc32 = app_sv_data_trans_handler_env.wholeCrc32UntilLastSeg;
	} else {
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
uint32_t app_sv_rcv_data(void *param1, uint32_t param2)
{
    TRACE("%s", __func__);
    
	if ((OP_DATA_XFER != (APP_SV_CMD_CODE_E)(((uint16_t *)param1)[0])) || 
		(param2 < SMARTVOICE_CMD_CODE_SIZE)) {
        TRACE("%s error len %d", __func__, param2);
		return 1;
	}

	uint8_t* rawDataPtr = (uint8_t*)param1 + SMARTVOICE_CMD_CODE_SIZE;
	uint32_t rawDataLen = param2 - SMARTVOICE_CMD_CODE_SIZE;
	app_sv_data_received_callback(rawDataPtr, rawDataLen);

	app_sv_data_rec_handler_env.wholeDataXferLen += rawDataLen;

	if (app_sv_data_rec_handler_env.isCrcCheckEnabled) {
		// calculate the CRC
		app_sv_data_rec_handler_env.currentWholeCrc32 = 
		crc32(app_sv_data_rec_handler_env.currentWholeCrc32, rawDataPtr, rawDataLen);
		app_sv_data_rec_handler_env.crc32OfCurrentSeg = 
			crc32(app_sv_data_rec_handler_env.crc32OfCurrentSeg, rawDataPtr, rawDataLen);
	}
	return 0;
}

#ifdef SPEECH_CAPTURE_TWO_CHANNEL
uint32_t app_sv_send_voice_stream(void *param1, uint32_t param2)
{
    uint8_t app_sv_tmpDataXferBuf[640];
	uint16_t fact_send_len = 0;
    uint32_t dataBytesToSend = 0;
	uint32_t encodedDataLength = voice_compression_get_encode_buf_size();

    TRACE("%s length %d max %d", __func__, encodedDataLength, ai_struct.txbuff_send_max_size);
    	
    if (encodedDataLength < (VOB_VOICE_XFER_CHUNK_SIZE*2)) {
        TRACE("%s data len too small %d", __func__, encodedDataLength);
        return 1;
    }

    dataBytesToSend = VOB_VOICE_XFER_CHUNK_SIZE;
    fact_send_len = dataBytesToSend + SMARTVOICE_CMD_CODE_SIZE + SMARTVOICE_ENCODED_DATA_SEQN_SIZE;

    //ai_struct.ai_stream.seqNumWithInFrame = 0;
	((uint16_t *)app_sv_tmpDataXferBuf)[0] = OP_DATA_XFER;
    app_sv_tmpDataXferBuf[SMARTVOICE_CMD_CODE_SIZE] = 0;
    //memcpy(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE, &(ai_struct.ai_stream.seqNumWithInFrame), 
        //SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
    voice_compression_get_encoded_data(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE + SMARTVOICE_ENCODED_DATA_SEQN_SIZE,
        dataBytesToSend);
    
    ai_transport_data_put(app_sv_tmpDataXferBuf, fact_send_len);
    ai_mailbox_put(SIGN_AI_TRANSPORT, fact_send_len);
    
    //ai_struct.ai_stream.seqNumWithInFrame++;
	((uint16_t *)app_sv_tmpDataXferBuf)[0] = OP_DATA_XFER;
    app_sv_tmpDataXferBuf[SMARTVOICE_CMD_CODE_SIZE] = 1;
    //memcpy(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE + fact_send_len, &(ai_struct.ai_stream.seqNumWithInFrame), 
        //SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
    voice_compression_get_encoded_data(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE + SMARTVOICE_ENCODED_DATA_SEQN_SIZE,
        dataBytesToSend);

#if 0
    app_sv_data_trans_handler_env.wholeDataXferLen += (dataBytesToSend+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
	if (app_sv_data_trans_handler_env.isCrcCheckEnabled) {
		app_sv_data_trans_handler_env.currentWholeCrc32 = 
			crc32(app_sv_data_trans_handler_env.currentWholeCrc32, &app_sv_tmpDataXferBuf[SMARTVOICE_CMD_CODE_SIZE],
			    dataBytesToSend+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
		app_sv_data_trans_handler_env.crc32OfCurrentSeg = 
			crc32(app_sv_data_trans_handler_env.crc32OfCurrentSeg, &app_sv_tmpDataXferBuf[SMARTVOICE_CMD_CODE_SIZE],
			    dataBytesToSend+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
	}
#endif

    //fact_send_len *= 2;
    TRACE("%s fact_send_len %d", __func__, fact_send_len);
    ai_transport_data_put(app_sv_tmpDataXferBuf, fact_send_len);
    ai_mailbox_put(SIGN_AI_TRANSPORT, fact_send_len);
	
    return 0;
}
#else
uint32_t app_sv_send_voice_stream(void *param1, uint32_t param2)
{
    uint8_t app_sv_tmpDataXferBuf[640];
	uint16_t fact_send_len = 0;
    uint32_t dataBytesToSend = 0;
	uint32_t encodedDataLength = voice_compression_get_encode_buf_size();

    //TRACE("%s length %d", __func__, encodedDataLength);
    
    #if 0
	if(!is_ai_audio_stream_allowed_to_send()){
        voice_compression_get_encoded_data(NULL ,encodedDataLength);
        TRACE("%s ama don't allowed_to_send encodedDataLength %d", __func__, encodedDataLength);
		return 1;
	}
    #endif
	
    if (encodedDataLength < VOB_VOICE_XFER_CHUNK_SIZE) {
        TRACE("%s data len too small %d", __func__, encodedDataLength);
        return 1;
    }

    if (encodedDataLength > ai_struct.txbuff_send_max_size) {
        dataBytesToSend = ai_struct.txbuff_send_max_size;
    } else {
        dataBytesToSend = encodedDataLength;
    }
    
    if ((ai_struct.ai_stream.sentDataSizeWithInFrame + dataBytesToSend) >= VOB_VOICE_XFER_CHUNK_SIZE) {
        dataBytesToSend = VOB_VOICE_XFER_CHUNK_SIZE - ai_struct.ai_stream.sentDataSizeWithInFrame;
    }
    
    ai_struct.ai_stream.sentDataSizeWithInFrame += dataBytesToSend;
    fact_send_len = dataBytesToSend + SMARTVOICE_CMD_CODE_SIZE + SMARTVOICE_ENCODED_DATA_SEQN_SIZE;
    
	((uint16_t *)app_sv_tmpDataXferBuf)[0] = OP_DATA_XFER;
    memcpy(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE, &(ai_struct.ai_stream.seqNumWithInFrame), 
        SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
    voice_compression_get_encoded_data(app_sv_tmpDataXferBuf + SMARTVOICE_CMD_CODE_SIZE + SMARTVOICE_ENCODED_DATA_SEQN_SIZE,
        dataBytesToSend);
    
    if (ai_struct.ai_stream.sentDataSizeWithInFrame < VOB_VOICE_XFER_CHUNK_SIZE) {
        ai_struct.ai_stream.seqNumWithInFrame++;
    } else {
        ai_struct.ai_stream.seqNumWithInFrame = 0;
        ai_struct.ai_stream.sentDataSizeWithInFrame = 0;
    }
	
	app_sv_data_trans_handler_env.wholeDataXferLen += (dataBytesToSend+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
	if (app_sv_data_trans_handler_env.isCrcCheckEnabled) {
		app_sv_data_trans_handler_env.currentWholeCrc32 = 
			crc32(app_sv_data_trans_handler_env.currentWholeCrc32, &app_sv_tmpDataXferBuf[SMARTVOICE_CMD_CODE_SIZE],
			    dataBytesToSend+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
		app_sv_data_trans_handler_env.crc32OfCurrentSeg = 
			crc32(app_sv_data_trans_handler_env.crc32OfCurrentSeg, &app_sv_tmpDataXferBuf[SMARTVOICE_CMD_CODE_SIZE],
			    dataBytesToSend+SMARTVOICE_ENCODED_DATA_SEQN_SIZE);
	}
    
    ai_transport_data_put(app_sv_tmpDataXferBuf, fact_send_len);
    ai_mailbox_put(SIGN_AI_TRANSPORT, fact_send_len);
	
    return 0;
}
#endif

void app_sv_start_data_xfer(APP_SV_START_DATA_XFER_T* req)
{
	app_sv_data_trans_handler_env.isCrcCheckEnabled = req->isHasCrcCheck;
    app_sv_send_command(OP_START_DATA_XFER, (uint8_t *)req, sizeof(APP_SV_START_DATA_XFER_T));	
}

void app_sv_stop_data_xfer(APP_SV_STOP_DATA_XFER_T* req)
{
	if (req->isHasWholeCrcCheck) {
		req->crc32OfWholeData = app_sv_data_trans_handler_env.currentWholeCrc32;
		req->wholeDataLenToCheck = app_sv_data_trans_handler_env.wholeDataXferLen;
	}
	
	app_sv_send_command(OP_STOP_DATA_XFER, (uint8_t *)req, sizeof(APP_SV_STOP_DATA_XFER_T));	
}

void app_sv_verify_data_segment(void)
{
	APP_SV_VERIFY_DATA_SEGMENT_T req;
	req.crc32OfSegment = app_sv_data_trans_handler_env.crc32OfCurrentSeg;
	req.segmentDataLen = (app_sv_data_trans_handler_env.wholeDataXferLen - 
			app_sv_data_trans_handler_env.dataXferLenUntilLastSegVerification);
	
	app_sv_send_command(OP_VERIFY_DATA_SEGMENT, (uint8_t *)&req, sizeof(APP_SV_VERIFY_DATA_SEGMENT_T));	
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

#endif //__SMART_VOICE__

