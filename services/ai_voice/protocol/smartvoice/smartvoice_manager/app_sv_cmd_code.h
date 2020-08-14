#ifndef __APP_SV_CMD_CODE_H__
#define __APP_SV_CMD_CODE_H__

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t __custom_handler_table_start[];
extern uint32_t __custom_handler_table_end[];

#define INVALID_CUSTOM_ENTRY_INDEX		0xFFFF
#define SMARTVOICE_CMD_CODE_SIZE		sizeof(uint16_t)


/**
 * @brief The command code
 *
 */
typedef enum
{
	/* 0 */ OP_RESPONSE_TO_CMD = 0x8000,	/**< the payload is: OP_RESPONSE_TO_CMD + paramLen + BLE_CUSTOM_CMD_RSP_T */
	/* 1 */ OP_START_DATA_XFER = 0x8001,	
	/* 2 */ OP_VERIFY_DATA_SEGMENT = 0x8002,
	/* 3 */ OP_STOP_DATA_XFER = 0x8003,	
	/* 4 */ OP_START_SMART_VOICE_STREAM = 0x8004,
	/* 5 */ OP_STOP_SMART_VOICE_STREAM = 0x8005,
	/* 6 */ OP_CLOUD_MUSIC_PLAYING_CONTROL = 0x8006,
	/* 7 */ OP_CHOOSE_CLOUD_PLATFORM = 0x8007,
	/* 8 */ OP_SPP_DATA_ACK = 0x8008,
	/* 9 */ OP_INFORM_THROUGHPUT_TEST_CONFIG = 0x8009,
	/* 10 */ OP_THROUGHPUT_TEST_DATA = 0x800A,
	/* 11 */ OP_THROUGHPUT_TEST_DATA_ACK = 0x800B,
	/* 12 */ OP_THROUGHPUT_TEST_DONE = 0x800C,
	/* 13 */ OP_INFORM_ALGORITHM_TYPE = 0x800D, 
	// TODO: add new command code here
	
	OP_COMMAND_COUNT,

	OP_DATA_XFER = 0xFFFF,	// to mark that it's a data packet	
		
} APP_SV_CMD_CODE_E;

/**
 * @brief The smart voice command handling return status
 *
 */
typedef enum
{
	NO_ERROR = 0,
	INVALID_CMD,
	INVALID_DATA_PACKET,
	PARAM_LEN_OUT_OF_RANGE,
	PARAMETER_LENGTH_TOO_SHORT,
	PARAM_LEN_TOO_SHORT,	
	CMD_HANDLING_FAILED,
	WAITING_RSP_TIMEOUT,
	DATA_XFER_ALREADY_STARTED,
	DATA_XFER_NOT_STARTED_YET,
	DATA_SEGMENT_CRC_CHECK_FAILED,
	WHOLE_DATA_CRC_CHECK_FAILED,
	DATA_XFER_LEN_NOT_MATCHED,	
	MUSIC_ALREADY_PLAYING,
	MUSIC_NOT_PLAYING_YET,	
	
	// TO ADD: new return status
} APP_SV_CMD_RET_STATUS_E;

/**
 * @brief Format of the smart voice command handler function, called when the command is received
 *
 * @param cmdCode	Custom command code
 * @param ptrParam 	Pointer of the received parameter
 * @param paramLen 	Length of the recevied parameter
 * 
 */
typedef void (*app_sv_cmd_handler_t)(APP_SV_CMD_CODE_E cmdCode, uint8_t* ptrParam, uint32_t paramLen);

/**
 * @brief Format of the smart voice command response handler function, 
 *	called when the response to formerly sent command is received
 *
 * @param retStatus	Handling return status of the command
 * @param ptrParam 	Pointer of the received parameter
 * @param paramLen 	Length of the recevied parameter
 * 
 */
typedef void (*app_sv_cmd_response_handler_t)(APP_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen);

/**
 * @brief Smart voice command definition data structure
 *
 */
typedef struct
{
	uint32_t				cmdCode;
    app_sv_cmd_handler_t 	cmdHandler;  			/**< command handler function */
    uint16_t 				isNeedResponse;     	/**< true if needs the response from the peer device */
	uint16_t				timeoutWaitingRspInMs;	/**< time-out of waiting for response in milli-seconds */
	app_sv_cmd_response_handler_t	cmdRspHandler;	/**< command response handler function */
} APP_SV_CMD_INSTANCE_T;


#define CUSTOM_COMMAND_TO_ADD(cmdCode, cmdHandler, isNeedResponse, timeoutWaitingRspInMs, cmdRspHandler)	\
	static APP_SV_CMD_INSTANCE_T cmdCode##_entry __attribute__((used, section(".custom_handler_table"))) = 	\
		{(cmdCode), (cmdHandler), (isNeedResponse), (timeoutWaitingRspInMs), (cmdRspHandler)};

#define CUSTOM_COMMAND_PTR_FROM_ENTRY_INDEX(index)	\
	((APP_SV_CMD_INSTANCE_T *)((uint32_t)__custom_handler_table_start + (index)*sizeof(APP_SV_CMD_INSTANCE_T)))



#ifdef __cplusplus
	}
#endif


#endif // #ifndef __APP_SV_CMD_CODE_H__

