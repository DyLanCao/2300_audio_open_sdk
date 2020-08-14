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
#ifndef __BRIDGE_H__
#define __BRIDGE_H__

#include "hci.h"
#include "ke_msg.h"

extern void bridge_hcif_send_acl(struct ke_msg *msg);
extern void bridge_hcif_recv_acl(HciBuffer * pBuffer);
extern void bridge_free_hciBuffer(HciBuffer *pBuffer);
extern void bridge_hci_ble_event(const HciEvent* event);
extern void bridge_free_token(void * token);
extern U8 bridge_check_ble_handle_valid(U16 handle);
extern void bridge_hcif_send_cmd(struct ke_msg *msg);

#endif
