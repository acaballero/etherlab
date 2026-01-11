//
// Created by Angel Dust on 15/06/2024.
//

#ifndef TRX_FRONTEND_USB_H
#define TRX_FRONTEND_USB_H

#define USB_CONN_STATUS_CONNECTED 1
#define USB_CONN_STATUS_DISCONNECTED 0

#include "fatfs/fatfs.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"

void OTG_HS_IRQHandler(void);

#ifdef __cplusplus
}
#endif

uint8_t getUSBConnectionStatus();

// bool init_USB_MSC();
// bool init_USB_CDC();

#endif // TRX_FRONTEND_USB_H
