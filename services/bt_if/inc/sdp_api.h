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

#ifndef	__SDP_API_H__
#define	__SDP_API_H__

#include "me_api.h"

typedef void sdp_query_token;

/*---------------------------------------------------------------------------
 * sdp_query_type type
 *
 *     Indicates the type of a query when calling SDP_Query.
 */
typedef uint8_t sdp_query_type;

/****************************************************************************
 *
 * Types and Constants
 *
 ****************************************************************************/
/* SDP REQUEST:
 *
 * Use for a ServiceAttributeRequest query. This query should result in
 * a response token of type BQSR_ATTRIB_RESP or BSQR_ERROR_RESP.
 *
 * The query parameters must include the following:
 *
 * 1) The ServiceRecordHandle (a 4-byte integer) that identifies the
 * service to query.
 *
 * 2) The maximum size, in bytes, of attribute data to be returned
 * (a 2-byte integer). Between 7 and 0xFFFF bytes can be specified.
 *
 * 3) A Data Element Sequence of ascending attribute IDs and ranges,
 * specifying the attributes of interest on the remote device.
 * IDs are 16-bit identifiers. Ranges are 32-bit identifiers with the
 * higher 16 bits specifying the first ID in the range, and the lower
 * 16 bits specifying the last ID in the range.
 *
 * SDP RESPONSE:
 *
 * A successful response (BSQR_ATTRIB_RESP) to a BSQR_ATTRIB_REQ
 * query includes the following parameters:
 *
 * 1) The number of bytes in the attribute list parameter (next).
 * This is stored as a 2-byte integer and ranges between 2 and 0xFFFF.
 *
 * 2) A Data Element Sequence of attribute ID and value pairs, ascending
 * by ID. Attributes with non-null values are not included.
 *
 * 3) The ContinuationState parameter.
 *
 * An unsuccessful query results in an error response (BSQR_ERROR_RESP)
 * which has the following parameters:
 *
 * 1) A two-byte ErrorCode
 *
 * 2) An ErrorInfo field (currently undefined).
 *
 * Error codes are expressed using the SdpErrorCode type.
 */
#define BTIF_BSQT_ATTRIB_REQ                 0x04

/* SDP REQUEST:
 *
 * Use for a ServiceSearchAttributeRequest query, which searches for
 * services and attributes simultaneously. This query should
 * result in a response token of type BSQR_SERVICE_SEARCH_ATTRIB_RESP
 * or BSQR_ERROR_RESP.
 *
 * The query parameters must include the following:
 *
 * 1) A Data Element Sequence of UUIDs that specify the service(s)
 * desired. Between 1 and 12 UUIDs must be present.
 *
 * 2) The maximum size, in bytes, of attribute data to be returned
 * (a 2-byte integer). Between 7 and 0xFFFF bytes can be specified.
 *
 * 3) A Data Element Sequence of ascending attribute IDs and ranges,
 * specifying the attributes of interest on the remote device.
 * IDs are 16-bit identifiers. Ranges are 32-bit identifiers with the
 * higher 16 bits specifying the first ID in the range, and the lower
 * 16 bits specifying the last ID in the range.
 *
 * SDP RESPONSE:
 *
 * A successful response (BSQR_SERVICE_SEARCH_ATTRIB_RESP) to a
 * BSQR_SERVICE_SEARCH_ATTRIB_REQ query includes the following
 * parameters:
 *
 * 1) The number of bytes in the attribute list parameter (next).
 * This is stored as a 2-byte integer and ranges between 2 and 0xFFFF.
 *
 * 2) A Data Element Sequence, where each element corresponds to a
 * particular service record matching the original pattern.
 * Each element of the sequence is itself a Data Element Sequence of
 * attribute ID and value pairs, ascending by ID.
 *
 * 3) The ContinuationState parameter.
 *
 * An unsuccessful query results in an error response (BSQR_ERROR_RESP)
 * which has the following parameters:
 *
 * 1) A two-byte ErrorCode
 *
 * 2) An ErrorInfo field (currently undefined).
 *
 * Error codes are expressed using the SdpErrorCode type.
 */
#define BTIF_BSQT_SERVICE_SEARCH_ATTRIB_REQ  0x06

/* End of sdp_query_type */

    /*---------------------------------------------------------------------------
	 * sdp_query_resp type
	 *
	 *	   Indicates the type of the response to a query during the BtCallBack.
	 */
typedef uint8_t sdp_query_resp;

    /* Indicates an error response to the query. Parameters include a two-byte
     * ErrorCode and an ErrorInfo field (currently undefined).
     *
     * Error codes are expressed using the SdpErrorCode type.
     */
#define BTIF_BSQR_ERROR_RESP                 0x01

    /* Indicates a successful response to a BSQR_SERVICE_SEARCH_REQ query.
     *
     * Parameters include the following:
     *
     * 1) The total number of matching service records (a 2-byte integer).
     *
     * 2) The total number of service record handles (a 2-byte integer)
     * expressed in this response. (Additional service records may require
     * a second query containing the ContinuationState parameter.)
     *
     * 3) An array of 32-bit service record handles.
     *
     * 4) The ContinuationState parameter.
     */
#define BTIF_BSQR_SERVICE_SEARCH_RESP        0x03

    /* Indicates a successful response to a BSQR_ATTRIB_REQ query.
     *
     * Parameters include the following:
     *
     * 1) The number of bytes in the attribute list parameter (next).
     * This is stored as a 2-byte integer and ranges between 2 and 0xFFFF.
     *
     * 2) A Data Element Sequence of attribute ID and value pairs, ascending
     * by ID. Attributes with non-null values are not included.
     *
     * 3) The ContinuationState parameter.
     */
#define BTIF_BSQR_ATTRIB_RESP                0x05

    /* Indicates a successful response to a BSQR_SERVICE_SEARCH_ATTRIB_REQ
     * query.
     *
     * Parameters include the following:
     *
     * 1) The number of bytes in the attribute list parameter (next).
     * This is stored as a 2-byte integer and ranges between 2 and 0xFFFF.
     *
     * 2) A Data Element Sequence, where each element corresponds to a
     * particular service record matching the original pattern.
     * Each element of the sequence is itself a Data Element Sequence of
     * attribute ID and value pairs, ascending by ID.
     *
     * 3) The ContinuationState parameter.
     */
#define BTIF_BSQR_SERVICE_SEARCH_ATTRIB_RESP 0x07

/* End of sdp_query_resp */

#if SDP_SERVER_SUPPORT == XA_ENABLED
    /*---------------------------------------------------------------------------
	 *
	 * Number of attributes in the SDP server's SDP record.
	 */
#define BTIF_SDP_SERVER_ATTRIBS 3

    /*---------------------------------------------------------------------------
	 *
	 * SDP_SERVER_TX_SIZE is the size of each transmit buffer used by the
	 * SDP server. The SDP PDU header is 5 bytes.
	 */
#define BTIF_SDP_SERVER_TX_SIZE (SDP_SERVER_SEND_SIZE - 5)

    /*---------------------------------------------------------------------------
	 *
	 * Service Record handle 0x00000000 is reserved for the SDP server's record.
	 * Handles 0x00000001 - 0x0000ffff are reserved. Thus usable record handles
	 * start at 0x00010000
	 */
#define BTIF_SDP_SERV_OWN_HANDLE     0x00000000
#define BTIF_SDP_SERV_FIRST_HANDLE   0x00010000
#define BTIF_SDP_SERV_BOGUS_HANDLE   0xffffffff

    /*---------------------------------------------------------------------------
	 *
	 * SDP Server's Service Database State size.  It contains a one byte header
	 * and a four byte value.
	 */
#define BTIF_SDP_ZERO_SERVICE_DATABASE_STATE_SIZE 5

#endif /* SDP_SERVER_SUPPORT == XA_ENABLED */

/*---------------------------------------------------------------------------
 * sdp_service_class_uuid type
 *
 *     Represents the UUID associated with a specific service and
 *     profile.
 *
 *     Any number of these UUIDs may be present in the
 *     AID_SERVICE_CLASS_ID_LIST attribute of a service record, and may
 *     appear in the AID_BT_PROFILE_DESC_LIST.
 */
typedef uint16_t sdp_service_class_uuid;

/* Service Discovery Server service. */
#define BTIF_SC_SERVICE_DISCOVERY_SERVER             0x1000

/* Browse Group Descriptor service. */
#define BTIF_SC_BROWSE_GROUP_DESC                    0x1001

/* Public Browse Group service. */
#define BTIF_SC_PUBLIC_BROWSE_GROUP                  0x1002

/* Serial Port service and profile. */
#define BTIF_SC_SERIAL_PORT                          0x1101

/* LAN Access over PPP service. */
#define BTIF_SC_LAN_ACCESS_PPP                       0x1102

/* Dial-up networking service and profile. */
#define BTIF_SC_DIALUP_NETWORKING                    0x1103

/* IrMC Sync service and Synchronization profile. */
#define BTIF_SC_IRMC_SYNC                            0x1104

/* OBEX Object Push service and Object Push profile. */
#define BTIF_SC_OBEX_OBJECT_PUSH                     0x1105

/* OBEX File Transfer service and File Transfer profile. */
#define BTIF_SC_OBEX_FILE_TRANSFER                   0x1106

/* IrMC Sync service and Synchronization profile
 * (Sync Command Scenario).
 */
#define BTIF_SC_IRMC_SYNC_COMMAND                    0x1107

/* Headset service and profile. */
#define BTIF_SC_HEADSET                              0x1108

/* Cordless telephony service and profile. */
#define BTIF_SC_CORDLESS_TELEPHONY                   0x1109

/* Audio Source */
#define BTIF_SC_AUDIO_SOURCE                         0x110A

/* Audio Sink */
#define BTIF_SC_AUDIO_SINK                           0x110B

/* Audio/Video Remote Control Target */
#define BTIF_SC_AV_REMOTE_CONTROL_TARGET             0x110C

/* Advanced Audio Distribution Profile */
#define BTIF_SC_AUDIO_DISTRIBUTION                   0x110D

/* Audio/Video Remote Control */
#define BTIF_SC_AV_REMOTE_CONTROL                    0x110E

/* Video Conferencing Profile */
#define BTIF_SC_VIDEO_CONFERENCING                   0x110F

/* Intercom service and profile. */
#define BTIF_SC_INTERCOM                             0x1110

/* Fax service and profile. */
#define BTIF_SC_FAX                                  0x1111

/* Headset Audio Gateway */
#define BTIF_SC_HEADSET_AUDIO_GATEWAY                0x1112

/* WAP service */
#define BTIF_SC_WAP                                  0x1113

/* WAP client service */
#define BTIF_SC_WAP_CLIENT                           0x1114

/* Personal Area Networking Profile */
#define BTIF_SC_PANU                                 0x1115

/* Personal Area Networking Profile */
#define BTIF_SC_NAP                                  0x1116

/* Personal Area Networking Profile */
#define BTIF_SC_GN                                   0x1117

/* Basic Printing Profile */
#define BTIF_SC_DIRECT_PRINTING                      0x1118

/* Basic Printing Profile */
#define BTIF_SC_REFERENCE_PRINTING                   0x1119

/* Imaging Profile */
#define BTIF_SC_IMAGING                              0x111A

/* Imaging Profile */
#define BTIF_SC_IMAGING_RESPONDER                    0x111B

/* Imaging Profile */
#define BTIF_SC_IMAGING_AUTOMATIC_ARCHIVE            0x111C

/* Imaging Profile */
#define BTIF_SC_IMAGING_REFERENCED_OBJECTS           0x111D

/* Handsfree Profile */
#define BTIF_SC_HANDSFREE                            0x111E

/* Handsfree Audio Gateway */
#define BTIF_SC_HANDSFREE_AUDIO_GATEWAY              0x111F

/* Basic Printing Profile */
#define BTIF_SC_DIRECT_PRINTING_REF_OBJECTS          0x1120

/* Basic Printing Profile */
#define BTIF_SC_REFLECTED_UI                         0x1121

/* Basic Printing Profile */
#define BTIF_SC_BASIC_PRINTING                       0x1122

/* Basic Printing Profile */
#define BTIF_SC_PRINTING_STATUS                      0x1123

/* Human Interface Device Profile */
#define BTIF_SC_HUMAN_INTERFACE_DEVICE               0x1124

/* Hardcopy Cable Replacement Profile */
#define BTIF_SC_HCR                                  0x1125

/* Hardcopy Cable Replacement Profile */
#define BTIF_SC_HCR_PRINT                            0x1126

/* Hardcopy Cable Replacement Profile */
#define BTIF_SC_HCR_SCAN                             0x1127

/* Common ISDN Access / CAPI Message Transport Protocol */
#define BTIF_SC_ISDN                                 0x1128

/* Video Conferencing Gateway */
#define BTIF_SC_VIDEO_CONFERENCING_GW                0x1129

/* Unrestricted Digital Information Mobile Termination */
#define BTIF_SC_UDI_MT                               0x112A

/* Unrestricted Digital Information Terminal Adapter */
#define BTIF_SC_UDI_TA                               0x112B

/* Audio Video service */
#define BTIF_SC_AUDIO_VIDEO                          0x112C

/* SIM Access Profile */
#define BTIF_SC_SIM_ACCESS                           0x112D

/* Phonebook Access Client */
#define BTIF_SC_PBAP_CLIENT                          0x112E

/* Phonebook Access Server */
#define BTIF_SC_PBAP_SERVER                          0x112F

/* Phonebook Access Profile Id */
#define BTIF_SC_PBAP_PROFILE                         0x1130

/* Message Access Server */
#define BTIF_SC_MAP_SERVER                           0x1132

/* Message Access Notification Server */
#define BTIF_SC_MAP_NOTIFY_SERVER                    0x1133

/* Message Access Profile */
#define BTIF_SC_MAP_PROFILE                          0x1134

/* Plug-n-Play service */
#define BTIF_SC_PNP_INFO                             0x1200

/* Generic Networking service. */
#define BTIF_SC_GENERIC_NETWORKING                   0x1201

/* Generic File Transfer service. */
#define BTIF_SC_GENERIC_FILE_TRANSFER                0x1202

/* Generic Audio service. */
#define BTIF_SC_GENERIC_AUDIO                        0x1203

/* Generic Telephony service. */
#define BTIF_SC_GENERIC_TELEPHONY                    0x1204

/* UPnP L2CAP based profile. */
#define BTIF_SC_UPNP_SERVICE                         0x1205

/* UPnP IP based profile. */
#define BTIF_SC_UPNP_IP_SERVICE                      0x1206

/* UPnP IP based solution using PAN */
#define BTIF_SC_ESDP_UPNP_IP_PAN                     0x1300

/* UPnP IP based solution using LAP */
#define BTIF_SC_ESDP_UPNP_IP_LAP                     0x1301

/* UPnP L2CAP based solution */
#define BTIF_SC_ESDP_UPNP_L2CAP                      0x1302

/* Video Source */
#define BTIF_SC_VIDEO_SOURCE                         0x1303

/* Video Sink */
#define BTIF_SC_VIDEO_SINK                           0x1304

/* Video Sink */
#define BTIF_SC_VIDEO_DISTRIBUTION                   0x1305

/* End of sdp_service_class_uuid */

/*---------------------------------------------------------------------------
 * sdp_parsing_mode type
 *
 *     Indicates the parsing mode when calling SDP_ParseAttributes.
 */
typedef uint8_t sdp_parsing_mode;

#define BTIF_BSPM_BEGINNING   0x00  /* Used to parse the first SDP response
                                     * packet from the beginning.  This is always
                                     * the first mode used.  This mode uses the
                                     * SdpQueryToken "results" and "rLen" buffer
                                     * fields when it start its parsing.
                                     */

#define BTIF_BSPM_RESUME      0x01  /* Used to parse the same SDP response packet
                                     * from where parsing stopped last.  This mode
                                     * can be used to find an attribute returned
                                     * in multiple SDP records.  This mode uses the
                                     * SdpQueryToken "remainLen" and "remainBuff"
                                     * buffer fields when it starts its parsing.
                                     */

#define BTIF_BSPM_CONT_STATE  0x02  /* Use to parse the beginning of a
                                     * continuation state SDP response packet.
                                     * This mode is only valid after performing an
                                     * SDP query using the BSQM_CONTINUE query mode.
                                     * This mode uses the SdpQueryToken "results"
                                     * and "rLen" buffer fields when it start its
                                     * parsing.
                                     */

#define BTIF_BSPM_NO_SKIP   0x04    /* Combined with BSPM_RESUME or BSPM_CONT_STATE to
                                     * bypass the default parsing behavior which skips
                                     * the previously parsed attribute Id to access subsequent
                                     * SDP records returned in the SDP query response. When
                                     * used, this parsing method will resume from the starting
                                     * point of the resume or continuation state buffer and
                                     * fine the first attribute Id that matches the parsing
                                     * information. Future instances of the same attribute Id in
                                     * the next SDP record (if one exists) can be performed by
                                     * not using this parsing method. NOTE: this mode cannot
                                     * be used by itself, but must be combined with BSPM_RESUME
                                     * or BSPM_CONT_STATE.
                                     */

/* End of sdp_parsing_mode */

/*---------------------------------------------------------------------------
 * sdp_query_mode type
 *
 *     Indicates the mode of a query when calling SDP_Query.
 */
typedef uint8_t sdp_query_mode;

#define BTIF_BSQM_FIRST     0x00    /* Use this mode to issue the first packet in
                                     * an SDP query. When using this mode, SDP will
                                     * internally initialize a 1-byte continuation
                                     * state parameter containing 0x00 and will store
                                     * this information in the "contState" and
                                     * "contStateLen" fields in the SdpQueryToken
                                     * structure.
                                     */

#define BTIF_BSQM_CONTINUE  0x01    /* Use this mode to send the next packet in an
                                     * SDP query. An SDP query takes multiple packets
                                     * when a response is received containing non-zero
                                     * continuation state information. When specifying
                                     * this mode, you must include the continuation
                                     * state information in the "contState" and
                                     * "contStateLen" fields in the SdpQueryToken
                                     * structure.  If SDP_ParseAttributes was used to
                                     * parse the response and BT_STATUS_SDP_CONT_STATE
                                     * was returned, this data will be automatically
                                     * copied into these SdpQueryToken fields.
                                     */

#define BTIF_BSQM_DONT_CARE 0xFF    /* Used internally. */

/* End of sdp_query_mode */

/*---------------------------------------------------------------------------
 * SdpDataElemType type
 *
 *     Specifies the type of a Data Element.
 *
 *     Data Elements begin with a single byte that contains both type and
 *     size information. To read the type from this byte, use the
 *     SDP_GetElemType macro.
 *
 *     To create the first byte of a Data Element, bitwise OR the
 *     SdpDataElemType and SdpDataElemSize values into a single byte.
 */
typedef U8 SdpDataElemType;

#define DETD_NIL  0x00          /* Specifies nil, the null type.
                                 * Requires a size of DESD_1BYTE, which for this type
                                 * means an actual size of 0.
                                 */
#define DETD_UINT 0x08          /* Specifies an unsigned integer. Must use size
                                 * DESD_1BYTE, DESD_2BYTES, DESD_4BYTES, DESD_8BYTES,
                                 * or DESD_16BYTES.
                                 */
#define DETD_SINT 0x10          /* Specifies a signed integer. May use size
                                 * DESD_1BYTE, DESD_2BYTES, DESD_4BYTES, DESD_8BYTES,
                                 * or DESD_16BYTES.
                                 */
#define DETD_UUID 0x18          /* Specifies a Universally Unique Identifier (UUID).
                                 * Must use size DESD_2BYTES, DESD_4BYTES, or
                                 * DESD_16BYTES.
                                 */
#define DETD_TEXT 0x20          /* Specifies a text string. Must use sizes
                                 * DESD_ADD_8BITS, DESD_ADD_16BITS, or DESD_ADD32_BITS.
                                 */
#define DETD_BOOL 0x28          /* Specifies a boolean value. Must use size
                                 * DESD_1BYTE.
                                 */
#define DETD_SEQ  0x30          /* Specifies a data element sequence. The data contains
                                 * a sequence of Data Elements. Must use sizes
                                 * DESD_ADD_8BITS, DESD_ADD_16BITS, or DESD_ADD_32BITS.
                                 */
#define DETD_ALT  0x38          /* Specifies a data element alternative. The data contains
                                 * a sequence of Data Elements. This type is sometimes
                                 * used to distinguish between two possible sequences.
                                 * Must use size DESD_ADD_8BITS, DESD_ADD_16BITS,
                                 * or DESD_ADD_32BITS.
                                 */
#define DETD_URL  0x40          /* Specifies a Uniform Resource Locator (URL).
                                 * Must use size DESD_ADD_8BITS, DESD_ADD_16BITS,
                                 * or DESD_ADD_32BITS.
                                 */

#define DETD_MASK 0xf8          /* AND this value with the first byte of a Data
                                 * Element to return the element's type.
                                 */

/* End of SdpDataElemType */

/*---------------------------------------------------------------------------
 * SdpDataElemSize type
 *
 *     Specifies the size of a Data Element.
 *
 *     Data Elements begin with a single byte that contains both type and
 *     size information. To read the size from this byte, use the
 *     SDP_GetElemSize macro.
 *
 *     To create the first byte of a Data Element, bitwise OR the
 *     SdpDataElemType and SdpDataElemSize values into a single byte.
 *     For example, a standard 16 bit unsigned integer with a value of 0x57
 *     could be encoded as follows:
 *
 *     U8 val[3] = { DETD_UINT | DESD_2BYTES, 0x00, 0x57 };
 *
 *     The text string "hello" could be encoded as follows:
 *
 *     U8 str[7] = { DETD_TEXT | DESD_ADD_8BITS, 0x05, 'h','e','l','l','o' };
 */
typedef uint8_t sdp_data_elem_size;

#define DESD_1BYTE      0x00    /* Specifies a 1-byte element. However, if
                                 * type is DETD_NIL then the size is 0.
                                 */
#define DESD_2BYTES     0x01    /* Specifies a 2-byte element. */
#define DESD_4BYTES     0x02    /* Specifies a 4-byte element. */
#define DESD_8BYTES     0x03    /* Specifies an 8-byte element. */
#define DESD_16BYTES    0x04    /* Specifies a 16-byte element. */
#define DESD_ADD_8BITS  0x05    /* The element's actual data size, in bytes,
                                 * is contained in the next 8 bits.
                                 */
#define DESD_ADD_16BITS 0x06    /* The element's actual data size, in bytes,
                                 * is contained in the next 16 bits.
                                 */
#define DESD_ADD_32BITS 0x07    /* The element's actual data size, in bytes,
                                 * is contained in the next 32 bits.
                                 */

#define DESD_MASK       0x07    /* AND this value with the first byte of a Data
                                 * Element to return the element's size.
                                 */

/* End of  sdp_data_elem_size*/

/*---------------------------------------------------------------------------
 * SDP_ATTRIBUTE()
 *
 *     Macro that formats an SdpAttribute structure using the supplied Attribute ID
 *     and Attribute value. This macro is very useful when formatting the
 *     SdpAttribute structures used to form the attributes in an SDP Record.
 *
 * Parameters:
 *     attribId - SdpAttributeId value (see the AID_ values).
 *     attrib - Array containing the attribute value.
 */
#define SDP_ATTRIBUTE(attribId, attrib) \
          { attribId,           /* Attribute ID */          \
            sizeof(attrib),     /* Attribute Size */        \
            attrib,             /* Attribute Value */       \
            0x0000 }            /* Flag - For Internal Use */

/*---------------------------------------------------------------------------
 * SDP_ATTRIBUTE_ARM()
 *
 *     Macro that formats an SdpAttribute structure using the supplied Attribute ID
 *     and Attribute size. This macro is very useful when formatting the
 *     SdpAttribute structures used to form the attributes in an SDP Record.
 *
 *     This macro is used for ARM compilers that cannot set the attribute value
 *     using a pointer to ROM. The pointer must be set later at run-time after
 *     the structure is copied to RAM.
 *
 * Parameters:
 *     attribId - SdpAttributeId value (see the AID_ values).
 *     attrib - Array containing the attribute value (used to set size only.)
 */
#define SDP_ATTRIBUTE_ARM(attribId, attrib) \
          { attribId,           /* Attribute ID */          \
            sizeof(attrib),     /* Attribute Size */        \
            0,                  /* Attribute Value - to be set later */ \
            0x0000 }            /* Flag - For Internal Use */

/*---------------------------------------------------------------------------
 * SDP_ATTRIB_HEADER_8BIT()
 *
 *     Macro that forms a Data Element Sequence header using the supplied 8-bit
 *     size value.  Data Element Sequence headers are used in SDP Record Attributes
 *     and SDP Queries. Notice that this macro only forms the header portion
 *     of the Data Element Sequence. The actual Data Elements within this
 *     sequence will need to be formed using other SDP macros.
 *
 * Parameters:
 *     size - 8-bit size of the Data Element Sequence.
 */
#define SDP_ATTRIB_HEADER_8BIT(size) \
            DETD_SEQ + DESD_ADD_8BITS,      /* Type & size index 0x35 */ \
            size                /* 8-bit size */
/*---------------------------------------------------------------------------
 * SDP_ATTRIB_HEADER_16BIT()
 *
 *     Macro that forms a Data Element Sequence header using the supplied 16-bit
 *     size value.  Data Element Sequence headers are used in SDP Record Attributes
 *     and SDP Queries. Notice that this macro only forms the header portion
 *     of the Data Element Sequence. The actual Data Elements within this
 *     sequence will need to be formed using other SDP macros.
 *
 * Parameters:
 *     size - 16-bit size of the Data Element Sequence.
 */
#define SDP_ATTRIB_HEADER_16BIT(size) \
            DETD_SEQ + DESD_ADD_16BITS,      /* Type & size index 0x36 */ \
            (U8)(((size) & 0xff00) >> 8),    /* Bits[15:8] of size */     \
            (U8)((size) & 0x00ff)   /* Bits[7:0] of size */

/*---------------------------------------------------------------------------
 * SDP_ATTRIB_HEADER_32BIT()
 *
 *     Macro that forms a Data Element Sequence header using the supplied 32-bit
 *     size value.  Data Element Sequence headers are used in SDP Record Attributes
 *     and SDP Queries. Notice that this macro only forms the header portion
 *     of the Data Element Sequence. The actual Data Elements within this
 *     sequence will need to be formed using other SDP macros.
 *
 * Parameters:
 *     size - 32-bit size of the Data Element Sequence.
 */
#define SDP_ATTRIB_HEADER_32BIT(size) \
            DETD_SEQ + DESD_ADD_32BITS,         /* Type & size index 0x37 */ \
            (U8)(((size) & 0xff000000) >> 24),  /* Bits[32:24] of size */    \
            (U8)(((size) & 0x00ff0000) >> 16),  /* Bits[23:16] of size */    \
            (U8)(((size) & 0x0000ff00) >> 8),   /* Bits[15:8] of size */     \
            (U8)((size) & 0x000000ff)   /* Bits[7:0] of size */

/*---------------------------------------------------------------------------
 * SDP_ATTRIB_HEADER_ALT_8BIT()
 *
 *     Macro that forms a Data Element Sequence Alternative header using the
 *     supplied 8-bit size value.  Data Element Sequence Alternative headers
 *     are used in SDP Record Attributes. Notice that this macro only forms
 *     the header portion of the Data Element Sequence Alternative. The actual
 *     Data Element Sequences within this alternative will need to be formed
 *     using other SDP macros.
 *
 * Parameters:
 *     size - 8-bit size of the Data Element Sequence Alternative.
 */
#define SDP_ATTRIB_HEADER_ALT_8BIT(size) \
            DETD_ALT + DESD_ADD_8BITS,      /* Type & size index 0x35 */ \
            size                /* 8-bit size */

/*---------------------------------------------------------------------------
 * SDP_ATTRIB_HEADER_ALT_16BIT()
 *
 *     Macro that forms a Data Element Sequence Alternative header using the
 *     supplied 16-bit size value.  Data Element Sequence Alternative headers
 *     are used in SDP Record Attributes. Notice that this macro only forms
 *     the header portion of the Data Element Sequence Alternative. The actual
 *     Data Element Sequences within this alternative will need to be formed
 *     using other SDP macros.
 *
 * Parameters:
 *     size - 16-bit size of the Data Element Sequence Alternative.
 */
#define SDP_ATTRIB_HEADER_ALT_16BIT(size) \
            DETD_ALT + DESD_ADD_16BITS,      /* Type & size index 0x36 */ \
            (U8)(((size) & 0xff00) >> 8),    /* Bits[15:8] of size */     \
            (U8)((size) & 0x00ff)   /* Bits[7:0] of size */

/*---------------------------------------------------------------------------
 * SDP_ATTRIB_HEADER_ALT_32BIT()
 *
 *     Macro that forms a Data Element Sequence Alternative header using the
 *     supplied 32-bit size value.  Data Element Sequence Alternative headers
 *     are used in SDP Record Attributes. Notice that this macro only forms
 *     the header portion of the Data Element Sequence Alternative. The actual
 *     Data Element Sequences within this alternative will need to be formed
 *     using other SDP macros.
 *
 * Parameters:
 *     size - 32-bit size of the Data Element Sequence Alternative.
 */
#define SDP_ATTRIB_HEADER_ALT_32BIT(size) \
            DETD_ALT + DESD_ADD_32BITS,         /* Type & size index 0x37 */ \
            (U8)(((size) & 0xff000000) >> 24),  /* Bits[32:24] of size */    \
            (U8)(((size) & 0x00ff0000) >> 16),  /* Bits[23:16] of size */    \
            (U8)(((size) & 0x0000ff00) >> 8),   /* Bits[15:8] of size */     \
            (U8)((size) & 0x000000ff)   /* Bits[7:0] of size */

/*---------------------------------------------------------------------------
 * SDP_UUID_16BIT()
 *
 *     Macro that forms a UUID Data Element from the supplied 16-bit UUID value.
 *     UUID data elements are used in SDP Record Attributes and SDP Queries.
 *
 * Parameters:
 *     uuid - 16-bit UUID value (see the SC_ and PROT_ values).
 */
#define SDP_UUID_16BIT(uuid) \
            DETD_UUID + DESD_2BYTES,         /* Type & size index 0x19 */ \
            (U8)(((uuid) & 0xff00) >> 8),    /* Bits[15:8] of UUID */     \
            (U8)((uuid) & 0x00ff)   /* Bits[7:0] of UUID */

/*---------------------------------------------------------------------------
 * SDP_UUID_32BIT()
 *
 *     Macro that forms a UUID Data Element from the supplied 32-bit UUID value.
 *     UUID data elements are used in SDP Record Attributes and SDP Queries.
 *
 * Parameters:
 *     uuid - 32-bit UUID value (see the SC_ and PROT_ values for 16-bit
 *            values). 16-bit UUID values can be converted to 32-bit by
 *            zero extending the 16-bit value.
 */
#define SDP_UUID_32BIT(uuid) \
            DETD_UUID + DESD_4BYTES,            /* Type & size index 0x1A */ \
            (U8)(((uuid) & 0xff000000) >> 24),  /* Bits[32:24] of UUID */    \
            (U8)(((uuid) & 0x00ff0000) >> 16),  /* Bits[23:16] of UUID */    \
            (U8)(((uuid) & 0x0000ff00) >> 8),   /* Bits[15:8] of UUID */     \
            (U8)((uuid) & 0x000000ff)   /* Bits[7:0] of UUID */

/*---------------------------------------------------------------------------
 * SDP_UUID_128BIT()
 *
 *     Macro that forms a UUID Data Element from the supplied 128-bit UUID value.
 *     UUID data elements are used in SDP Record Attributes and SDP Queries.
 *
 * Parameters:
 *     uuid - 128-bit UUID value (see the SC_ and PROT_ values for 16-bit
 *            values). 16-bit UUID values can be converted to 128-bit using
 *            the following conversion: 128_bit_value = 16_bit_value * 2^96 +
 *            Bluetooth Base UUID.
 */
#define SDP_UUID_128BIT(uuid)                /* UUID must be a 16-byte array */ \
            DETD_UUID + DESD_16BYTES,        /* Type & size index 0x1C */ \
            (U8)(uuid[15]), /* Bits[128:121] of UUID */ \
            (U8)(uuid[14]), \
            (U8)(uuid[13]), \
            (U8)(uuid[12]), \
            (U8)(uuid[11]), \
            (U8)(uuid[10]), \
            (U8)(uuid[9]),  \
            (U8)(uuid[8]),  \
            (U8)(uuid[7]),  \
            (U8)(uuid[6]),  \
            (U8)(uuid[5]),  \
            (U8)(uuid[4]),  \
            (U8)(uuid[3]),  \
            (U8)(uuid[2]),  \
            (U8)(uuid[1]),  \
            (U8)(uuid[0])       /* Bits[7:0] of UUID */
/*---------------------------------------------------------------------------
 * SDP_UINT_8BIT()
 *
 *     Macro that forms a UINT Data Element from the supplied 8-bit UINT value.
 *     UINT data elements are used in SDP Record Attributes and SDP Queries.
 *
 * Parameters:
 *     uint - 8-bit UINT value.
 */
#define SDP_UINT_8BIT(uint) \
            DETD_UINT + DESD_1BYTE,          /* Type & size index 0x08 */ \
            (U8)(uint)          /* 8-bit UINT */

/*---------------------------------------------------------------------------
 * SDP_UINT_16BIT()
 *
 *     Macro that forms a UINT Data Element from the supplied 16-bit UINT value.
 *     UINT data elements are used in SDP Record Attributes and SDP Queries.
 *
 * Parameters:
 *     uint - 16-bit UINT value.
 */
#define SDP_UINT_16BIT(uint) \
            DETD_UINT + DESD_2BYTES,         /* Type & size index 0x09 */ \
            (U8)(((uint) & 0xff00) >> 8),    /* Bits[15:8] of UINT */     \
            (U8)((uint) & 0x00ff)   /* Bits[7:0] of UINT */

/*---------------------------------------------------------------------------
 * SDP_UINT_32BIT()
 *
 *     Macro that forms a UINT Data Element from the supplied 32-bit UINT value.
 *     UINT data elements are used in SDP Record Attributes and SDP Queries.
 *
 * Parameters:
 *     uint - 32-bit UINT value.
 */
#define SDP_UINT_32BIT(uint) \
            DETD_UINT + DESD_4BYTES,            /* Type & size index 0x0A */ \
            (U8)(((uint) & 0xff000000) >> 24),  /* Bits[31:24] of UINT */    \
            (U8)(((uint) & 0x00ff0000) >> 16),  /* Bits[23:16] of UINT */    \
            (U8)(((uint) & 0x0000ff00) >> 8),   /* Bits[15:8] of UINT */     \
            (U8)((uint) & 0x000000ff)   /* Bits[7:0] of UINT */

/*---------------------------------------------------------------------------
 * SDP_UINT_64BIT()
 *
 *     Macro that forms a UINT Data Element from the supplied 64-bit UINT value.
 *     UINT data elements are used in SDP Record Attributes and SDP Queries.
 *
 * Parameters:
 *     uint - 64-bit UINT value.
 */
#define SDP_UINT_64BIT(uint)                    /* UINT must be an 8-byte array */ \
            DETD_UINT + DESD_8BYTES,            /* Type & size index 0x0B */ \
            uint                /* 64-bit UINT */

/*---------------------------------------------------------------------------
 * SDP_UINT_128BIT()
 *
 *     Macro that forms a UINT Data Element from the supplied 128-bit UINT value.
 *     UINT data elements are used in SDP Record Attributes and SDP Queries.
 *
 * Parameters:
 *     uint - 128-bit UINT value.
 */
#define SDP_UINT_128BIT(uint)                   /* UINT must be a 16-byte array */ \
            DETD_UINT + DESD_16BYTES,           /* Type & size index 0x0C */ \
            uint                /* 128-bit UINT */

/*---------------------------------------------------------------------------
 * SDP_TEXT_8BIT()
 *
 *     Macro that forms a TEXT Data Element Header from the supplied 8-bit size
 *     value. TEXT data elements are used in SDP Record Attributes and SDP Queries.
 *     Notice that this macro only forms the header portion of the TEXT Data
 *     Element. The actual TEXT data within this data element will need to
 *     be provided separately.
 *
 * Parameters:
 *     size - 8-bit size value.
 */
#define SDP_TEXT_8BIT(size) \
            DETD_TEXT + DESD_ADD_8BITS,      /* Type & size index 0x25 */ \
            (U8)(size)          /* 8-bit size */

/*---------------------------------------------------------------------------
 * SDP_TEXT_16BIT()
 *
 *     Macro that forms a TEXT Data Element Header from the supplied 16-bit size
 *     value. TEXT data elements are used in SDP Record Attributes and SDP Queries.
 *     Notice that this macro only forms the header portion of the TEXT Data
 *     Element. The actual TEXT data within this data element will need to
 *     be provided separately.
 *
 * Parameters:
 *     size - 16-bit size value.
 */
#define SDP_TEXT_16BIT(size) \
            DETD_TEXT + DESD_ADD_16BITS,      /* Type & size index 0x26 */ \
            (U8)(((size) & 0xff00) >> 8),     /* Bits[15:8] of size */     \
            (U8)((size) & 0x00ff)   /* Bits[7:0] of size */

/*---------------------------------------------------------------------------
 * SDP_TEXT_32BIT()
 *
 *     Macro that forms a TEXT Data Element Header from the supplied 32-bit size
 *     value. TEXT data elements are used in SDP Record Attributes and SDP Queries.
 *     Notice that this macro only forms the header portion of the TEXT Data
 *     Element. The actual TEXT data within this data element will need to
 *     be provided separately.
 *
 * Parameters:
 *     size - 32-bit size value.
 */
#define SDP_TEXT_32BIT(size) \
            DETD_TEXT + DESD_ADD_32BITS,        /* Type & size index 0x27 */ \
            (U8)(((size) & 0xff000000) >> 24),  /* Bits[32:24] of size */    \
            (U8)(((size) & 0x00ff0000) >> 16),  /* Bits[23:16] of size */    \
            (U8)(((size) & 0x0000ff00) >> 8),   /* Bits[15:8] of size */     \
            (U8)((size) & 0x000000ff)   /* Bits[7:0] of size */

/*---------------------------------------------------------------------------
 * SDP_BOOL()
 *
 *     Macro that forms a BOOL Data Element from the supplied 8-bit boolean value.
 *     BOOL data elements are used in SDP Record Attributes and SDP Queries.
 *
 * Parameters:
 *     value - 8-bit boolean value.
 */
#define SDP_BOOL(value) \
            DETD_BOOL + DESD_1BYTE,          /* Type & size index 0x28 */ \
            (U8)(value)         /* Boolean value */

/*---------------------------------------------------------------------------
 * sdp_service_class_uuid type
 *
 *     Represents the UUID associated with a specific service and
 *     profile.
 *
 *     Any number of these UUIDs may be present in the
 *     AID_SERVICE_CLASS_ID_LIST attribute of a service record, and may
 *     appear in the AID_BT_PROFILE_DESC_LIST.
 */
typedef uint16_t sdp_service_class_uuid;

/* Service Discovery Server service. */
#define SC_SERVICE_DISCOVERY_SERVER             0x1000

/* Browse Group Descriptor service. */
#define SC_BROWSE_GROUP_DESC                    0x1001

/* Public Browse Group service. */
#define SC_PUBLIC_BROWSE_GROUP                  0x1002

/* Serial Port service and profile. */
#define SC_SERIAL_PORT                          0x1101

/* LAN Access over PPP service. */
#define SC_LAN_ACCESS_PPP                       0x1102

/* Dial-up networking service and profile. */
#define SC_DIALUP_NETWORKING                    0x1103

/* IrMC Sync service and Synchronization profile. */
#define SC_IRMC_SYNC                            0x1104

/* OBEX Object Push service and Object Push profile. */
#define SC_OBEX_OBJECT_PUSH                     0x1105

/* OBEX File Transfer service and File Transfer profile. */
#define SC_OBEX_FILE_TRANSFER                   0x1106

/* IrMC Sync service and Synchronization profile
 * (Sync Command Scenario).
 */
#define SC_IRMC_SYNC_COMMAND                    0x1107

/* Headset service and profile. */
#define SC_HEADSET                              0x1108

/* Cordless telephony service and profile. */
#define SC_CORDLESS_TELEPHONY                   0x1109

/* Audio Source */
#define SC_AUDIO_SOURCE                         0x110A

/* Audio Sink */
#define SC_AUDIO_SINK                           0x110B

/* Audio/Video Remote Control Target */
#define SC_AV_REMOTE_CONTROL_TARGET             0x110C

/* Advanced Audio Distribution Profile */
#define SC_AUDIO_DISTRIBUTION                   0x110D

/* Audio/Video Remote Control */
#define SC_AV_REMOTE_CONTROL                    0x110E

/* Video Conferencing Profile */
#define SC_VIDEO_CONFERENCING                   0x110F

/* Intercom service and profile. */
#define SC_INTERCOM                             0x1110

/* Fax service and profile. */
#define SC_FAX                                  0x1111

/* Headset Audio Gateway */
#define SC_HEADSET_AUDIO_GATEWAY                0x1112

/* WAP service */
#define SC_WAP                                  0x1113

/* WAP client service */
#define SC_WAP_CLIENT                           0x1114

/* Personal Area Networking Profile */
#define SC_PANU                                 0x1115

/* Personal Area Networking Profile */
#define SC_NAP                                  0x1116

/* Personal Area Networking Profile */
#define SC_GN                                   0x1117

/* Basic Printing Profile */
#define SC_DIRECT_PRINTING                      0x1118

/* Basic Printing Profile */
#define SC_REFERENCE_PRINTING                   0x1119

/* Imaging Profile */
#define SC_IMAGING                              0x111A

/* Imaging Profile */
#define SC_IMAGING_RESPONDER                    0x111B

/* Imaging Profile */
#define SC_IMAGING_AUTOMATIC_ARCHIVE            0x111C

/* Imaging Profile */
#define SC_IMAGING_REFERENCED_OBJECTS           0x111D

/* Handsfree Profile */
#define SC_HANDSFREE                            0x111E

/* Handsfree Audio Gateway */
#define SC_HANDSFREE_AUDIO_GATEWAY              0x111F

/* Basic Printing Profile */
#define SC_DIRECT_PRINTING_REF_OBJECTS          0x1120

/* Basic Printing Profile */
#define SC_REFLECTED_UI                         0x1121

/* Basic Printing Profile */
#define SC_BASIC_PRINTING                       0x1122

/* Basic Printing Profile */
#define SC_PRINTING_STATUS                      0x1123

/* Human Interface Device Profile */
#define SC_HUMAN_INTERFACE_DEVICE               0x1124

/* Hardcopy Cable Replacement Profile */
#define SC_HCR                                  0x1125

/* Hardcopy Cable Replacement Profile */
#define SC_HCR_PRINT                            0x1126

/* Hardcopy Cable Replacement Profile */
#define SC_HCR_SCAN                             0x1127

/* Common ISDN Access / CAPI Message Transport Protocol */
#define SC_ISDN                                 0x1128

/* Video Conferencing Gateway */
#define SC_VIDEO_CONFERENCING_GW                0x1129

/* Unrestricted Digital Information Mobile Termination */
#define SC_UDI_MT                               0x112A

/* Unrestricted Digital Information Terminal Adapter */
#define SC_UDI_TA                               0x112B

/* Audio Video service */
#define SC_AUDIO_VIDEO                          0x112C

/* SIM Access Profile */
#define SC_SIM_ACCESS                           0x112D

/* Phonebook Access Client */
#define SC_PBAP_CLIENT                          0x112E

/* Phonebook Access Server */
#define SC_PBAP_SERVER                          0x112F

/* Phonebook Access Profile Id */
#define SC_PBAP_PROFILE                         0x1130

/* Message Access Server */
#define SC_MAP_SERVER                           0x1132

/* Message Access Notification Server */
#define SC_MAP_NOTIFY_SERVER                    0x1133

/* Message Access Profile */
#define SC_MAP_PROFILE                          0x1134

/* Plug-n-Play service */
#define SC_PNP_INFO                             0x1200

/* Generic Networking service. */
#define SC_GENERIC_NETWORKING                   0x1201

/* Generic File Transfer service. */
#define SC_GENERIC_FILE_TRANSFER                0x1202

/* Generic Audio service. */
#define SC_GENERIC_AUDIO                        0x1203

/* Generic Telephony service. */
#define SC_GENERIC_TELEPHONY                    0x1204

/* UPnP L2CAP based profile. */
#define SC_UPNP_SERVICE                         0x1205

/* UPnP IP based profile. */
#define SC_UPNP_IP_SERVICE                      0x1206

/* UPnP IP based solution using PAN */
#define SC_ESDP_UPNP_IP_PAN                     0x1300

/* UPnP IP based solution using LAP */
#define SC_ESDP_UPNP_IP_LAP                     0x1301

/* UPnP L2CAP based solution */
#define SC_ESDP_UPNP_L2CAP                      0x1302

/* Video Source */
#define SC_VIDEO_SOURCE                         0x1303

/* Video Sink */
#define SC_VIDEO_SINK                           0x1304

/* Video Sink */
#define SC_VIDEO_DISTRIBUTION                   0x1305

/* End of sdp_service_class_uuid */

/*---------------------------------------------------------------------------
 * sdp_protocol_uuid type
 *
 *     Represents the UUID associated with a protocol.
 *
 *     Any number of these UUIDs may be present in the
 *     AID_SERVICE_CLASS_ID_LIST attribute of a service record, and may
 *     appear in the AID_BT_PROFILE_DESC_LIST.
 */
typedef uint16_t sdp_protocol_uuid;

/* Service Discovery Protocol */
#define PROT_SDP                     0x0001

/* UDP Protocol */
#define PROT_UDP                     0x0002

/* RFCOMM Protocol */
#define PROT_RFCOMM                  0x0003

/* TCP Protocol */
#define PROT_TCP                     0x0004

/* TCS Binary Protocol */
#define PROT_TCS_BIN                 0x0005

/* TCS-AT Protocol */
#define PROT_TCS_AT                  0x0006

/* OBEX Protocol */
#define PROT_OBEX                    0x0008

/* IP Protocol */
#define PROT_IP                      0x0009

/* FTP Protocol */
#define PROT_FTP                     0x000A

/* HTTP Protocol */
#define PROT_HTTP                    0x000C

/* WSP Protocol */
#define PROT_WSP                     0x000E

/* BNEP Protocol */
#define PROT_BNEP                    0x000F

/* Universal Plug and Play */
#define PROT_UPNP                    0x0010

/* Human Interface Device Profile */
#define PROT_HIDP                    0x0011

/* Hardcopy Cable Replacement Control Channel */
#define PROT_HCR_CONTROL_CHANNEL     0x0012

/* Hardcopy Cable Replacement Data Channel */
#define PROT_HCR_DATA_CHANNEL        0x0014

/* Hardcopy Cable Replacement Notification*/
#define PROT_HCR_NOTIFICATION        0x0016

/* Audio/Video Control Transport Protocol */
#define PROT_AVCTP                   0x0017

/* Audio/Video Distribution Transport Protocol */
#define PROT_AVDTP                   0x0019

/* Audio/Video Control Transport Protocol Browsing Channel*/
#define PROT_AVCTP_BROWSING          0x001B

/* Unrestricted Digital Information Control Plane */
#define PROT_UDI_C                   0x001D

/* L2CAP Protocol */
#define PROT_L2CAP                   0x0100

/* End of sdp_protocol_uuid */

/*---------------------------------------------------------------------------
 * sdp_attribute_id type
 *
 *     Represents an attribute ID.
 *
 *     Attribute are identified by attribute ID. This type includes
 *     many (but not all) of the possible attributes available on
 *     a Bluetooth device.
 *
 *     Higher layer services may use these attributes, but may also
 *     need to define their own. In this case, the service must define
 *     attributes with IDs between 0x0200 through 0xFFFF.
 */
typedef uint16_t sdp_attribute_id;

/* Group: The following attributes are required to be present in all
 * service records on all Bluetooth devices.
 */

/* A 32-bit UINT that uniquely identifies the service record for a
 * particular SDP server.
 */
#define AID_SERVICE_RECORD_HANDLE               0x0000

/* A Data Element Sequence of UUIDs. Each UUID represents a service
 * class supported by the service record. At least one UUID must
 * be present.
 *
 * The SdpServiceClassUuid type represents these UUIDs.
 */
#define AID_SERVICE_CLASS_ID_LIST               0x0001

/* Group: The following attributes are "universal" to all service records,
 * meaning that the same attribute IDs are always used. However, attributes
 * may or may not be present within a service record.
 *
 * See the Bluetooth Core specification, Service Discovery Protocol (SDP)
 * chapter, section 5.1 for more detailed explanations of these attributes.
 */

#define AID_SERVICE_RECORD_STATE                0x0002
#define AID_SERVICE_ID                          0x0003
#define AID_PROTOCOL_DESC_LIST                  0x0004
#define AID_BROWSE_GROUP_LIST                   0x0005
#define AID_LANG_BASE_ID_LIST                   0x0006
#define AID_SERVICE_INFO_TIME_TO_LIVE           0x0007
#define AID_SERVICE_AVAILABILITY                0x0008
#define AID_BT_PROFILE_DESC_LIST                0x0009
#define AID_DOC_URL                             0x000a
#define AID_CLIENT_EXEC_URL                     0x000b
#define AID_ICON_URL                            0x000c
#define AID_ADDITIONAL_PROT_DESC_LISTS          0x000d

/* Group: The following "universal" attribute IDs must be added to
 * the appropriate value from the AID_LANG_BASE_ID_LIST attribute (usually
 * 0x0100).
 */
#define AID_SERVICE_NAME                        0x0000
#define AID_SERVICE_DESCRIPTION                 0x0001
#define AID_PROVIDER_NAME                       0x0002

/* Personal Area Networking Profile */
#define AID_IP_SUBNET                           0x0200

/* Group: The following attribute applies only to a service record that
 * corresponds to a BrowseGroupDescriptor service.
 */

/* A UUID used to locate services that are part of the browse group. */
#define AID_GROUP_ID                            0x0200

/* Group: The following attributes apply only to the service record that
 * corresponds to the Service Discovery Server itself. Therefore, they
 * are valid only when the AID_SERVICE_CLASS_ID_LIST contains
 * a UUID of SC_SERVICE_DISCOVERY_SERVER.
 */
#define AID_VERSION_NUMBER_LIST                 0x0200
#define AID_SERVICE_DATABASE_STATE              0x0201

/* Group: The following attributes are for use by specific profiles as
 * defined in the profile specification.
 */
#define AID_SERVICE_VERSION                     0x0300

/* Cordless Telephony Profile */
#define AID_EXTERNAL_NETWORK                    0x0301

/* Synchronization Profile */
#define AID_SUPPORTED_DATA_STORES_LIST          0x0301

/* Fax Class 1 */
#define AID_FAX_CLASS_1_SUPPORT                 0x0302

/* GAP Profile */
#define AID_REMOTE_AUDIO_VOL_CONTROL            0x0302

/* Fax Class 2.0 */
#define AID_FAX_CLASS_20_SUPPORT                0x0303

/* Object Push Profile */
#define AID_SUPPORTED_FORMATS_LIST              0x0303

/* Fax Service Class 2 - Manufacturer specific */
#define AID_FAX_CLASS_2_SUPPORT                 0x0304
#define AID_AUDIO_FEEDBACK_SUPPORT              0x0305

/* Bluetooth as WAP requirements */
#define AID_NETWORK_ADDRESS                     0x0306
#define AID_WAP_GATEWAY                         0x0307
#define AID_HOME_PAGE_URL                       0x0308
#define AID_WAP_STACK_TYPE                      0x0309

/* Personal Area Networking Profile */
#define AID_SECURITY_DESC                       0x030A
#define AID_NET_ACCESS_TYPE                     0x030B
#define AID_MAX_NET_ACCESS_RATE                 0x030C
#define AID_IPV4_SUBNET                         0x030D
#define AID_IPV6_SUBNET                         0x030E

/* Imaging Profile */
#define AID_SUPPORTED_CAPABILITIES              0x0310
#define AID_SUPPORTED_FEATURES                  0x0311
#define AID_SUPPORTED_FUNCTIONS                 0x0312
#define AID_TOTAL_IMAGE_DATA_CAPACITY           0x0313

/* Phonebook Access Profile */
#define AID_SUPPORTED_REPOSITORIES              0x0314

/* Message Access Profile */
#define AID_MAS_INSTANCE_ID                     0x0315
#define AID_SUPPORTED_MESSAGE_TYPES             0x0316

/* Basic Printing Profile */
#define AID_SUPPORTED_DOC_FORMATS               0x0350
#define AID_SUPPORTED_CHAR_REPERTOIRES          0x0352
#define AID_SUPPORTED_XHTML_IMAGE_FORMATS       0x0354
#define AID_COLOR_SUPPORTED                     0x0356
#define AID_PRINTER_1284ID                      0x0358
#define AID_DUPLEX_SUPPORTED                    0x035E
#define AID_SUPPORTED_MEDIA_TYPES               0x0360
#define AID_MAX_MEDIA_WIDTH                     0x0362
#define AID_MAX_MEDIA_LENGTH                    0x0364

/*HID PROFILE */

#define AID_HID_PARSERVERSION     0x201
#define AID_HID_DEVICESUBCLASS     0x202
#define AID_HID_CONTRYCODE         0x203
#define AID_HID_VIRTUALCABLE         0x204
#define AID_HID_RECONNECTINITIATE         0x205
#define AID_HID_DESCRIPTORLIST         0x206
#define AID_HID_LANGIDBASELIST         0x207
#define AID_HID_BATTERYPOWER         0x209
#define AID_HID_REMOTEWAKE         0x20A
#define AID_HID_SUPERVISIONTIMEOUT         0x20C
#define AID_HID_NORMALLYCONNECTABLE         0x20D
#define AID_HID_BOOTDEVICE         0x20E
#define AID_HID_SSRHOSTMAXLATENCY         0x20F
#define AID_HID_SSRHOSTMINTIMEOUT         0x210

/* End of sdp_attribute_id */

/*---------------------------------------------------------------------------
 * sdp_get_u16()
 *
 *     Reads a 16-bit integer from the SDP results buffer.
 *
 * Parameters:
 *     buff - The location in the results buffer of a 16-bit integer
 *
 * Returns:
 *     A 16-bit value.
 */
U16 sdp_get_u16(U8 * buff);

#define sdp_get_u16(buff) be_to_host16((buff))

/*---------------------------------------------------------------------------
 * sdp_get_u32()
 *
 *     Reads a 32-bit integer from the SDP results buffer.
 *
 * Parameters:
 *     buff - The location in the results buffer of a 32-bit integer
 *
 * Returns:
 *     A 32-bit value.
 */
U32 sdp_get_u32(U8 * buff);

#define sdp_get_u32(buff) BEtoHost32((buff))

/*---------------------------------------------------------------------------
 * sdp_put_u16()
 *
 *     Writes a 16-bit integer into an SDP buffer.
 *
 * Parameters:
 *     buff - The write location
 *
 *     val  - The integer to write
 */
void sdp_put_u16(U8 * buff, U16 val);

#define sdp_put_u16(buff,val) StoreBE16((buff),(val))

/*---------------------------------------------------------------------------
 * sdp_put_u32()
 *
 *     Writes a 32-bit integer into an SDP buffer.
 *
 * Parameters:
 *     buff - The write location
 *
 *     val  - The integer to write
 */
void sdp_put_u32(U8 * buff, U32 val);

#define sdp_put_u32(buff,val) StoreBE32((buff),(val))

typedef struct {
    uint16_t id;                /* Attribute ID. */

    uint16_t len;               /* Length of the value buffer */

    const uint8_t *value;       /* An array of bytes that contains the value
                                 * of the attribute. The buffer is in
                                 * Data Element form (see SdpDataElemType
                                 * and SdpDataElemSize).
                                 */
    /* Group: The following field is for internal use only */
    uint16_t flags;
} sdp_attribute_t;

typedef struct {
    list_entry_t node;          /* For internal use only. */

    uint8_t num;                /* Total number of attributes in "attribs". */

    /* Pointer to an array of attributes.
     *
     * Store the attributes in ascending order by attribute ID, and
     * do not add two attributes with the same ID.
     *
     * Do not include an AID_SERVICE_RECORD_HANDLE attribute. This
     * attribute is handled automatically by SDP.
     */
    sdp_attribute_t *attribs;

    /* The service class fields of the class of device. Use the values defined
     * in me_api.h. The device class portion is ignored.
     */
    btif_class_of_device_t classOfDevice;

    /* Group: The following fields are for internal use only. */
    uint32_t handle;            /* Service Record Handle */
    uint32_t recordState;       /* Service Record State */
    uint16_t flags;             /* Flag to keep track of marked attributes */
    uint16_t handleFlag;        /* Flag to keep track of service record handle */
    uint16_t stateFlag;         /* Flag to keep track of service record state */
} sdp_record_t;

struct sdp_query_token_info {

    const uint8_t *parms;       /* Parameters to be sent with the query.
                                 * The caller of SDP_Query must make sure
                                 * that this buffer is composed correctly.
                                 */
    uint16_t parm_len;          /* Number of bytes in the "parms" buffer. */
    sdp_query_type query_type;  /* Type of query to send. */
    sdp_attribute_id attr_id;   /* Attribute ID to search for */
    uint16_t uuid;              /* UUID to search for (use 0 if unused) */
    sdp_parsing_mode mode;      /* SDP parsing mode (see SdpParsingMode) */
    btif_callback callback;  /* Function to call with results */
    void *priv;
};

#ifdef __cplusplus
extern "C" {
#endif

    sdp_query_token *btif_sdp_create_query_token(void);

    void btif_sdp_free_token(sdp_query_token * query_token);

    int btif_sdp_set_query_token(sdp_query_token * query_token, struct sdp_query_token_info *info);

    int btif_sdp_set_remote_device(sdp_query_token * query_token,
                                   btif_remote_device_t * r_device);

    bt_status_t btif_sdp_query(sdp_query_token * query_token, sdp_query_mode mode);

    bt_status_t btif_sdp_parse_attrs(sdp_query_token * query_token);

    btif_remote_device_t *btif_sdp_get_remote_device(sdp_query_token * query_token);

    uint8_t btif_sdp_get_server_id(sdp_query_token * query_token);

    void *btif_sdp_get_token_priv(sdp_query_token * query_token);

    bt_status_t btif_sdp_add_record(sdp_record_t * record);

    bt_status_t btif_sdp_remove_record(sdp_record_t * record);

    void btif_sdp_set_parsing_mode(sdp_query_token * query_token, sdp_parsing_mode mode);

    sdp_query_token *btif_me_get_callback_event_sdp_token(const btif_event_t *event);
#ifdef __cplusplus
}
#endif
#endif              /*__SDP_API_H__*/
