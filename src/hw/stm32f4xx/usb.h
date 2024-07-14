//
// Created by Angel Dust on 15/06/2024.
//

#ifndef TRX_FRONTEND_USB_H
#define TRX_FRONTEND_USB_H

#include "stdio.h"

#define USB_CONN_STATUS_CONNECTED 1
#define USB_CONN_STATUS_DISCONNECTED 0

#ifdef __cplusplus
extern "C" {
#endif

void OTG_HS_IRQHandler(void);

#ifdef __cplusplus
}
#endif

uint8_t getConnectionStatus();

#endif //TRX_FRONTEND_USB_H
