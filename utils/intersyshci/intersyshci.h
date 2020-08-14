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
/***
 * uart hci
 * YuLongWang @2015
*/

#ifndef UARTHCI_H
#define UARTHCI_H

#if defined(__cplusplus)
extern "C" {
#endif

void BESHCI_BufferAvai(void *packet);
void BESHCI_Open(void);
void BESHCI_Close(void);
void BESHCI_Poll(void);
void BESHCI_LockBuffer(void);
void BESHCI_UNLockBuffer(void);
void BESHCI_SendData(void *packet);
int BESHCI_SendBuffer(uint8_t packet_type, uint8_t *packet, int size);
void BESHCI_SCO_Data_Start(void);
void BESHCI_SCO_Data_Stop(void);
void uartrxtx(void const *argument);

#if defined(__cplusplus)
}
#endif

#endif /* UARTHCI_H */
