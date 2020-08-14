#ifdef __AMA_VOICE__

#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "spp.h"
#include "sdp_api.h"

#include "ai_spp.h"
#include "ai_manager.h"


/****************************************************************************
 * SPP SDP Entries
 ****************************************************************************/

/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList 
 */


/* Extend UUID */
/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList
 */
//#define IOS_IAP2_PROTOCOL //for test
#ifdef IOS_IAP2_PROTOCOL
//ios uuid
static const U8 AMA_SPP_128UuidClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(17),        /* Data Element Sequence, 17 bytes */
    DETD_UUID + DESD_16BYTES,        /* Type & size index 0x1C */ \
            0xFF, /* Bits[128:121] of AMA UUID */ \
            0xCA, \
            0xCA, \
            0xDE, \
            0xAF, \
            0xDE, \
            0xCA, \
            0xDE, \
            0xDE, \
            0xFA, \
            0xCA, \
            0xDE, \
            0x00, \
            0x00, \
            0x00, \
            0x00,
};
#else
static const U8 AMA_SPP_128UuidClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(17),        /* Data Element Sequence, 17 bytes */
    DETD_UUID + DESD_16BYTES,        /* Type & size index 0x1C */ \
            0x93, /* Bits[128:121] of AMA UUID */ \
            0x1C, \
            0x7E, \
            0x8A, \
            0x54, \
            0x0F, \
            0x46, \
            0x86, \
            0xB7, \
            0x98, \
            0xE8, \
            0xDF, \
            0x0A, \
            0x2A, \
            0xD9, \
            0xF7,
};

#endif
static const U8 AMA_SPP_128UuidProtoDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(12),  /* Data element sequence, 12 bytes */ 

    /* Each element of the list is a Protocol descriptor which is a 
     * data element sequence. The first element is L2CAP which only 
     * has a UUID element.  
     */ 
    SDP_ATTRIB_HEADER_8BIT(3),   /* Data element sequence for L2CAP, 3 
                                  * bytes 
                                  */ 

    SDP_UUID_16BIT(PROT_L2CAP),  /* Uuid16 L2CAP */ 

    /* Next protocol descriptor in the list is RFCOMM. It contains two 
     * elements which are the UUID and the channel. Ultimately this 
     * channel will need to filled in with value returned by RFCOMM.  
     */ 

    /* Data element sequence for RFCOMM, 5 bytes */
    SDP_ATTRIB_HEADER_8BIT(5),

    SDP_UUID_16BIT(PROT_RFCOMM), /* Uuid16 RFCOMM */

    /* Uint8 RFCOMM channel number - value can vary */
    SDP_UINT_8BIT(RFCOMM_CHANNEL_AMA)
};

/*
 * * OPTIONAL * Language BaseId List (goes with the service name).  
 */ 
static const U8 AMA_SPP_128UuidLangBaseIdList[] = {
    SDP_ATTRIB_HEADER_8BIT(9),  /* Data Element Sequence, 9 bytes */ 
    SDP_UINT_16BIT(0x656E),     /* English Language */ 
    SDP_UINT_16BIT(0x006A),     /* UTF-8 encoding */ 
    SDP_UINT_16BIT(0x0100)      /* Primary language base Id */ 
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 AMA_SPP_128UuidServiceName[] = {
    SDP_TEXT_8BIT(8),          /* Null terminated text string */ 
    'U', 'U', 'I', 'D', '1', '2', '8', '\0'
};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t AMA_SPP_128UuidSdpAttributes[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, AMA_SPP_128UuidClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, AMA_SPP_128UuidProtoDescList),

    /* Language base id (Optional: Used with service name) */ 
    SDP_ATTRIBUTE(AID_LANG_BASE_ID_LIST, AMA_SPP_128UuidLangBaseIdList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), AMA_SPP_128UuidServiceName),
};

void AMA_SPP_Register128UuidSdpServices(struct _spp_dev *dev)
{
    dev = dev;

    /* Register 128bit UUID SDP Attributes */
    SPP(rf_service).serviceId = RFCOMM_CHANNEL_AMA;
    SPP(numPorts) = 0;

    SPP(sdpRecord)->attribs = (sdp_attribute_t *)(&AMA_SPP_128UuidSdpAttributes[0]);
    SPP(sdpRecord)->num = ARRAY_SIZE(AMA_SPP_128UuidSdpAttributes);
    SPP(sdpRecord)->classOfDevice = BTIF_COD_MAJOR_PERIPHERAL;
    
    TRACE("%s serviceId %d", __func__, SPP(rf_service).serviceId);
}

AI_SPP_TO_ADD(AI_SPEC_AMA, AMA_SPP_Register128UuidSdpServices);

#endif

