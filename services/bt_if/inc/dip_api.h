#ifndef __DIP_API__H__
#define __DIP_API__H__

#ifdef BTIF_DIP_DEVICE

#include "bluetooth.h"
#include "sdp_api.h"


#define SRC_BT (1)
#define SRC_USB (2)
#define SRC_BT_SANSUMG (0x0075)
#define SRC_BT_APPLE   (0X004c)
#define SRC_USB_APPLE   (0x05AC)

#ifdef __cplusplus
extern "C" {
#endif

typedef void btif_dip_client_t;

void btif_dip_init(void);
bt_status_t btif_dip_query_for_service(btif_dip_client_t *client_t,btif_remote_device_t *btDevice);
bool btif_dip_check_is_ios_device(btif_remote_device_t *btDevice);
void btif_dip_get_remote_info(btif_remote_device_t *btDevice);

#ifdef __cplusplus
}
#endif

#endif
#endif

