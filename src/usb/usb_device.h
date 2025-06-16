
#ifndef __USB_DEVICE__H__
#define __USB_DEVICE__H__

#include "usbd_def.h"

#ifdef __cplusplus
extern "C" {
#endif

void USB_SetupCDC(void);
void USB_SetupMSC(void);

#ifdef __cplusplus
}
#endif

#endif
