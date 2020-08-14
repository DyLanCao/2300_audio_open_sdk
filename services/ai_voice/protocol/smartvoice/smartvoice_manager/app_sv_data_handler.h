#ifndef __APP_SV_DATA_HANDLER_H__
#define __APP_SV_DATA_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "app_sv_cmd_code.h"

typedef struct {
	uint8_t		isHasCrcCheck 	:	1;
	uint8_t		reserved		:	7;
	uint8_t		reservedBytes[7];	
} APP_SV_START_DATA_XFER_T;

typedef struct {
	uint8_t		isHasWholeCrcCheck 	:	1;
	uint8_t		reserved		:	7;
	uint8_t		reservedBytes1[3];	
	uint32_t	wholeDataLenToCheck;
	uint32_t	crc32OfWholeData;
} APP_SV_STOP_DATA_XFER_T;

typedef struct {
	uint32_t	segmentDataLen;
	uint32_t	crc32OfSegment;
	uint8_t		reserved[4];
} APP_SV_VERIFY_DATA_SEGMENT_T;

typedef struct {
	uint32_t	reserved;
} APP_SV_START_DATA_XFER_RSP_T;

typedef struct {
	uint32_t	reserved;
} APP_SV_STOP_DATA_XFER_RSP_T;

typedef struct {
	uint32_t	reserved;
} APP_SV_VERIFY_DATA_SEGMENT_RSP_T;


void app_sv_data_xfer_started(APP_SV_CMD_RET_STATUS_E retStatus);
void app_sv_data_xfer_stopped(APP_SV_CMD_RET_STATUS_E retStatus);
void app_sv_data_segment_verified(APP_SV_CMD_RET_STATUS_E retStatus);
void app_sv_data_received_callback(uint8_t* ptrData, uint32_t dataLength);
uint32_t app_sv_send_voice_stream(void *param1, uint32_t param2);
void app_sv_start_data_xfer(APP_SV_START_DATA_XFER_T* req);
void app_sv_stop_data_xfer(APP_SV_STOP_DATA_XFER_T* req);
uint32_t app_sv_rcv_data(void *param1, uint32_t param2);
bool app_sv_data_is_data_transmission_started(void);
void app_sv_kickoff_dataxfer(void);
void app_sv_stop_dataxfer(void);
void app_sv_data_reset_env(void);

#ifdef __cplusplus
	}
#endif

#endif // #ifndef __APP_SV_DATA_HANDLER_H__

